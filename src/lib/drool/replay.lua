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
struct replay_stats {
    int64_t sent, received, responses, timeouts, errors;
};
void* malloc(size_t);
void free(void*);
]]

local object = require("dnsjit.core.objects")
require("dnsjit.core.timespec_h")

Replay = {}

function Replay.new(getopt)
    local self = setmetatable({
        file = nil,
        host = nil,
        port = nil,
        no_responses = false,
        use_threads = false,
        print_dns = false,
        timeout = 10.0,
        timing = nil,
        timing_mode = "ignore",
        timing_opt = nil,
        layer = nil,
        input = nil,
        no_udp = false,
        no_tcp = false,
        udp_threads = 4,
        tcp_threads = 2,

        packets = 0,
        queries = 0,
        sent = 0,
        received = 0,
        responses = 0,
        errors = 0,
        timeouts = 0,

        log = require("dnsjit.core.log").new("replay"),

        _timespec = ffi.new("core_timespec_t"),
        _udp_channels = {},
        _tcp_channels = {},
        _threads = {},
        _udpcli = nil,
        _tcpcli = nil,
        _stat_channels = {},
    }, { __index = Replay })

    if getopt then
        getopt.usage_desc = arg[1] .. " replay [options...] file host port"
        getopt:add("t", "timing", "ignore", "Set the timing mode [mode=option], default ignore", "?")
        getopt:add("n", "no-responses", false, "Do not wait for responses before sending next request", "?")
        getopt:add(nil, "no-udp", false, "Do not use UDP", "?")
        getopt:add(nil, "no-tcp", false, "Do not use TCP", "?")
        getopt:add("T", "threads", false, "Use threads", "?")
        getopt:add(nil, "udp-threads", 4, "Set the number of UDP threads to use, default 4", "?")
        getopt:add(nil, "tcp-threads", 2, "Set the number of TCP threads to use, default 2", "?")
        getopt:add(nil, "timeout", "10.0", "Set timeout for waiting on responses [seconds.nanoseconds], default 10.0", "?")
        getopt:add("D", nil, false, "Show DNS queries and responses as processing goes", "?")
    end

    return self
end

function Replay:getopt(getopt)
    local _, file, host, port = unpack(getopt.left)

    if getopt:val("no-udp") and getopt:val("no-tcp") then
        self.log:fatal("can not disable all transports")
    end
    if getopt:val("udp-threads") < 1 then
        self.log:fatal("--udp-threads must be 1 or greater")
    end
    if getopt:val("tcp-threads") < 1 then
        self.log:fatal("--tcp-threads must be 1 or greater")
    end

    if file == nil then
        self.log:fatal("no file given")
    end
    self.file = file
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
    self.no_responses = getopt:val("n")
    self.print_dns = getopt:val("D")
    self.no_udp = getopt:val("no-udp")
    self.udp_threads = getopt:val("udp-threads")
    self.no_tcp = getopt:val("no-tcp")
    self.tcp_threads = getopt:val("tcp-threads")

    if getopt:val("t") ~= "ignore" then
        self.timing_mode, self.timing_opt = getopt:val("t"):match("(%w+)=([%w%.]+)")
        if self.timing_mode == nil then
            self.timing_mode = getopt:val("t")
        end
    end
end

local _thr_func = function(thr)
    local mode, thrid, host, port, chan, stats, resp, print_dns, to_sec, to_nsec = thr:pop(10)
    local log = require("dnsjit.core.log").new(mode .. "#" .. thrid)
    require("dnsjit.core.objects")
    local ffi = require("ffi")
    local C = ffi.C
    ffi.cdef[[
struct replay_stats {
    int64_t sent, received, responses, timeouts, errors;
};
void* malloc(size_t);
void free(void*);
]]
    local dns = require("dnsjit.core.object.dns").new()

    if print_dns == 1 then
        print_dns = function(payload)
            dns.obj_prev = payload
            dns:print()
        end
    else
        print_dns = nil
    end

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

    local stat = ffi.cast("struct replay_stats*", C.malloc(ffi.sizeof("struct replay_stats")))
    ffi.fill(stat, ffi.sizeof("struct replay_stats"))
    ffi.gc(stat, C.free)

    local send
    if resp == 0 then
        send = function(obj)
            log:info("sending query")
            recv(ctx, obj)
            if print_dns then
                print_dns(obj)
            end
        end
    else
        send = function(obj)
            log:info("sending query")
            recv(ctx, obj)
            if print_dns then
                print_dns(obj)
            end

            local response = prod(ctx)
            if response == nil then
                log:warning("producer error")
                return
            end
            local payload = response:cast()
            if payload.len == 0 then
                stat.timeouts = stat.timeouts + 1
                log:info("timeout")
                return
            end

            stat.responses = stat.responses + 1
            log:info("got response")
            if print_dns then
                print_dns(response)
            end
        end
    end

    while true do
        local obj = chan:get()
        if obj == nil then break end
        obj = ffi.cast("core_object_t*", obj)
        dns.obj_prev = obj
        if dns:parse_header() == 0 and dns.qr == 0 then
            send(obj)
            stat.sent = stat.sent + 1
        end
        obj:free()
    end

    stat.errors = cli:errors()
    ffi.gc(stat, nil)
    stats:put(stat)
