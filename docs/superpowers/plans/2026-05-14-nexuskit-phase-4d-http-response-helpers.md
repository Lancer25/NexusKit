# NexusKit Phase 4D HTTP Response Helpers Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use TDD for public HTTP behavior and verification-before-completion before committing.

**Goal:** Improve `nexus_net` HTTP response ergonomics.

## Scope

- Add `HttpResponse::ok` for 2xx success checks.
- Add `HttpResponse::header` for case-insensitive response header lookup.
- Return `StatusCode::kNotFound` for missing headers.
- Add loopback tests for 2xx/non-2xx status helpers and header lookup.
- Update module documentation.

## Public API

- `HttpResponse::ok`
- `HttpResponse::header`

## Verification

- Build `nexus_net_tests` and run the test executable during the TDD cycle.
- Run the full MSVC Debug build.
- Run all CTest tests.
- Install the package to confirm target export still works.
