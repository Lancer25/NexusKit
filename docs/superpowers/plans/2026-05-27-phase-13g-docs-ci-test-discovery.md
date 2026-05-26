# Phase 13G Docs CI Test Discovery Plan

## Goal

Make the documentation CI job automatically run future `tests/scripts` test
modules.

## Steps

1. Add a test that asserts the docs workflow uses unittest discovery against
   `tests/scripts`.
2. Run the new test and confirm it fails with the current hardcoded workflow.
3. Replace the hardcoded workflow module list with unittest discovery.
4. Run docs test discovery and the unified documentation checks.
5. Clean generated Python cache directories.
6. Commit and push to `master`.
