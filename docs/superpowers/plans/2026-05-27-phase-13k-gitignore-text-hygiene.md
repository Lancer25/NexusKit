# Phase 13K Git Ignore Text Hygiene Plan

## Goal

Include `.gitignore` in the generic text hygiene scan.

## Steps

1. Add a failing test for `.gitignore` text hygiene.
2. Run focused text checker tests and confirm the new test fails.
3. Add `.gitignore` to explicit text filenames.
4. Run focused tests, docs test discovery, text checks, unified docs checks, and
   whitespace checks.
5. Clean generated Python cache directories.
6. Commit and push to `master`.
