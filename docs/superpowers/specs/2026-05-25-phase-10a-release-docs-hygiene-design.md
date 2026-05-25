# Phase 10A Release Documentation Hygiene Design

## Goal

Make the post-0.1.0 documentation set consistent, readable, and useful for the next iteration before adding more API surface.

## Scope

Phase 10A is a documentation-only release-maintenance phase. It fixes visible encoding artifacts, aligns release wording with the published `0.1.0` tag, corrects build-option descriptions against `CMakeLists.txt`, and records the next iteration direction.

## Requirements

- `CHANGELOG.md` must describe `0.1.0` as released, not unreleased.
- User-facing docs must not contain visible encoding artifacts.
- Build docs must describe module defaults consistently with top-level CMake options.
- PowerShell examples must use PowerShell-valid continuation syntax.
- The "all modules" build command must actually enable all optional modules.
- Roadmap docs must preserve the decision that HID-level hotplug is not planned.
- The next phases should be explicit enough for a new session to continue without rediscovering the same context.

## Non-Goals

- No C++ API changes.
- No test behavior changes.
- No dependency or CMake option changes.
- No HID-level hotplug support.

## Verification

Use text checks instead of unit tests because this phase changes documentation only:

- Search for remaining mojibake markers.
- Search for stale unreleased wording on the `0.1.0` heading.
- Compare documented module defaults with `CMakeLists.txt` option defaults.
- Confirm the full-build command includes `NEXUS_ENABLE_SCREEN=ON`.
- Confirm `git status` contains only the intended documentation files before commit.
