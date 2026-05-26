# Phase 13E Public Header Contract Categories Plan

## Goal

Make public header contract failures easier to triage by adding stable category
prefixes to checker messages.

## Steps

1. Update public header contract checker tests to expect category-prefixed
   messages.
2. Run the tests and confirm they fail before implementation.
3. Add message categorization in `scripts/check_public_header_contracts.py`.
4. Run checker tests, the unified documentation check, and whitespace checks.
5. Commit and push to `master`.
