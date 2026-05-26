# Phase 13C Text Encoding Checks Plan

## Goal

Turn the project-wide UTF-8 source/documentation convention into an automated
repository check.

## Steps

1. Add focused tests for a text encoding checker:
   - UTF-8 files pass.
   - Invalid byte sequences fail with a path and reason.
   - Binary extensions are ignored.
2. Run the new tests and confirm they fail before implementation.
3. Implement `scripts/check_text_encoding.py` with reusable scan helpers and a
   command-line entry point.
4. Add the checker to `.github/workflows/ci.yml` documentation checks.
5. Run the new tests, the repository encoding check, the existing documentation
   checks, and whitespace checks.
6. Commit and push to `master`.
