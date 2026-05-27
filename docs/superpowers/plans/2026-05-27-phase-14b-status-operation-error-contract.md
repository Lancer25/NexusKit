# Phase 14B Status Operation Error Contract Plan

## Goal

Require public non-lifecycle `Status` operations to document expected error
status semantics.

## Steps

1. Add a failing public-header checker test for a public `Status` method whose
   Doxygen block lacks status-code wording.
2. Implement a checker that scans free functions and public class sections for
   non-excluded `Status` declarations.
3. Add the checker to `check_repo` with the `status-error-contracts` category.
4. Fix any real public header comments reported by the new rule.
5. Add a real-repository unit test for the new checker.
6. Run focused public-header tests, docs test discovery, public header checks,
   unified docs checks, text encoding checks, and whitespace checks.
7. Clean generated Python cache directories.
8. Commit and push to `master`.
