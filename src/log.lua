function enable_log(_, facility, level)
    local facility_ok = false
    if level == nil then
        level = "all"
    end

    L:info("enable log facility %s level %s", facility, level)

    if facility == "all" then
        if level == "debug" then
            log.enable("debug")
        elseif level == "info" then
            log.enable("info")
        elseif level == "notice" then
            log.enable("notice")
        elseif level == "warning" then
            log.enable("warning")
        elseif level == "all" then
            log.enable("debug")
            log.enable("info")
            log.enable("notice")
            log.enable("warning")
        else
            L:fatal("invalid log level: %s", level)
        end
        facility_ok = true
    end

    if facility == "core" or facility == "all" then
        if level == "debug" then
            L:enable("debug")
        elseif level == "info" then
            L:enable("info")
        elseif level == "notice" then
            L:enable("notice")
        elseif level == "warning" then
            L:enable("warning")
        elseif level == "all" then
            L:enable("debug")
            L:enable("info")
            L:enable("notice")
            L:enable("warning")
        else
            L:fatal("invalid log level: %s", level)
        end
        facility_ok = true
    end

    if facility == "input" or facility == "all" then
        if level == "debug" or level == "info" or level == "notice" or level == "warning" or level == "all" then
            table.insert(facility_log["input"], { "enable", level })
        else
            L:fatal("invalid log level: %s", level)
        end
        facility_ok = true
    end

    if facility == "processing" or facility == "all" then
        if level == "debug" or level == "info" or level == "notice" or level == "warning" or level == "all" then
            table.insert(facility_log["processing"], { "enable", level })
        else
            L:fatal("invalid log level: %s", level)
        end
        facility_ok = true
    end

    if facility == "network" or facility == "all" then
        if level == "debug" or level == "info" or level == "notice" or level == "warning" or level == "all" then
            table.insert(facility_log["network"], { "enable", level })
        else
            L:fatal("invalid log level: %s", level)
        end
        facility_ok = true
    end

    if not facility_ok then
        L:fatal("invalid facility: %s", facility)
    end
end

function disable_log(_, facility, level)
    local facility_ok = false
    if level == nil then
        level = "all"
    end

    L:info("disable log facility %s level %s", facility, level)

    if facility == "all" then
        if level == "debug" then
            log.disable("debug")
        elseif level == "info" then
            log.disable("info")
        elseif level == "notice" then
            log.disable("notice")
        elseif level == "warning" then
            log.disable("warning")
        elseif level == "all" then
            log.disable("debug")
            log.disable("info")
            log.disable("notice")
            log.disable("warning")
        else
            L:fatal("invalid log level: %s", level)
        end
        facility_ok = true
    end

    if facility == "core" or facility == "all" then
        if level == "debug" then
            L:disable("debug")
        elseif level == "info" then
            L:disable("info")
        elseif level == "notice" then
            L:disable("notice")
        elseif level == "warning" then
            L:disable("warning")
        elseif level == "all" then
            L:disable("debug")
            L:disable("info")
            L:disable("notice")
            L:disable("warning")
        else
            L:fatal("invalid log level: %s", level)
        end
        facility_ok = true
    end

    if facility == "input" or facility == "all" then
        if level == "debug" or level == "info" or level == "notice" or level == "warning" or level == "all" then
            table.insert(facility_log["input"], { "disable", level })
        else
            L:fatal("invalid log level: %s", level)
        end
        facility_ok = true
    end

    if facility == "processing" or facility == "all" then
        if level == "debug" or level == "info" or level == "notice" or level == "warning" or level == "all" then
            table.insert(facility_log["processing"], { "disable", level })
        else
            L:fatal("invalid log level: %s", level)
        end
        facility_ok = true
    end

    if facility == "network" or facility == "all" then
        if level == "debug" or level == "info" or level == "notice" or level == "warning" or level == "all" then
            table.insert(facility_log["network"], { "disable", level })
        else
            L:fatal("invalid log level: %s", level)
        end
        facility_ok = true
    end

    if not facility_ok then
        L:fatal("invalid facility: %s", facility)
    end
end

function set_log(facility, obj)
    local v
    for _, v in pairs(facility_log[facility]) do
        local what, level = unpack(v)
        if what == "enable" then
            if level == "debug" then
                obj:log():enable("debug")
            elseif level == "info" then
                obj:log():enable("info")
            elseif level == "notice" then
                obj:log():enable("notice")
            elseif level == "warning" then
                obj:log():enable("warning")
            elseif level == "all" then
                obj:log():enable("debug")
                obj:log():enable("info")
                obj:log():enable("notice")
                obj:log():enable("warning")
            end
        elseif what == "disable" then
            if level == "debug" then
                obj:log():disable("debug")
            elseif level == "info" then
                obj:log():disable("info")
            elseif level == "notice" then
                obj:log():disable("notice")
            elseif level == "warning" then
                obj:log():disable("warning")
            elseif level == "all" then
                obj:log():disable("debug")
                obj:log():disable("info")
                obj:log():disable("notice")
                obj:log():disable("warning")
            end
        end
    end
end
