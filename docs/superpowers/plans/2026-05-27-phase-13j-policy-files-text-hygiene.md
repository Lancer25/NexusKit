# Phase 13J Policy Files Text Hygiene Plan

## Goal

Include repository policy files in the generic text hygiene scan.

## Steps

1. Add failing tests for `.editorconfig` and `.gitattributes` text hygiene.
2. Run focused text checker tests and confirm the new tests fail.
3. Add `.editorconfig` and `.gitattributes` to explicit text filenames.
4. Run focused tests, docs test discovery, text checks, unified docs checks, and
   whitespace checks.
5. Clean generated Python cache directories.
6. Commit and push to `master`.
