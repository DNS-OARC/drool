-- DNS Reply Tool (drool)
--
-- Copyright (c) 2017-2018, OARC, Inc.
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
        no_responses = false,
        threads = false,
        print_dns = false,
        packets = 0,
        queries = 0,
        sent = 0,
        received = 0,
        responses = 0,
        errors = 0,
        timeouts = 0,
        log = require("dnsjit.core.log").new("replay"),

        _udp_channels = {},
        _tcp_channels = {},
        _threads = {},
        _timeout = 10.0,
        _timespec = ffi.new("core_timespec_t"),
    }, { __index = Replay })

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

    return self
end

function Replay:setup(getopt)
    local _, file, host, port = unpack(getopt.left)

    if file == nil then
        self.log:fatal("no file given")
    end
    if host == nil then
        self.log:fatal("no target host given")
    end
    if port == nil then
        self.log:fatal("no target port given")
    end
    if getopt:val("no-udp") and getopt:val("no-tcp") then
        self.log:fatal("can not disable all transports")
    end
    self.threads = getopt:val("T")
    self._timeout = tonumber(getopt:val("timeout"))
    self._timespec.sec = math.floor(self._timeout)
    self._timespec.nsec = (self._timeout - math.floor(self._timeout)) * 1000000000
    self.log:info("timeout %d.%09d", self._timespec.sec, self._timespec.nsec)
    self.no_responses = getopt:val("n")
    self.print_dns = getopt:val("D")

    local input = require("dnsjit.input.mmpcap").new()
    if input:open(file) ~= 0 then
        self.log:fatal("unable to open file " .. file)
    end

    local timing
    if getopt:val("t") ~= "ignore" then
        timing = require("dnsjit.filter.timing").new()
        timing:producer(input)

        local mode, opt = getopt:val("t")
        if mode == "keep" then
        else
            mode, opt = mode:match("^(%w+)=(.*)")
            if mode == "inc" or mode == "increase" then
                timing:increase(tonumber(opt))
            elseif mode == "red" or mode == "reduce" then
                timing:reduce(tonumber(opt))
            elseif mode == "mul" or mode == "multiply" then
                timing:multiply(tonumber(opt))
            elseif mode == "fix" or mode == "fixed" then
                timing:fixed(tonumber(opt))
            end
        end
    end

    local layer = require("dnsjit.filter.layer").new()
    if timing then
        layer:producer(timing)
    else
        layer:producer(input)
    end

    if self.threads then
        local stats = require("dnsjit.core.channel").new()
        self._stats = stats

        if not getopt:val("no-udp") then
            if getopt:val("udp-threads") < 1 then
                self.log:fatal("--udp-threads must be 1 or greater")
            end

            self.log:info("starting " .. getopt:val("udp-threads") .. " UDP threads")
            for n = 1, getopt:val("udp-threads") do

                local chan = require("dnsjit.core.channel").new()
                local thr = require("dnsjit.core.thread").new()

                thr:start(function(thr)
                    local thrid = string.format("udp#%d", thr:pop())
                    local log = require("dnsjit.core.log").new(thrid)
                    require("dnsjit.core.objects")
                    require("drool.replay_stats")
                    local ffi = require("ffi")
                    local host = thr:pop()
                    local port = thr:pop()
                    local chan = thr:pop()
                    local stats = thr:pop()
                    local resp = thr:pop()
                    local print_dns, dns = thr:pop()
                    local to_sec, to_nsec = thr:pop(), thr:pop()

                    if print_dns == 1 then
                        dns = require("dnsjit.core.object.dns").new()
                        print_dns = function(payload)
                            dns.obj_prev = payload
                            dns:print()
                        end
                    else
                        print_dns = nil
                    end

                    local udpcli, urecv, uctx, uprod
                    udpcli = require("dnsjit.output.udpcli").new()
                    udpcli:timeout(to_sec, to_nsec)
                    if udpcli:connect(host, port) ~= 0 then
                        log:fatal("unable to connect to host " .. host .. " port " .. port)
                    end
                    urecv, uctx = udpcli:receive()
                    uprod = udpcli:produce()

                    local stat = ffi.cast("struct replay_stats*", ffi.C.malloc(ffi.sizeof("struct replay_stats")))
                    stat.sent = 0
                    stat.received = 0
                    stat.responses = 0
                    stat.timeouts = 0
                    stat.errors = 0
                    ffi.gc(stat, ffi.C.free)

                    local send
                    if resp == 0 then
                        send = function(obj)
                            log:info("sending udp query")
                            urecv(uctx, obj)
                            if print_dns then
                                print_dns(obj)
                            end
                        end
                    else
                        send = function(obj)
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

                    local dns = require("dnsjit.core.object.dns").new()
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

                    stat.errors = udpcli:errors()
                    ffi.gc(stat, nil)
                    stats:put(stat)
                end)

                thr:push(n)
                thr:push(host)
                thr:push(port)
                thr:push(chan)
                thr:push(stats)
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
                thr:push(tonumber(self._timespec.sec))
                thr:push(tonumber(self._timespec.nsec))

                table.insert(self._udp_channels, chan)
                table.insert(self._threads, thr)
                self.log:info("UDP thread " .. n .. " started")
            end
        end

        if not getopt:val("no-tcp") then
            if getopt:val("tcp-threads") < 1 then
                self.log:fatal("--tcp-threads must be 1 or greater")
            end

            self.log:info("starting " .. getopt:val("tcp-threads") .. " TCP threads")
            for n = 1, getopt:val("tcp-threads") do

                local chan = require("dnsjit.core.channel").new()
                local thr = require("dnsjit.core.thread").new()

                thr:start(function(thr)
                    local thrid = string.format("tcp#%d", thr:pop())
                    local log = require("dnsjit.core.log").new(thrid)
                    require("dnsjit.core.objects")
                    require("drool.replay_stats")
                    local ffi = require("ffi")
                    local host = thr:pop()
                    local port = thr:pop()
                    local chan = thr:pop()
                    local stats = thr:pop()
                    local resp = thr:pop()
                    local print_dns, dns = thr:pop()
                    local to_sec, to_nsec = thr:pop(), thr:pop()

                    if print_dns == 1 then
                        dns = require("dnsjit.core.object.dns").new()
                        print_dns = function(payload)
                            dns.obj_prev = payload
                            dns:print()
                        end
                    else
                        print_dns = nil
                    end

                    local tcpcli, trecv, tctx, tprod
                    tcpcli = require("dnsjit.output.tcpcli").new()
                    tcpcli:timeout(to_sec, to_nsec)
                    if tcpcli:connect(host, port) ~= 0 then
                        log:fatal("unable to connect to host " .. host .. " port " .. port)
                    end
                    trecv, tctx = tcpcli:receive()
                    tprod = tcpcli:produce()

                    local stat = ffi.cast("struct replay_stats*", ffi.C.malloc(ffi.sizeof("struct replay_stats")))
                    stat.sent = 0
                    stat.received = 0
                    stat.responses = 0
                    stat.timeouts = 0
                    stat.errors = 0
                    ffi.gc(stat, ffi.C.free)

                    local send
                    if resp == 0 then
                        send = function(obj)
                            log:info("sending tcp query")
                            trecv(tctx, obj)
                            if print_dns then
                                print_dns(obj)
                            end
                        end
                    else
                        send = function(obj)
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

                    local dns = require("dnsjit.core.object.dns").new()
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

                    ffi.gc(stat, nil)
                    stats:put(stat)
                end)

                thr:push(n)
                thr:push(host)
                thr:push(port)
                thr:push(chan)
                thr:push(stats)
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
                thr:push(tonumber(self._timespec.sec))
                thr:push(tonumber(self._timespec.nsec))

                table.insert(self._tcp_channels, chan)
                table.insert(self._threads, thr)
                self.log:info("TCP thread " .. n .. " started")
            end
        end
    else
        local udpcli
        if not getopt:val("no-udp") then
            udpcli = require("dnsjit.output.udpcli").new()
            udpcli:timeout(self._timespec.sec, self._timespec.nsec)
            if udpcli:connect(host, port) ~= 0 then
                self.log:fatal("unable to connect to host " .. host .. " port " .. port .. " with UDP")
            end
        end
        local tcpcli
        if not getopt:val("no-tcp") then
            tcpcli = require("dnsjit.output.tcpcli").new()
            tcpcli:timeout(self._timespec.sec, self._timespec.nsec)
            if tcpcli:connect(host, port) ~= 0 then
                self.log:fatal("unable to connect to host " .. host .. " port " .. port .. " with TCP")
            end
        end

        self.udpcli = udpcli
        self.tcpcli = tcpcli
    end

    self.file = file
    self.host = host
    self.port = port
    self.input = input
    self.layer = layer
