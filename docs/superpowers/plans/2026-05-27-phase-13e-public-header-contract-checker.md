# Phase 13E Public Header Contract Checker Plan

## Goal

Promote public header contracts from unittest-only checks into the unified
documentation checker path.

## Steps

1. Add tests for a reusable `scripts/check_public_header_contracts.py` checker
   and for `check_docs.run_checks()` including a `public-header-contracts`
   result.
2. Run the tests and confirm they fail before implementation.
3. Implement the checker by moving contract logic out of
   `tests/scripts/test_public_header_contracts.py`.
4. Update `scripts/check_docs.py` to call the checker.
5. Run checker tests, the unified documentation check, and whitespace checks.
6. Commit and push to `master`.
