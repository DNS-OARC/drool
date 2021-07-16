-- DNS Reply Tool (drool)
--
-- Copyright (c) 2017-2021, OARC, Inc.
-- Copyright (c) 2017, Comcast Corporation
-- All rights reserved.
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
--
-- 1. Redistributions of source code must retain the above copyright
--    notice, this list of conditions and the following disclaimer.
--
-- 2. Redistributions in binary form must reproduce the above copyright
--    notice, this list of conditions and the following disclaimer in
--    the documentation and/or other materials provided with the
--    distribution.
--
-- 3. Neither the name of the copyright holder nor the names of its
--    contributors may be used to endorse or promote products derived
--    from this software without specific prior written permission.
--
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
-- "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
-- LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
-- FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
-- COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
-- INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
-- BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
-- LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
-- CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
-- LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
-- ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
-- POSSIBILITY OF SUCH DAMAGE.

module(...,package.seeall)

local ffi = require("ffi")
local C = ffi.C
ffi.cdef[[
struct respdiff_stats {
    int64_t sent, received, responses, timeouts, errors;
};
void* malloc(size_t);
void free(void*);
]]

local clock = require("dnsjit.lib.clock")
local object = require("dnsjit.core.objects")
require("dnsjit.core.timespec_h")

Respdiff = {}

function Respdiff.new(getopt)
    local self = setmetatable({
        path = nil,
        fname = nil,
        file = nil,
        hname = nil,
        host = nil,
        port = nil,

        use_threads = false,
        timeout = 10.0,
        layer = nil,
        input = nil,
        respdiff = nil,
        no_udp = false,
        no_tcp = false,
        udp_threads = 4,
        tcp_threads = 2,
        size = 10485760,

        packets = 0,
        queries = 0,
        sent = 0,
        received = 0,
        responses = 0,
        errors = 0,
        timeouts = 0,

        log = require("dnsjit.core.log").new("respdiff"),

        _timespec = ffi.new("core_timespec_t"),
        _udp_channels = {},
        _tcp_channels = {},
        _threads = {},
        _udpcli = nil,
        _tcpcli = nil,
        _stat_channels = {},
        _result_channels = {},
    }, { __index = Respdiff })

    if getopt then
        getopt.usage_desc = arg[1] .. " respdiff [options...] path name file name host port"
        getopt:add(nil, "no-udp", false, "Do not use UDP", "?")
        getopt:add(nil, "no-tcp", false, "Do not use TCP", "?")
        getopt:add("T", "threads", false, "Use threads", "?")
        getopt:add(nil, "udp-threads", 4, "Set the number of UDP threads to use, default 4", "?")
        getopt:add(nil, "tcp-threads", 2, "Set the number of TCP threads to use, default 2", "?")
        getopt:add(nil, "timeout", "10.0", "Set timeout for waiting on responses [seconds.nanoseconds], default 10.0", "?")
        getopt:add(nil, "size", 10485760, "Set the size (in bytes, multiple of OS page size) of the LMDB database, default 10485760.", "?")
    end

    return self
end

function Respdiff:getopt(getopt)
    local _, path, fname, file, hname, host, port = unpack(getopt.left)

    if getopt:val("no-udp") and getopt:val("no-tcp") then
        self.log:fatal("can not disable all transports")
    end
    if getopt:val("udp-threads") < 1 then
        self.log:fatal("--udp-threads must be 1 or greater")
    end
    if getopt:val("tcp-threads") < 1 then
        self.log:fatal("--tcp-threads must be 1 or greater")
    end

    if path == nil then
        self.log:fatal("no path given")
    end
    self.path = path
    if fname == nil then
        self.log:fatal("no name for file given")
    end
    self.fname = fname
    if file == nil then
        self.log:fatal("no file given")
    end
    self.file = file
    if hname == nil then
        self.log:fatal("no name for host given")
    end
    self.hname = hname
    if host == nil then
        self.log:fatal("no target host given")
    end
    self.host = host
    if port == nil then
        self.log:fatal("no target port given")
    end
    self.port = port

    self.use_threads = getopt:val("T")
    self.timeout = tonumber(getopt:val("timeout"))
    self.no_udp = getopt:val("no-udp")
    self.udp_threads = getopt:val("udp-threads")
    self.no_tcp = getopt:val("no-tcp")
    self.tcp_threads = getopt:val("tcp-threads")
    self.size = getopt:val("size")
