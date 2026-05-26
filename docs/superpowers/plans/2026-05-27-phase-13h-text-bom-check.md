# Phase 13H Text BOM Check Plan

## Goal

Reject UTF-8 BOMs in repository text files through the existing text checker.

## Steps

1. Add a failing test for a text file that starts with a UTF-8 BOM.
2. Run the focused text checker tests and confirm the new test fails.
3. Update `scan_tree` to report a UTF-8 BOM before decoding text.
4. Run docs test discovery, text checks, unified docs checks, and whitespace
   checks.
5. Clean generated Python cache directories.
6. Commit and push to `master`.
