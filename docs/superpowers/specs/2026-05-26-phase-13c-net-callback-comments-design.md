# Phase 13C Net Callback Comments Design

## Goal

Document the callback contracts for public `nexus_net` asynchronous APIs so users can understand threading, invocation, error, and lifetime semantics from headers alone.

## Scope

This phase only updates public header comments and a lightweight contract test. It covers handler typedefs in TCP, TCP listener, UDP, HTTP, WebSocket client, and WebSocket server headers.

## Requirements

- Every public `*Handler` typedef in `include/nexus/net/*.h` must have an adjacent Doxygen `///` comment.
- Comments must describe callback thread context, invocation cardinality, and success/error result semantics.
- Comments must document data lifetime expectations where buffers or message payloads are involved.
- No ABI, behavior, implementation, or dependency changes.

## Non-Goals

- No new net API.
- No callback behavior changes.
- No CMake changes.
- No broad public-header annotation audit outside callback typedefs.

## Verification

- Add a failing Python contract test before editing headers.
- Run the contract test after adding comments.
- Run existing public-comment and release-doc checks.
