# Phase 13D Public Header ASCII Punctuation Plan

## Goal

Prevent public headers from looking corrupted in Windows console output by
keeping English dash punctuation ASCII-only.

## Steps

1. Add a public header contract test that scans `include/nexus/**/*.h` for em
   dash and en dash characters.
2. Run the test and confirm it fails on the current public header occurrences.
3. Replace reported punctuation with ASCII wording.
4. Run public header contract tests, the unified documentation check, and
   whitespace checks.
5. Commit and push to `master`.
