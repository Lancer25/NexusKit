# NexusKit Phase 4A Net HTTP Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use TDD for public HTTP behavior and verification-before-completion before committing.

**Goal:** Start `nexus_net` with a minimal HTTP client wrapper.

## Scope

- Enable `NEXUS_ENABLE_NET` by default.
- Add a `nexus_net` CMake target exported as `nexus::net`.
- Keep cpp-httplib private to implementation details.
- Provide a minimal `HttpClient` API for `http://` GET requests.
- Return completed HTTP responses even for non-2xx status codes.
- Add loopback tests using a local HTTP server.
- Document the module.

## Public API

- `nexus::net::HttpHeader`
- `nexus::net::HttpResponse`
- `nexus::net::HttpClientOptions`
- `nexus::net::HttpClient`

## Verification

- Configure and build with MSVC 2022.
- Run all CTest tests.
- Install the package to confirm target export still works.
