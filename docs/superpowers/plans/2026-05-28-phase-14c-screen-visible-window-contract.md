# Phase 14C Screen Visible Window Contract Plan

## Goal

Protect the public `nexus_screen` visible-window capture semantics in header
contract checks.

## Steps

1. Add a failing public-header checker test for `ScreenWindow` and
   `capture_window` comments that omit visible/occlusion wording.
2. Implement a checker for `include/nexus/screen/screen.h` that verifies the
   required visible-window terms.
3. Add the checker to `check_repo` with the `screen-visible-window` category.
4. Add a real-repository unit test for the new checker.
5. Run focused public-header tests, docs test discovery, public header checks,
   unified docs checks, text encoding checks, and whitespace checks.
6. Clean generated Python cache directories.
7. Commit and push to `master`.
