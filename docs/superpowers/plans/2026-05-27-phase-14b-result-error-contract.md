# Phase 14B Result Error Contract Plan

## Goal

Require public `Result<T>` declarations to document expected error status
semantics.

## Steps

1. Add a failing public-header checker test for a public `Result<T>` method
   whose Doxygen block lacks status-code wording.
2. Implement a checker that scans free functions and public class sections for
   `Result<T>` declarations.
3. Add the checker to `check_repo` with the `result-error-contracts` category.
4. Fix any real public header comments reported by the new rule.
5. Add a real-repository unit test for the new checker.
6. Run focused public-header tests, docs test discovery, public header checks,
   unified docs checks, text encoding checks, and whitespace checks.
7. Clean generated Python cache directories.
8. Commit and push to `master`.
