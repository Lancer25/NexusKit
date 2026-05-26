# Phase 14A Net Callback Threading Contract Plan

## Goal

Require net async callback typedefs to document callback threading or
synchronous-completion behavior.

## Steps

1. Add a failing contract-checker test for a documented callback typedef that
   omits threading/synchronous invocation semantics.
2. Implement comment-block extraction and the net callback threading contract
   check.
3. Add the new check category to `check_repo`.
4. Run focused contract tests, docs test discovery, public header checks,
   unified docs checks, text checks, and whitespace checks.
5. Clean generated Python cache directories.
6. Commit and push to `master`.
