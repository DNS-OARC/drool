function main()
    local k, v, o

    o = getopt:val("c")
    if type(o) == "string" then
        if o > "" then
            o = { o }
        else
            o = {}
        end
    end
    for k, v in pairs(o) do
        local t = v:sub(1, 5)
        if t == "text:" then
            conf:line(v:sub(6))
        elseif t == "file:" then
            conf:file(v:sub(6))
        else
            L:fatal("unknown conf type: %s", v:sub(1,4))
        end
    end

    o = getopt:val("l")
    if type(o) == "string" then
        if o > "" then
            o = { o }
        else
            o = {}
        end
    end
    for k, v in pairs(o) do
        local n = v:find(":")
        print(v, n)
        if n == nil then
            enable_log(nil, v)
        else
            enable_log(nil, v:sub(1,n), v:sub(n+1))
        end
    end

    o = getopt:val("L")
    if type(o) == "string" then
        if o > "" then
            o = { o }
        else
            o = {}
        end
    end
    for k, v in pairs(o) do
        local n = v:find(":")
        if n == nil then
            disable_log(nil, v)
        else
            disable_log(nil, v:sub(1,n), v:sub(n+1))
        end
    end

    o = getopt:val("R")
    if o > "" then
        if o:sub(1,5) == "iter:" then
            read_iter = tonumber(o:sub(6))
        elseif o == "loop" then
            read_loop = true
        else
            L:fatal("unknown read mode: %s", o)
        end
    end


    if read_loop then
        while true do
            run()
            collectgarbage()
        end
    else
        for iter = 1, read_iter do
            run()
            collectgarbage()
        end
    end
end

args()
main()
