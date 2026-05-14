# NexusKit Phase 4F UDP Socket Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use TDD for public UDP behavior and verification-before-completion before committing.

**Goal:** Add a minimal synchronous UDP socket to `nexus_net`.

## Scope

- Reuse standalone Asio as a private `nexus_net` backend.
- Add `UdpEndpoint`.
- Add `UdpDatagram`.
- Add move-only `UdpSocket`.
- Support bind, open-state checks, send-to, receive-from, and close.
- Add loopback echo tests using a local UDP server.
- Update module documentation.

## Public API

- `nexus::net::UdpEndpoint`
- `nexus::net::UdpDatagram`
- `nexus::net::UdpSocket`

## Verification

- Build and run `nexus_net_tests` during the TDD cycle.
- Run the full MSVC Debug build.
- Run all CTest tests.
- Install the package to confirm target export still works.