end

local _thr_func = function(thr)
    local mode, thrid, host, port, chan, stats, to_sec, to_nsec, result = thr:pop(9)
    local log = require("dnsjit.core.log").new(mode .. "#" .. thrid)
    require("dnsjit.core.objects")
    local ffi = require("ffi")
    local C = ffi.C
    ffi.cdef[[
struct respdiff_stats {
    int64_t sent, received, responses, timeouts, errors;
};
void* malloc(size_t);
void free(void*);
]]

    local cli, recv, ctx, prod
    if mode == "udp" then
        cli = require("dnsjit.output.udpcli").new()
    else
        cli = require("dnsjit.output.tcpcli").new()
    end
    cli:timeout(to_sec, to_nsec)
    if cli:connect(host, port) ~= 0 then
        log:fatal("unable to connect to host " .. host .. " port " .. port)
    end
    recv, ctx = cli:receive()
    prod = cli:produce()

    local stat = ffi.cast("struct respdiff_stats*", C.malloc(ffi.sizeof("struct respdiff_stats")))
    ffi.fill(stat, ffi.sizeof("struct respdiff_stats"))
    ffi.gc(stat, C.free)

    while true do
        local obj = chan:get()
        if obj == nil then break end
        obj = ffi.cast("core_object_t*", obj)
        local resp = ffi.cast("core_object_t*", obj.obj_prev)

        log:info("sending query")
        recv(ctx, obj)
        stat.sent = stat.sent + 1

        local response = prod(ctx)
        if response == nil then
            log:warning("producer error")
            stat.errors = stat.errors + 1
            break
        end
        local payload = response:cast()
        if payload.len == 0 then
            stat.timeouts = stat.timeouts + 1
            log:info("timeout")
        else
            stat.responses = stat.responses + 1
            log:info("got response")
            resp.obj_prev = response:copy()
        end
        result:put(obj)
    end

    stat.errors = stat.errors + cli:errors()
    ffi.gc(stat, nil)
    stats:put(stat)
end

function Respdiff:setup()
    self.input = require("dnsjit.input.mmpcap").new()
    if self.input:open(self.file) ~= 0 then
        self.log:fatal("unable to open file " .. self.file)
    end

    self.layer = require("dnsjit.filter.layer").new()
    self.layer:producer(self.input)

    self.respdiff = require("dnsjit.output.respdiff").new(self.path, self.fname, self.hname, self.size)

    self._timespec.sec = math.floor(self.timeout)
    self._timespec.nsec = (self.timeout - math.floor(self.timeout)) * 1000000000

    if self.use_threads then
        if not self.no_udp then
            self.log:info("starting " .. self.udp_threads .. " UDP threads")
            for n = 1, self.udp_threads do
                local chan = require("dnsjit.core.channel").new()
                local stats = require("dnsjit.core.channel").new()
                local result = require("dnsjit.core.channel").new()
                local thr = require("dnsjit.core.thread").new()

                thr:start(_thr_func)
                thr:push("udp", n, self.host, self.port, chan, stats, tonumber(self._timespec.sec), tonumber(self._timespec.nsec), result)

                table.insert(self._udp_channels, chan)
                table.insert(self._stat_channels, stats)
                table.insert(self._result_channels, result)
                table.insert(self._threads, thr)
                self.log:info("UDP thread " .. n .. " started")
            end
        end
        if not self.no_tcp then
            self.log:info("starting " .. self.tcp_threads .. " TCP threads")
            for n = 1, self.tcp_threads do
                local chan = require("dnsjit.core.channel").new()
                local stats = require("dnsjit.core.channel").new()
                local result = require("dnsjit.core.channel").new()
                local thr = require("dnsjit.core.thread").new()

                thr:start(_thr_func)
                thr:push("tcp", n, self.host, self.port, chan, stats, tonumber(self._timespec.sec), tonumber(self._timespec.nsec), result)

                table.insert(self._tcp_channels, chan)
                table.insert(self._stat_channels, stats)
                table.insert(self._result_channels, result)
                table.insert(self._threads, thr)
                self.log:info("TCP thread " .. n .. " started")
            end
        end
    else
        if not self.no_udp then
            self._udpcli = require("dnsjit.output.udpcli").new()
            self._udpcli:timeout(self._timespec.sec, self._timespec.nsec)
            if self._udpcli:connect(self.host, self.port) ~= 0 then
                self.log:fatal("unable to connect to host " .. self.host .. " port " .. self.port .. " with UDP")
            end
        end
        if not self.no_tcp then
            self._tcpcli = require("dnsjit.output.tcpcli").new()
            self._tcpcli:timeout(self._timespec.sec, self._timespec.nsec)
            if self._tcpcli:connect(self.host, self.port) ~= 0 then
                self.log:fatal("unable to connect to host " .. self.host .. " port " .. self.port .. " with TCP")
            end
        end
    end