end

function Replay:setup()
    self.input = require("dnsjit.input.mmpcap").new()
    if self.input:open(self.file) ~= 0 then
        self.log:fatal("unable to open file " .. self.file)
    end

    if self.timing_mode ~= "ignore" then
        self.timing = require("dnsjit.filter.timing").new()
        self.timing:producer(self.input)

        if self.timing_mode == "keep" then
        else
            if self.timing_mode == "inc" or self.timing_mode == "increase" then
                self.timing:increase(tonumber(self.timing_opt))
            elseif self.timing_mode == "red" or self.timing_mode == "reduce" then
                self.timing:reduce(tonumber(self.timing_opt))
            elseif self.timing_mode == "mul" or self.timing_mode == "multiply" then
                self.timing:multiply(tonumber(self.timing_opt))
            elseif self.timing_mode == "fix" or self.timing_mode == "fixed" then
                self.timing:fixed(tonumber(self.timing_opt))
            else
                self.log:fatal("Invalid timing mode " .. self.timing_mode)
            end
        end
    end

    self.layer = require("dnsjit.filter.layer").new()
    if self.timing then
        self.layer:producer(self.timing)
    else
        self.layer:producer(self.input)
    end

    self._timespec.sec = math.floor(self.timeout)
    self._timespec.nsec = (self.timeout - math.floor(self.timeout)) * 1000000000

    if self.use_threads then
        if not self.no_udp then
            self.log:info("starting " .. self.udp_threads .. " UDP threads")
            for n = 1, self.udp_threads do
                local chan = require("dnsjit.core.channel").new()
                local stats = require("dnsjit.core.channel").new()
                local thr = require("dnsjit.core.thread").new()

                thr:start(_thr_func)
                thr:push("udp", n, self.host, self.port, chan, stats)
                if self.no_responses then
                    thr:push(0)
                else
                    thr:push(1)
                end
                if self.print_dns then
                    thr:push(1)
                else
                    thr:push(0)
                end
                thr:push(tonumber(self._timespec.sec), tonumber(self._timespec.nsec))

                table.insert(self._udp_channels, chan)
                table.insert(self._stat_channels, stats)
                table.insert(self._threads, thr)
                self.log:info("UDP thread " .. n .. " started")
            end
        end
        if not self.no_tcp then
            self.log:info("starting " .. self.tcp_threads .. " TCP threads")
            for n = 1, self.tcp_threads do
                local chan = require("dnsjit.core.channel").new()
                local stats = require("dnsjit.core.channel").new()
                local thr = require("dnsjit.core.thread").new()

                thr:start(_thr_func)
                thr:push("tcp", n, self.host, self.port, chan, stats)
                if self.no_responses then
                    thr:push(0)
                else
                    thr:push(1)
                end
                if self.print_dns then
                    thr:push(1)
                else
                    thr:push(0)
                end
                thr:push(tonumber(self._timespec.sec), tonumber(self._timespec.nsec))

                table.insert(self._tcp_channels, chan)
                table.insert(self._stat_channels, stats)
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

