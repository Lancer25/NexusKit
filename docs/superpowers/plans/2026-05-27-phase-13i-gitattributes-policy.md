# Phase 13I Git Attributes Policy Plan

## Goal

Require the Git line-ending policy that backs the repository text rules.

## Steps

1. Add a failing test for missing `.gitattributes` text policy.
2. Update the passing policy fixture to include the required `.gitattributes`
   lines.
3. Implement `.gitattributes` validation inside `check_text_policy`.
4. Run focused tests, docs test discovery, text checks, unified docs checks, and
   whitespace checks.
5. Clean generated Python cache directories.
6. Commit and push to `master`.