end

function Respdiff:run()
    local lprod, lctx = self.layer:produce()
    local udpcli = self._udpcli
    local tcpcli = self._tcpcli
    local log, packets, queries, responses, errors, timeouts = self.log, 0, 0, 0, 0, 0
    local send
    local resprecv, respctx = self.respdiff:receive()

    if self.use_threads then
        -- TODO: generate code for all udp/tcp channels, see split gen code in test
        local udpidx, tcpidx = 1, 1

        local send_udp = function(obj, resp)
            local chan = self._udp_channels[udpidx]
            if not chan then
                udpidx = 1
                chan = self._udp_channels[1]
            end
            local obj_copy, resp_copy = obj:copy(), resp:copy()
            obj_copy = ffi.cast("core_object_t*", obj_copy)
            obj_copy.obj_prev = resp_copy
            chan:put(obj_copy)
            udpidx = udpidx + 1
        end
        local send_tcp = function(obj, resp)
            local chan = self._tcp_channels[tcpidx]
            if not chan then
                tcpidx = 1
                chan = self._tcp_channels[1]
            end
            local obj_copy, resp_copy = obj:copy(), resp:copy()
            obj_copy = ffi.cast("core_object_t*", obj_copy)
            obj_copy.obj_prev = resp_copy
            chan:put(obj_copy)
            tcpidx = tcpidx + 1
        end
        if self._udp_channels[1] and self._tcp_channels[1] then
            send = function(obj, resp, protocol)
                if protocol.obj_type == object.UDP then
                    send_udp(obj, resp)
                elseif protocol.obj_type == object.TCP then
                    send_tcp(obj, resp)
                end
            end
        elseif self._udp_channels[1] then
            send = send_udp
        elseif self._tcp_channels[1] then
            send = send_tcp
        end
    else
        local urecv, uctx, uprod
        if udpcli then
            urecv, uctx = udpcli:receive()
            uprod = udpcli:produce()
        end
        local trecv, tctx, tprod
        if tcpcli then
            trecv, tctx = tcpcli:receive()
            tprod = tcpcli:produce()
        end

        local send_udp = function(obj, resp)
            log:info("sending udp query")
            urecv(uctx, obj)

            local response = uprod(uctx)
            if response == nil then
                log:warning("producer error")
                return
            end

            obj = ffi.cast("core_object_t*", obj)
            obj.obj_prev = resp
            resp = ffi.cast("core_object_t*", resp)

            local payload = response:cast()
            if payload.len == 0 then
                timeouts = timeouts + 1
                log:info("timeout")
                resp.obj_prev = nil
            else
                responses = responses + 1
                log:info("got response")
                resp.obj_prev = response
            end
            resprecv(respctx, obj)
        end
        local send_tcp = function(obj, resp)
            log:info("sending tcp query")
            trecv(tctx, obj)

            local response = tprod(tctx)
            if response == nil then
                log:warning("producer error")
                return
            end

            obj = ffi.cast("core_object_t*", obj)
            obj.obj_prev = resp
            resp = ffi.cast("core_object_t*", resp)

            local payload = response:cast()
            if payload.len == 0 then
                timeouts = timeouts + 1
                log:info("timeout")
                resp.obj_prev = nil
            else
                responses = responses + 1
                log:info("got response")
                resp.obj_prev = response
            end
            resprecv(respctx, obj)
        end
        if udpcli and tcpcli then
            send = function(obj, resp, protocol)
                if protocol.obj_type == object.UDP then
                    send_udp(obj, resp)
                elseif protocol.obj_type == object.TCP then
                    send_tcp(obj, resp)
                end
            end
        elseif udpcli then
            send = send_udp
        elseif tcpcli then
            send = send_tcp
        end
    end

    self.start_sec = clock:realtime()

    local qtbl = {}
    local dns = require("dnsjit.core.object.dns").new()
    while true do
        local obj = lprod(lctx)
        if obj == nil then break end
        packets = packets + 1
        local payload = obj:cast()
        if obj:type() == "payload" and payload.len > 0 then

            local transport = obj.obj_prev
            while transport ~= nil do
                if transport.obj_type == object.IP or transport.obj_type == object.IP6 then
                    break
                end
                transport = transport.obj_prev
            end
            local protocol = obj.obj_prev
            while protocol ~= nil do
                if protocol.obj_type == object.UDP or protocol.obj_type == object.TCP then
                    break
                end
                protocol = protocol.obj_prev
            end

            if transport ~= nil and protocol ~= nil then
                transport = transport:cast()
                protocol = protocol:cast()

                dns.obj_prev = obj
                if dns:parse_header() == 0 then
                    if dns.qr == 0 then
                        local k = string.format("%s %d %s %d", transport:source(), protocol.sport, transport:destination(), protocol.dport)
                        log:info("query " .. k .. " id " .. dns.id)
                        qtbl[k] = {
                            id = dns.id,
                            payload = payload:copy(),
                        }
                    else
                        local k = string.format("%s %d %s %d", transport:destination(), protocol.dport, transport:source(), protocol.sport)
                        local q = qtbl[k]
                        if q and q.id == dns.id then
                            log:info("response " .. k .. " id " .. dns.id)
                            queries = queries + 1
                            send(q.payload:uncast(), obj, protocol)
                            qtbl[k] = nil
                        end
                    end
                end
            end
        end

        if self.use_threads and responses < queries then
            for _, result in pairs(self._result_channels) do
                local res = result:try_get()
                if res ~= nil then
                    res = ffi.cast("core_object_t*", res)
                    resprecv(respctx, res)
                    responses = responses + 1

                    if res.obj_prev.obj_prev ~= nil then
                        ffi.cast("core_object_t*", res.obj_prev.obj_prev):free()
                    end
                    ffi.cast("core_object_t*", res.obj_prev):free()
                    res:free()
                end
            end
        end
    end

    if self.use_threads then
        for _, chan in pairs(self._udp_channels) do
            chan:put(nil)
        end
        for _, chan in pairs(self._tcp_channels) do
            chan:put(nil)
        end
    end

    self.packets = packets
    self.queries = queries
    self.sent = 0
    if udpcli then
        self.sent = self.sent + udpcli:packets()
    end
    if tcpcli then
        self.sent = self.sent + tcpcli:packets()
    end
    -- TODO: received == responses ?
    self.received = responses
    self.responses = responses
    self.timeouts = timeouts
    self.errors = errors
    if udpcli then
        self.errors = self.errors + udpcli:errors()
    end
    if tcpcli then
        self.errors = self.errors + tcpcli:errors()
    end
