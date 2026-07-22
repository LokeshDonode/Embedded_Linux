# Socket Programming Notes

### Key Terms First

- `socket()` — Creates endpoint, returns fd
- `bind()` — Attach fd to local IP:port
- `listen()` — Mark socket passive; set pending-connection queue size (backlog)
- `accept()` — Dequeue 1 connection, return new fd (TCP only)
- `connect()` — TCP: trigger 3-way handshake. UDP: set default peer addr
- `send()`/`recv()` — TCP data transfer
- `sendto()`/`recvfrom()` — UDP data transfer — must pass addr each time
- `AF_INET` — IPv4 address family
- `SOCK_STREAM` — TCP — connection-oriented, reliable, ordered
- `SOCK_DGRAM` — UDP — connectionless, unreliable, no order guarantee
- `sockaddr_in` — Struct: `sin_family` + `sin_port` + `sin_addr`
- `htons`/`htonl` — Host→Network byte order (short/long). Network = big-endian
- `ntohs`/`ntohl` — Network→Host byte order
- `inet_pton` — Text IP (`"192.168.1.5"`) → binary
- `inet_ntop` — Binary → text IP
- `SO_REUSEADDR` — Bypass TIME_WAIT; reuse port immediately after server restart
- `INADDR_ANY` — Bind to all interfaces (`0.0.0.0`)
- `backlog` — Max half-open connections queued before `accept()` pulls them
- `TIME_WAIT` — TCP state after active close — lasts 2×MSL (~120s), blocks port reuse
- `MSL` — Maximum Segment Lifetime ~60s
- `TCP_NODELAY` — Disable Nagle — send small packets immediately (critical for low-latency)
- `Nagle algo` — Kernel buffers small TCP writes until ACK received or buffer full
- `shutdown()` — Half-close: `SHUT_WR` stops sending, `SHUT_RD` stops receiving
- `O_NONBLOCK` — Makes socket non-blocking; syscalls return `EAGAIN` if not ready
- `epoll` — Linux I/O multiplexing, O(1) for large fd sets (vs `select` O(n))
- `MSS` — Max TCP payload ~1460 bytes (MTU 1500 − IP hdr 20 − TCP hdr 20)
