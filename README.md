# DNS Replay Tool (drool)

[![Build Status](https://travis-ci.org/DNS-OARC/drool.svg?branch=develop)](https://travis-ci.org/DNS-OARC/drool)

`drool` can replay DNS traffic from packet capture (PCAP) files and send
it to a specified server, with options such as to manipulate the timing
between packets, as well as loop packets infinitely or for a set number
of iterations. This tool's goal is to be able to produce a high amount
of UDP packets per second and TCP sessions per second on common hardware.

The purpose can be to simulate Distributed Denial of Service (DDoS) attacks
on the DNS and measure normal DNS querying. For example, the tool could
enable you to take a snapshot of a DDoS and be able to replay it later
to test if new code or hardening techniques are useful, safe & effective.
Another example is to be able to replay a packet stream for a
bug that is sequence- and/or timing-related in order to validate the
efficacy of subsequent bug fixes.

## Known Issues

- IP fragments are currently not processed and will be discarded.
- TCP sessions are not reassembled, each packet is parsed as DNS after
  discarding the first two bytes.

## Usage example

Send all DNS queries twice as fast as found in the PCAP file to localhost
using UDP:

```shell
drool -vv \
  -c 'text:timing multiply 0.5; client_pool target "127.0.0.1" "53"; client_pool sendas udp;' \
  -r file.pcap
```

Only look for DNS queries in TCP traffic and send it to localhost:

```shell
drool -vv \
  -c 'text:filter "tcp"; client_pool target "127.0.0.1" "53";' \
  -r file.pcap
```

Listen for DNS queries on eth0 and send them to an (assuming) internal server:

```shell
drool -vv \
  -c 'text:filter "port 53"; client_pool target "172.16.1.2" "53";' \
  -i eth0
```

Take all UDP DNS queries found in the PCAP file and send them as fast as
possible to localhost by ignoring both timings, replies and starting 5
contexts (threads) that will simultaneously send queries:

```shell
drool -vv \
  -c 'text:filter "udp"; timing ignore; context client_pools 5; client_pool target "127.0.0.1" "53"; client_pool skip_reply;' \
  -r file.pcap
```

## Dependencies

`drool` is built upon [dnsjit](https://github.com/DNS-OARC/dnsjit) and
requires it to be installed.

## Build

```
sh autogen.sh
./configure
make
make test
make install
```

## Author(s)

Jerry Lundström <jerry@dns-oarc.net>

## Copyright

Copyright (c) 2017-2018, OARC, Inc.

Copyright (c) 2017, Comcast Corporation

All rights reserved.

```
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in
   the documentation and/or other materials provided with the
   distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
```