end

function Respdiff:finish()
    if self.use_threads then
        local left = 0 - self.responses
        self.responses = 0

        for _, thr in pairs(self._threads) do
            thr:stop()
        end
        for _, stats in pairs(self._stat_channels) do
            local stat = ffi.cast("struct respdiff_stats*", stats:get())
            self.sent = self.sent + stat.sent
            self.received = self.received + stat.received
            self.responses = self.responses + stat.responses
            self.timeouts = self.timeouts + stat.timeouts
            self.errors = self.errors + stat.errors
            C.free(stat)
        end
        self.sent = tonumber(self.sent)
        self.received = tonumber(self.received)
        self.responses = tonumber(self.responses)
        self.timeouts = tonumber(self.timeouts)
        self.errors = tonumber(self.errors)

        -- TODO: received == responses ?
        self.received = self.responses

        left = left + self.responses + self.timeouts
        local resprecv, respctx = self.respdiff:receive()
        local tries = 0
        while left > 0 and tries < 10000 do
            for _, result in pairs(self._result_channels) do
                local res = result:try_get()
                if res ~= nil then
                    res = ffi.cast("core_object_t*", res)
                    resprecv(respctx, res)
                    left = left - 1
                    tries = 0

                    if res.obj_prev.obj_prev ~= nil then
                        ffi.cast("core_object_t*", res.obj_prev.obj_prev):free()
                    end
                    ffi.cast("core_object_t*", res.obj_prev):free()
                    res:free()
                end
            end
            tries = tries + 1
        end
    end

    local end_sec = clock:realtime()
    self.respdiff:commit(self.start_sec, end_sec)
end

return Respdiff
