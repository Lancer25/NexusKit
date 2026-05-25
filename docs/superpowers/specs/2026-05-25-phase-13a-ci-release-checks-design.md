# Phase 13A CI And Release Checks Design

## Goal

Add lightweight automated checks that protect the 0.1.0 release documentation, default build surface, and installed package export before larger feature work continues.

## Scope

Phase 13A adds a small release-doc validation script, unit tests for that script, and a GitHub Actions workflow for default build/test/install/package-consumer smoke checks. The workflow stays on the default dependency surface so CI remains fast and does not require FFmpeg, hidapi, PortAudio, libuvc, OpenSSL, or platform hardware.

## Requirements

- Release-facing docs must reject stale `0.1.0 - Unreleased` changelog headings.
- PowerShell fenced command blocks must reject invalid continuation lines.
- README and build guide full-build examples must include all optional module enable flags, including `NEXUS_ENABLE_SCREEN=ON`.
- Docs must keep `nexus_screen` default wording aligned with `CMakeLists.txt` (`OFF`).
- HID-level hotplug must remain explicitly not planned while USB hotplug remains allowed.
- CI must run the release-doc checker, the existing public-comment checker, default CMake configure/build/test, install smoke, and a downstream `find_package(NexusKit CONFIG REQUIRED)` consumer.

## Non-Goals

- No C++ API changes.
- No heavyweight optional-module CI in this phase.
- No release asset regeneration.
- No CPack package generation.

## Verification

- Run the new checker unit tests and verify they fail before the checker exists.
- Run the checker on the real repository.
- Run existing public-comment checks.
- Run default configure/build/test/install locally where feasible.
- Run a consumer smoke configure/build against the local install prefix.