end

function Replay:run()
    local lprod, lctx = self.layer:produce()
    local log, packets, queries, responses, errors, timeouts = self.log, 0, 0, 0, 0, 0

    if self.threads then
        -- TODO: generate code for all udp/tcp channels, see split gen code in test
        local udpidx, tcpidx = 1, 1

        local send, send_udp, send_tcp
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

        while true do
            local obj = lprod(lctx)
            if obj == nil then break end
            packets = packets + 1
            local payload = obj:cast()
            if obj:type() == "payload" and payload.len > 0 then
                send(obj)
            end
        end

        for _, chan in pairs(self._udp_channels) do
            chan:put(nil)
        end
        for _, chan in pairs(self._tcp_channels) do
            chan:put(nil)
        end

        self.packets = packets
        self.queries = 0
        self.sent = 0
        self.received = 0
        self.responses = 0
        self.errors = 0

        return
    end

    local udpcli, urecv, uctx, uprod = self.udpcli
    if udpcli then
        urecv, uctx = udpcli:receive()
        uprod = udpcli:produce()
    end
    local tcpcli, trecv, tctx, tprod = self.tcpcli
    if tcpcli then
        trecv, tctx = tcpcli:receive()
        tprod = tcpcli:produce()
    end

    local print_dns, dns
    if self.print_dns then
        dns = require("dnsjit.core.object.dns").new()
        print_dns = function(payload)
            dns.obj_prev = payload
            dns:print()
        end
    end
    local send, send_udp, send_tcp
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
    elseif udpcli then
        send = send_udp
    elseif tcpcli then
        send = send_tcp
    end

    local dns = require("dnsjit.core.object.dns").new()
    while true do
        local obj = lprod(lctx)
        if obj == nil then break end
        packets = packets + 1
        local payload = obj:cast()
        if obj:type() == "payload" and payload.len > 0 then
            dns.obj_prev = obj
            if dns:parse_header() == 0 and dns.qr == 0 then
                queries = queries + 1
                send(obj)
            end
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
    if self.threads then
        local threads = 0
        for _, thr in pairs(self._threads) do
            thr:stop()
            threads = threads + 1
        end

        for n = 1, threads do
            local stat = ffi.cast("struct replay_stats*", self._stats:get())
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
    end
end

return Replay
