conf:func("log", enable_log)
conf:func("nolog", disable_log)
conf:func("read", function(_, file)
    table.insert(files, file)
end)
conf:func("input", function(_, interface)
    table.insert(interfaces, interface)
end)
conf:func("filter", function(_, arg1)
    bpf = arg1
end)
conf:func("timing", function(_, mode, arg1)
    if mode == "ignore" then
        return
    elseif mode == "keep" or mode == "best_effort" or mode == "increase" or mode == "reduce" or mode == "multiply" then
        timing_arg = arg1
    else
        L:fatal("unknown timing mode: %s", mode)
    end
end)
conf:func("context", function(_, option, arg1)
    if option == "client_pools" then
        client_pools = arg1
    else
        L:fatal("unknown context option: %s", option)
    end
end)
conf:func("client_pool", function(_, option, arg1, arg2)
    if option == "target" then
        host = arg1
        port = arg2
    elseif option == "max_clients" then
        max_clients = arg1
    elseif option == "client_ttl" then
        client_ttl = arg1
    elseif option == "skip_reply" then
        skip_reply = true
    elseif option == "max_reuse_clients" then
        max_reuse_clients = arg1
    elseif option == "sendas" then
        sendas = arg1
    else
        L:fatal("unknown client_pool option: %s", option)
    end
end)
conf:func("backend", function(_, section, module)
    if section == "input" then
        if module == "pcapthread" then
            backend.input = "pcapthread"
        elseif module == "fpcap" then
            backend.input = "fpcap"
        elseif module == "mmpcap" then
            backend.input = "mmpcap"
        elseif module == "pcap" then
            backend.input = "pcap"
        else
            L:fatal("unknown backend input: %s", module)
        end
    elseif section == "output" then
        if module == "cpool" then
            backend.output = "cpool"
        elseif module == "udpcli" then
            backend.output = "udpcli"
        else
            L:fatal("unknown backend output: %s", module)
        end
    else
        L:fatal("unknown backend section: %s", section)
    end
end)
