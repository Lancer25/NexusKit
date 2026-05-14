# NexusKit Phase 4E TCP Client Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use TDD for public TCP behavior and verification-before-completion before committing.

**Goal:** Add a minimal synchronous TCP client to `nexus_net`.

## Scope

- Add standalone Asio as a private `nexus_net` backend.
- Add an `asio::asio` interface target in the dependency recipe.
- Add `TcpEndpoint`.
- Add move-only `TcpClient`.
- Support connect, open-state checks, write-all, read-some, and close.
- Add loopback echo tests using a local TCP server.
- Update module documentation.

## Public API

- `nexus::net::TcpEndpoint`
- `nexus::net::TcpClient`

## Verification

- Build and run `nexus_net_tests` during the TDD cycle.
- Run the full MSVC Debug build.
- Run all CTest tests.
- Install the package to confirm target export still works.
