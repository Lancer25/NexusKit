# Phase 13C Unified Documentation Checks Plan

## Goal

Make local and CI documentation hygiene checks easier to run by adding one
canonical command.

## Steps

1. Add tests for `scripts/check_docs.py`:
   - It reports success when all underlying checks are clean.
   - It aggregates failures from multiple underlying checks.
2. Run the new tests and confirm they fail before implementation.
3. Implement `scripts/check_docs.py` by reusing existing checker functions.
4. Update GitHub Actions documentation checks to run the unified command.
5. Add the local command to maintenance guidance.
6. Run script tests, the unified checker, and whitespace checks.
7. Commit and push to `master`.
