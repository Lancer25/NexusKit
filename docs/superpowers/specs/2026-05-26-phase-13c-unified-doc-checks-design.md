# Phase 13C Unified Documentation Checks Design

## Problem

Documentation and API hygiene checks now live behind several separate commands.
That is easy to forget locally and makes CI harder to read as more lightweight
quality gates are added.

## Scope

- Add `scripts/check_docs.py` as the single repository documentation quality
  gate.
- Reuse existing checker functions instead of shelling out to Python scripts.
- Keep checker unit tests separate from the production quality gate.
- Update CI to run the checker tests and then the unified documentation check.
- Document the local command in maintenance guidance.

## Non-Goals

- Do not change the existing checker semantics.
- Do not broaden public API documentation enforcement in this phase.
- Do not run CMake builds from the documentation check.

## Contract

- `python scripts/check_docs.py --dir .` runs:
  - UTF-8 text encoding checks.
  - Release-facing documentation checks.
  - Public header Doxygen checks.
- Failures are grouped by checker name and return a non-zero exit code.
- A clean repository prints one success line.

## Verification

- Add tests that fail before `scripts/check_docs.py` exists.
- Implement the unified checker.
- Run all script checker tests and the unified repository check.