function Replay:run()
    local lprod, lctx = self.layer:produce()
    local udpcli = self._udpcli
    local tcpcli = self._tcpcli
    local log, packets, queries, responses, errors, timeouts = self.log, 0, 0, 0, 0, 0
    local send

    if self.use_threads then
        -- TODO: generate code for all udp/tcp channels, see split gen code in test
        local udpidx, tcpidx = 1, 1

        local send_udp, send_tcp
        send_udp = function(obj)
            local chan = self._udp_channels[udpidx]
            if not chan then
                udpidx = 1
                chan = self._udp_channels[1]
            end
            chan:put(obj:copy())
            udpidx = udpidx + 1
        end
        send_tcp = function(obj)
            local chan = self._tcp_channels[tcpidx]
            if not chan then
                tcpidx = 1
                chan = self._tcp_channels[1]
            end
            chan:put(obj:copy())
            tcpidx = tcpidx + 1
        end
        if self._udp_channels[1] and self._tcp_channels[1] then
            send = function(obj)
                local protocol = obj.obj_prev
                while protocol ~= nil do
                    if protocol.obj_type == object.UDP then
                        send_udp(obj)
                        break
                    elseif protocol.obj_type == object.TCP then
                        send_tcp(obj)
                        break
                    end
                    protocol = protocol.obj_prev
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

        local dns = require("dnsjit.core.object.dns").new()
        local print_dns
        if self.print_dns then
            print_dns = function(payload)
                dns.obj_prev = payload
                dns:print()
            end
        end
        local send_udp, send_tcp
        if self.no_responses then
            send_udp = function(obj)
                log:info("sending udp query")
                urecv(uctx, obj)
                if print_dns then
                    print_dns(obj)
                end
            end
            send_tcp = function(obj)
                log:info("sending tcp query")
                trecv(tctx, obj)
                if print_dns then
                    print_dns(obj)
                end
            end
        else
            send_udp = function(obj)
                log:info("sending udp query")
                urecv(uctx, obj)
                if print_dns then
                    print_dns(obj)
                end

                local response = uprod(uctx)
                if response == nil then
                    log:warning("producer error")
                    return
                end
                local payload = response:cast()
                if payload.len == 0 then
                    timeouts = timeouts + 1
                    log:info("timeout")
                    return
                end

                responses = responses + 1
                log:info("got response")
                if print_dns then
                    print_dns(response)
                end
            end
            send_tcp = function(obj)
                log:info("sending tcp query")
                trecv(tctx, obj)
                if print_dns then
                    print_dns(obj)
                end

                local response = tprod(tctx)
                if response == nil then
                    log:warning("producer error")
                    return
                end
                local payload = response:cast()
                if payload.len == 0 then
                    timeouts = timeouts + 1
                    log:info("timeout")
                    return
                end

                responses = responses + 1
                log:info("got response")
                if print_dns then
                    print_dns(response)
                end
            end
        end
        if udpcli and tcpcli then
            send = function(obj)
                dns.obj_prev = obj
                if dns:parse_header() == 0 and dns.qr == 0 then
                    queries = queries + 1

                    local protocol = obj.obj_prev
                    while protocol ~= nil do
                        if protocol.obj_type == object.UDP then
                            send_udp(obj)
                            break
                        elseif protocol.obj_type == object.TCP then
                            send_tcp(obj)
                            break
                        end
                        protocol = protocol.obj_prev
                    end
                end
            end
        elseif udpcli then
            send = function(obj)
                dns.obj_prev = obj
                if dns:parse_header() == 0 and dns.qr == 0 then
                    queries = queries + 1
                    send_udp(obj)
                end
            end
        elseif tcpcli then
            send = function(obj)
                dns.obj_prev = obj
                if dns:parse_header() == 0 and dns.qr == 0 then
                    queries = queries + 1
                    send_tcp(obj)
                end
            end
        end
    end

    while true do
        local obj = lprod(lctx)
        if obj == nil then break end
        packets = packets + 1
        local payload = obj:cast()
        if obj:type() == "payload" and payload.len > 0 then
            send(obj)
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

function Replay:finish()
    if self.use_threads then
        for _, thr in pairs(self._threads) do
            thr:stop()
        end
        for _, stats in pairs(self._stat_channels) do
            local stat = ffi.cast("struct replay_stats*", stats:get())
            self.queries = self.queries + stat.sent
            self.sent = self.sent + stat.sent
            self.received = self.received + stat.received
            self.responses = self.responses + stat.responses
            self.timeouts = self.timeouts + stat.timeouts
            self.errors = self.errors + stat.errors
            C.free(stat)
        end
        self.queries = tonumber(self.queries)
        self.sent = tonumber(self.sent)
        self.received = tonumber(self.received)
        self.responses = tonumber(self.responses)
        self.timeouts = tonumber(self.timeouts)
        self.errors = tonumber(self.errors)

        -- TODO: received == responses ?
        self.received = self.responses
    end
end

return Replay
