# Phase 14A Move-Only Ownership Contract Plan

## Goal

Require move-only public RAII classes to document copy deletion and move
ownership transfer at the class level.

## Steps

1. Add a failing contract-checker test for a move-only class missing ownership
   wording.
2. Implement class-level Doxygen extraction and the move-only ownership checker.
3. Add the new check category to `check_repo`.
4. Fix any real public headers reported by the new check.
5. Add a real-repository public header contract unit test.
6. Run focused contract tests, docs test discovery, public header checks,
   unified docs checks, text checks, and whitespace checks.
7. Clean generated Python cache directories.
8. Commit and push to `master`.
