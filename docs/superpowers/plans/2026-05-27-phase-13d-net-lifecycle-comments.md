# Phase 13D Net Lifecycle Comments Plan

## Goal

Close lifecycle documentation gaps for net public handles without changing
behavior.

## Steps

1. Extend `tests/scripts/test_public_header_contracts.py` to track net public
   handle headers.
2. Run the public header contract test and confirm it fails on missing net
   lifecycle comments.
3. Add focused `///` comments to the reported declarations and clean touched
   English punctuation.
4. Run public header contract tests, the unified documentation check, and
   whitespace checks.
5. Commit and push to `master`.
