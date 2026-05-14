# NexusKit Phase 4H: WebSocket Client

## Goal

Add a small synchronous WebSocket client to `nexus_net` as the first WebSocket building block.

## Scope

- Add `nexus/net/websocket_client.h` with a PIMPL-based public API.
- Support `ws://` connection, text send, text receive, close, and open-state checks.
- Use websocketpp privately with standalone Asio and without Boost headers.
- Add loopback echo tests using a local websocketpp server.
- Route WebSocket lifecycle events through NexusKit diagnostic logging.

## Verification

- Add failing WebSocket tests first.
- Build and run targeted WebSocket tests.
- Run full configure, build, test, install, and export checks before committing.
