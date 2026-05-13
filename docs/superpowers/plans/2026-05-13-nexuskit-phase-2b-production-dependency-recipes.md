# NexusKit Phase 2B Production Dependency Recipes Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans or superpowers:subagent-driven-development to implement this plan task-by-task.

**Goal:** Add maintainable CMake dependency recipes for the first production dependency set without forcing every dependency to build during the default configure.

**Scope:** This phase adds recipes and documentation only. It does not introduce production modules that link to the recipes yet.

## Dependency Set

| Dependency | Version | Source revision | Purpose |
| --- | --- | --- | --- |
| spdlog | v1.14.1 | `27cb4c76708608465c413f6d0e6b8d99a4d84302` | Logging backend |
| nlohmann_json | v3.11.3 | `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03` | JSON utilities |
| pugixml | v1.14 | `db78afc2b7d8f043b4bc6b185635d949ea2ed2a8` | XML utilities |
| asio | 1.30.2 | `42a93679dc4c8c5caf3d3082542f1bfa2438271d` | TCP/UDP async I/O |
| cpp-httplib | v0.15.3 | `5c00bbf36ba8ff47b4fb97712fc38cb2884e5b98` | HTTP client/server |
| websocketpp | 0.8.2 | `0e4241727c199e208ceb41134e840ba9968cf181` | WebSocket transport |
| hidapi | 0.14.0 | `73d292a8f4d18f7fb532b9829b9a26ca8d864419` | HID device access |
| PortAudio | v19.7.0 | `147dd722548358763a8b649b3e4b41dfffbcfbb6` | Audio I/O |
| OpenSSL | 3.3.1 | `243b18a4c9e2865caf7901ec4506e899cfc34d7c` | Crypto and TLS |
| FFmpeg | n7.0.1 | `47f70eda3e2ff003a787e512afd07b0c266f7a70` | Media processing |

## Tasks

- [ ] Add lightweight FetchContent recipes for CMake-native or header-only dependencies.
- [ ] Add source-build recipes for OpenSSL and FFmpeg with explicit prerequisite checks and clear fatal messages.
- [ ] Wire optional source-build options in the top-level CMake file so heavy builds are opt-in.
- [ ] Update dependency documentation and build guide.
- [ ] Verify default configure/build/tests still use only required dependencies.
