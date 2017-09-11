# TODO
- Analysis/dry-run mode is needed to check config, read pcaps and evaluate if it can be done

# Emulate source
- limit of maximum sources

# Client pool
- per context or one
- ip/net to use and the port range

# Issues
- dns cookies
- tcp multi query connection
  - check rfc, is it suppose to close the conn
  - do we get everything if pcap is filtered
- rate limiting at server
 - not really a problem if large enough client pool
- emulating multiple clients by ip address
  - not really a problem if large enough client pool
  - mapping may eat memory
- too few resources to emulate full traffic
  - do analysis/dry-run first
- max open files

# Problems
- sending real traffic
  - kernel may block/drop addresses not it's own
  - may change the link layers
- connection tracking at kernel level

# Compare Q+R in pcap
- Push R on queue also
- Keep long list of Q, FIFO
  - Check R against list of Q
  - If no match
    - Keep short list of R, FIFO
- Check Q against list of R
