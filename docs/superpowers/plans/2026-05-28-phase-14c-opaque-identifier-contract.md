# Phase 14C Opaque Identifier Contract Plan

## Goal

Require public opaque identifier/path fields to document persistence or open-use
lifecycle semantics.

## Steps

1. Add a failing public-header checker test for an opaque `std::string id`
   field whose comment lacks lifecycle wording.
2. Implement a checker that scans public headers for opaque `std::string`
   identifier/path fields.
3. Add the checker to `check_repo` with the `opaque-identifiers` category.
4. Add a real-repository unit test for the new checker.
5. Run focused public-header tests, docs test discovery, public header checks,
   unified docs checks, text encoding checks, and whitespace checks.
6. Clean generated Python cache directories.
7. Commit and push to `master`.
