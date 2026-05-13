# NexusKit Phase 4C HTTP REST And Query Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use TDD for public HTTP behavior and verification-before-completion before committing.

**Goal:** Extend `nexus_net` HTTP client with common REST methods and query path construction.

## Scope

- Add `HttpClient::put` with body, content type, and optional request headers.
- Add `HttpClient::del` with optional request headers.
- Add `HttpQueryParameter`.
- Add `build_query_path` with percent encoding.
- Keep cpp-httplib private to implementation and tests.
- Add loopback tests for PUT, DELETE, query encoding, and query execution.
- Update module documentation.

## Public API

- `nexus::net::HttpQueryParameter`
- `HttpClient::put`
- `HttpClient::del`
- `nexus::net::build_query_path`

## Verification

- Build `nexus_net_tests` and run the test executable during the TDD cycle.
- Run the full MSVC Debug build.
- Run all CTest tests.
- Install the package to confirm target export still works.
