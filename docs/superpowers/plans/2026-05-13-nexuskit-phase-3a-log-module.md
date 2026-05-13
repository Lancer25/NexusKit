# NexusKit Phase 3A Log Module Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use TDD for public logging behavior and verification-before-completion before committing.

**Goal:** Add the first production module after `nexus_core`: `nexus_log`.

## Scope

- Add `NEXUS_ENABLE_LOG`.
- Add a `nexus_log` CMake target exported as `nexus::log`.
- Keep spdlog private to the implementation.
- Provide a minimal file logger API with severity filtering.
- Add unit tests for writing, flushing, and level filtering.
- Document the module.

## Public API

- `nexus::log::Level`
- `nexus::log::LoggerOptions`
- `nexus::log::Logger`
- `nexus::log::create_file_logger`

## Verification

- Configure and build with MSVC 2022.
- Run all CTest tests.
- Install the package to confirm target export still works.
