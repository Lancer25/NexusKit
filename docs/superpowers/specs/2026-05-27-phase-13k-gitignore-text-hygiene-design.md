# Phase 13K Git Ignore Text Hygiene Design

## Problem

`.gitignore` is a repository-maintenance text file with no extension, so it is
currently skipped by `scan_tree`. That leaves it outside the same UTF-8, BOM,
final-newline, and trailing-whitespace checks that now cover source files,
documentation, `.editorconfig`, and `.gitattributes`.

## Scope

- Treat `.gitignore` as an explicit text candidate in
  `scripts/check_text_encoding.py`.
- Keep all existing policy checks unchanged.
- Keep binary-extension skipping unchanged.
- Keep UTF-8 non-ASCII comments valid.

## Non-Goals

- Do not validate ignore pattern semantics.
- Do not change `.gitignore` contents unless the new check exposes a hygiene
  issue.
- Do not add automatic formatting.
- Do not change C++ code or build behavior.

## Contract

- `scan_tree` reports text hygiene failures in `.gitignore`.
- `python scripts/check_text_encoding.py --dir .` validates `.gitignore` through
  the same text-file rules as other text candidates.

## Verification

- Add a focused test that fails before `.gitignore` is treated as a text
  candidate.
- Run docs test discovery, text checks, unified docs checks, and whitespace
  checks before commit.
