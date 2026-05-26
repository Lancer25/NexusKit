# Phase 14A Zero Timeout Contract Plan

## Goal

Require net timeout fields defaulted to zero to document that zero means no
explicit timeout.

## Steps

1. Add a failing contract-checker test for a zero-default timeout field that
   omits the zero-timeout semantics.
2. Implement a net zero-timeout field contract checker.
3. Add the new check category to `check_repo`.
4. Add a real-repository public header contract unit test.
5. Run focused contract tests, docs test discovery, public header checks,
   unified docs checks, text checks, and whitespace checks.
6. Clean generated Python cache directories.
7. Commit and push to `master`.
