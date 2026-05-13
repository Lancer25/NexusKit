# NexusKit Phase 3B Common JSON Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use TDD for public JSON behavior and verification-before-completion before committing.

**Goal:** Add `nexus_common` as the shared utility module and start it with a stable JSON wrapper.

## Scope

- Enable `NEXUS_ENABLE_COMMON` by default.
- Add a `nexus_common` CMake target exported as `nexus::common`.
- Keep nlohmann_json private to the implementation.
- Provide a minimal JSON value API based on `nexus::Status` and `nexus::Result<T>`.
- Add unit tests for parsing, invalid input, object construction, and type/precondition errors.
- Document the module.

## Public API

- `nexus::common::JsonValue`
- `nexus::common::parse_json`

## Verification

- Configure and build with MSVC 2022.
- Run all CTest tests.
- Install the package to confirm target export still works.
