# Phase 13F Text File Hygiene Plan

## Goal

Make the repository text checker enforce the final-newline and trailing
whitespace rules declared by `.editorconfig`.

## Steps

1. Add tests for missing final newline and trailing whitespace in text-like
   files.
2. Run the focused text checker tests and confirm the new tests fail.
3. Update `scan_tree` to report missing final newlines and trailing whitespace
   after successful UTF-8 decoding.
4. Run focused tests, unified documentation checks, and whitespace checks.
5. Clean generated Python cache directories.
6. Commit and push to `master`.
