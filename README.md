[![progress-banner](https://backend.codecrafters.io/progress/redis/397dd72b-29a1-4470-b6a5-ed98dd459199)](https://app.codecrafters.io/users/solomon-os?r=2qF)

This is a starting point for C++ solutions to the
["Build Your Own Redis" Challenge](https://codecrafters.io/challenges/redis).

In this challenge, you'll build a toy Redis clone that's capable of handling
basic commands like `PING`, `SET` and `GET`. Along the way we'll learn about
event loops, the Redis protocol and more.

**Note**: If you're viewing this repo on GitHub, head over to
[codecrafters.io](https://codecrafters.io) to try the challenge.

# Architecture

A toy Redis server written in modern C++ (C++23). It speaks the RESP protocol
and currently supports `PING`, `ECHO`, and the `SET`/`GET` foundations.

## Concurrency model — single-threaded epoll event loop

Rather than a thread (or process) per client, the server multiplexes **all**
connections on a single thread using a Linux `epoll` event loop. This is the
same model real Redis uses: one thread, no per-connection locking, no context
switching under load.

How it works (`src/server.cpp`):

- **One `epoll` instance** (`epoll_create1`) watches the listening socket plus
  every client socket. `epoll_wait` blocks until *any* of them is readable, then
  hands back only the ready fds — so the cost scales with active connections,
  not total connections.
- **The listening socket** is registered level-triggered (`EPOLLIN`). When it
  fires, `accept()` runs in a loop that drains the backlog until `EAGAIN`.
- **Client sockets** are registered **edge-triggered** (`EPOLLET`). Edge mode
  notifies once per readiness transition, so each handler must read until the
  socket would block — anything less and the connection stalls.
- **All sockets are non-blocking** (`O_NONBLOCK`). This is what makes the
  accept-until-`EAGAIN` and read-until-`EAGAIN` loops safe: a syscall with no
  more data returns immediately instead of parking the single thread.
- **Per-connection state** lives in a `Conn` object (owned by `unique_ptr`,
  keyed by fd in a map). Each `Conn` carries its own input and output buffers,
  so a partial RESP frame is buffered and resumed on the next `epoll` wakeup
  without blocking other clients.
- **Socket tuning**: `SO_REUSEADDR` for fast restart, `TCP_NODELAY` to disable
  Nagle so small replies (`+PONG\r\n`) go out immediately.

The payoff: many concurrent clients handled on one thread with no locks, no
thread pool, and predictable latency.

# Passing the first stage

The entry point for your Redis implementation is in `src/main.cpp`. Study and
uncomment the relevant code, then run the command below to execute the tests on
our servers:

```sh
codecrafters submit
```

That's all!

# Stage 2 & beyond

Note: This section is for stages 2 and beyond.

1. Ensure you have `cmake` installed locally
1. Run `./your_program.sh` to run your Redis server, which is implemented in
   `src/main.cpp`.
1. Run `codecrafters submit` to submit your solution to CodeCrafters. Test
   output will be streamed to your terminal.
