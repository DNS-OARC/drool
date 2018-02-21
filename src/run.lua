function run()
    local input = require("dnsjit.input.pcap").new()
    set_log("input", input)
    input:only_queries(true)

    o = getopt:val("f")
    if o > "" then
        bpf = f
    end
    if bpf and bpf > "" then
        input:filter(bpf)
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
        input:open(v)
    end
    for k, v in pairs(o) do
        input:open(v)
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
        input:open_offline(v)
    end
    for k, v in pairs(o) do
        input:open_offline(v)
    end

    function new_output()
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
        input:run()
        output:stop()
    else
        local n
        local outputs = {}
        local roundrobin = require("dnsjit.filter.roundrobin").new()
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
        input:run()
        for _, o in pairs(outputs) do
            o:stop()
        end
    end

    local start_sec, start_nsec, end_sec, end_nsec, runtime

    start_sec, start_nsec = input:start_time()
    end_sec, end_nsec = input:end_time()
    runtime = 0
    if end_sec > start_sec then
        runtime = ((end_sec - start_sec) - 1) + ((1000000000 - start_nsec + end_nsec)/1000000000)
    elseif end_sec == start_sec and end_nsec > start_nsec then
        runtime = (end_nsec - start_nsec) / 1000000000
    end

    print("runtime", runtime)
    print("packets", input:packets(), input:packets()/runtime, "/pps")
    print("queries", input:queries(), input:queries()/runtime, "/qps")
    print("dropped", input:dropped())
    print("ignored", input:ignored())
    print("total", input:queries() + input:dropped() + input:ignored())
end
