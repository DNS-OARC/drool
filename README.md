# DNS Replay Tool (drool)

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

## Usage example

This example reads all DNS queries from a PCAP file, sends them to
a nameserver at `127.0.0.1` and ignores both timings and replies.

```
drool \
  -vv \
  -c 'text:timing ignore;' \
  -c 'text:client_pool target "127.0.0.1" "53";' \
  -c 'text:client_pool skip_reply;' \
  -r file.pcap
```

## Build from GitHub

```
git clone https://github.com/DNS-OARC/drool.git
cd drool
git submodule update --init
sh autogen.sh
./configure
make
make test
make install
```

## Build from tarball

```
cd drool-VERSION...
./configure
make
make test
make install
```

## Author(s)

Jerry Lundström <jerry@dns-oarc.net>

## Copyright

Copyright (c) 2017, OARC, Inc.

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
