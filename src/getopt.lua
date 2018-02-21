function args()
    getopt = require("dnsjit.lib.getopt").new({
        { "c", nil, "", "", "?+" },
        { "l", nil, "", "", "?+" },
        { "L", nil, "", "", "?+" },
        { "f", nil, "", "", "?" },
        { "i", nil, "", "", "?+" },
        { "r", nil, "", "", "?+" },
        { "R", nil, "", "", "?" },
        { "n", nil, "", "", "?" },
        { "v", nil, 0, "", "?+" },
        { "V", nil, false, "", "?" },
    })
    arg = getopt:parse()

    local v = getopt:val("v")
    if v > 0 then
        log.enable("warning")
    end
    if v > 1 then
        log.enable("notice")
    end
    if v > 2 then
        log.enable("info")
    end
    if v > 3 then
        log.enable("debug")
    end

    if getopt:val("help") then
        print("usage: "..program.." [options]")
        print[[

  -c [type:]config
                Specify the configuration to use, if no type is given then
                config expects to be a file. Valid types are file and text.
                Can be given multiple times and will be processed in the
                given order. See drool.conf(5) for configuration syntax.
  -l facility[:level]
                Enable logging for facility, optional log level can be given
                to enable just that. Can be given multiple times and will be
                processed in the given order. See drool(1) for available
                facilities and log levels.
  -L facility[:level]
                Same as -l but to disable the given facility and log level.
  -f filter     Set the Berkeley Packet Filter to use.
  -i interface  Capture packets from interface, can be given multiple times.
  -r file.pcap  Read packets from PCAP file, can be given multiple times.
  -R mode       Specify the mode for reading PCAP files, see drool(1) for
                available modes.
  -n            Dry run mode, do not allocate any outbound sockets or
                generate any network traffic.
  -v            Enable verbose, a simple way to enable logging. Can be
                given multiple times to increase verbosity level.
  -h            Print this help and exit
  -V            Print version and exit
]]
        os.exit(1)
    elseif getopt:val("V") then
        print("drool v"..version)
        os.exit(0)
    end
end
