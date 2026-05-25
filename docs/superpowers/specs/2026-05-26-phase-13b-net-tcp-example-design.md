# Phase 13B Net TCP Example Design

## Goal

Add the first `nexus_net` example: a local TCP echo demo that exercises the public `TcpListener` and `TcpClient` APIs without external network access.

## Scope

This phase adds `nexus_example_net_tcp_echo` under `examples/net_tcp_echo`. The example binds a listener to `127.0.0.1:0`, accepts one client connection, echoes a short message, prints the response, and exits.

## Requirements

- Example source must include only NexusKit public net headers, not Asio, httplib, or websocketpp headers.
- Example target must link `nexus::net` and must not link private backend targets.
- `examples/CMakeLists.txt` must gate the example with `if(TARGET nexus::net)`.
- The example must use an OS-assigned loopback port instead of a fixed port.
- The example must avoid external network access and return non-zero on failed connect, accept, read, or write.
- README and `docs/modules/net.md` must mention the example.
- The examples contract test must protect the public-header and CMake gating contract.

## Non-Goals

- No UDP, HTTP, WebSocket, TLS, or multi-client example in this phase.
- No new net API.
- No CI runtime execution requirement for the example; build-only coverage is enough for this phase.

## Verification

- Add a failing contract test before creating the example.
- Run the contract test after implementation.
- Run release-doc and public-comment checks.
- Attempt a default configure/build target for `nexus_example_net_tcp_echo` when dependency fetching allows it.
