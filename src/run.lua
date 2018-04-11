function run()
    local input

    if backend.input == "pcapthread" then
        local pcapthread = pcall(function() require("dnsjit.input.pcapthread") end)
        if not pcapthread then
            pcapthread = require("dnsjit.input.pcap")
        else
            pcapthread = require("dnsjit.input.pcapthread")
        end
        input = pcapthread.new()
        input:only_queries(true)
    elseif backend.input == "fpcap" then
        input = require("dnsjit.input.fpcap").new()
    elseif backend.input == "mmpcap" then
        input = require("dnsjit.input.mmpcap").new()
    elseif backend.input == "pcap" then
        input = require("dnsjit.input.pcap").new()
    end
    set_log("input", input)
    if backend.input == "fpcap" or backend.input == "mmpcap" then
        input:use_shared(true)
    end

    o = getopt:val("f")
    if o > "" then
        bpf = f
    end
    if bpf and bpf > "" then
        if backend.input == "pcapthread" then
            input:filter(bpf)
        else
            L:fatal("input backend "..backend.input.." can not use BPF")
        end
    end

    o = getopt:val("i")
    if type(o) == "string" then
        if o > "" then
            o = { o }
        else
            o = {}
        end
    end
    for k, v in pairs(interfaces) do
        if backend.input == "pcapthread" then
            input:open(v)
        else
            L:fatal("input backend "..backend.input.." can not open interfaces")
        end
    end
    for k, v in pairs(o) do
        if backend.input == "pcapthread" then
            input:open(v)
        else
            L:fatal("input backend "..backend.input.." can not open interfaces")
        end
    end

    o = getopt:val("r")
    if type(o) == "string" then
        if o > "" then
            o = { o }
        else
            o = {}
        end
    end
    for k, v in pairs(files) do
        if backend.input == "pcapthread" or backend.input == "pcap" then
            input:open_offline(v)
        elseif backend.input == "fpcap" or backend.input == "mmpcap" then
            input:open(v)
        else
            L:fatal("input backend "..backend.input.." can not open offline files")
        end
    end
    for k, v in pairs(o) do
        if backend.input == "pcapthread" or backend.input == "pcap" then
            input:open_offline(v)
        elseif backend.input == "fpcap" or backend.input == "mmpcap" then
            input:open(v)
        else
            L:fatal("input backend "..backend.input.." can not open offline files")
        end
    end

    function new_timing()
        local timing = require("dnsjit.filter.timing").new()
        set_log("processing", timing)
        if timing_mode == "keep" or timing_mode == "best_effort" then
            timing:keep()
        elseif mode == "increase" then
            timing:increase(timing_arg)
        elseif mode == "reduce" then
            timing:reduce(timing_arg)
        elseif mode == "multiply" then
            timing:multiply(timing_arg)
        end
        return timing
    end

    local start_sec, start_nsec, end_sec, end_nsec, runtime, output_queries

    if backend.output == "cpool" then
        local new_output = function()
            local output = require("dnsjit.output.cpool").new(host, port)
            set_log("network", output)
            if max_clients ~= nil then
                output:max_clients(max_clients)
            end
            if client_ttl ~= nil then
                output:client_ttl(client_ttl)
            end
            if skip_reply ~= nil then
                output:skip_reply(skip_reply)
            end
            if max_reuse_clients ~= nil then
                output:max_reuse_clients(max_reuse_clients)
            end
            if sendas ~= nil then
                output:sendas(sendas)
            end
            output:dry_run(getopt:val("n"))
            return output
        end

        if client_pools == nil or client_pools == 1 then
            local output = new_output()
            if timing_mode then
                local timing = new_timing()
                input:receiver(timing)
                timing:receiver(output)
            else
                input:receiver(output)
            end
            output:start()
            start_sec, start_nsec = clock:monotonic()
            input:run()
            output:stop()
        else
            local n
            local outputs = {}
            local roundrobin = require("dnsjit.filter.split").new()
            set_log("processing", roundrobin)
            for n = 1, client_pools do
                local output = new_output()
                if timing_mode then
                    local timing = new_timing()
                    roundrobin:receiver(timing)
                    timing:receiver(output)
                else
                    roundrobin:receiver(output)
                end
                table.insert(outputs, output)
            end
            input:receiver(roundrobin)
            for _, o in pairs(outputs) do
                o:start()
            end
            start_sec, start_nsec = clock:monotonic()
            input:run()
            for _, o in pairs(outputs) do
                o:stop()
            end
        end
    elseif backend.output == "udpcli" then
        if max_clients == nil or max_clients == 1 then
            local layer = require("dnsjit.filter.layer").new()
            set_log("processing", layer)
            local output = require("dnsjit.output.udpcli").new(host, port)
            set_log("network", output)

            layer:receiver(output)
            input:receiver(layer)
            start_sec, start_nsec = clock:monotonic()
            input:run()
            output_queries = output:packets()
        else
            local layers = {}
            local outputs = {}
            local thread = require("dnsjit.filter.thread").new()
            set_log("processing", thread)
            local n

            thread:use_writers(true)

            for n = 1, max_clients do
                local layer = require("dnsjit.filter.layer").new()
                set_log("processing", layer)
                local output = require("dnsjit.output.udpcli").new(host, port)
                set_log("network", output)

                layer:receiver(output)
                thread:receiver(layer)
                table.insert(layers, layer)
                table.insert(outputs, output)
            end

            input:receiver(thread)
            thread:start()
            start_sec, start_nsec = clock:monotonic()
            input:run()
            thread:stop()
            output_queries = 0
            for _, output in pairs(outputs) do
                output_queries = output_queries + output:packets()
            end
        end
    end
    end_sec, end_nsec = clock:monotonic()

    runtime = 0
    if end_sec > start_sec then
        runtime = ((end_sec - start_sec) - 1) + ((1000000000 - start_nsec + end_nsec)/1000000000)
    elseif end_sec == start_sec and end_nsec > start_nsec then
        runtime = (end_nsec - start_nsec) / 1000000000
    end

    print("runtime", runtime)
    print("packets", input:packets(), input:packets()/runtime, "/pps")
    if backend.input == "pcapthread" then
        print("queries", input:queries(), input:queries()/runtime, "/qps")
        print("dropped", input:dropped())
        print("ignored", input:ignored())
        print("total", input:queries() + input:dropped() + input:ignored())
    end
    if output_queries ~= nil then
        print("output", output_queries, output_queries/runtime, "/qps")
    end
end
