# NexusKit Phase 4B HTTP POST And Headers Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use TDD for public HTTP behavior and verification-before-completion before committing.

**Goal:** Extend `nexus_net` HTTP client with request headers and POST.

## Scope

- Allow `HttpClient::get` to send optional request headers.
- Add `HttpClient::post` with body, content type, and optional request headers.
- Keep cpp-httplib private to implementation and tests.
- Add loopback tests for request headers and POST body/content type handling.
- Update module documentation.

## Public API

- `HttpClient::get(std::string_view, const std::vector<HttpHeader>&)`
- `HttpClient::post(std::string_view, std::string_view, std::string_view, const std::vector<HttpHeader>&)`

## Verification

- Build `nexus_net_tests` and run the test executable during the TDD cycle.
- Run the full MSVC Debug build.
- Run all CTest tests.
- Install the package to confirm target export still works.
