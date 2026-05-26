# Phase 13J Policy Files Text Hygiene Design

## Problem

`.editorconfig` and `.gitattributes` are now validated for required policy
content, but they are not part of the generic text-file scan because they have
no extension and are not listed as explicit text filenames. That means these
policy files can bypass BOM, final-newline, and trailing-whitespace checks.

## Scope

- Treat `.editorconfig` and `.gitattributes` as text candidates in
  `scripts/check_text_encoding.py`.
- Keep all existing policy content checks unchanged.
- Keep binary-extension skipping unchanged.
- Keep support for UTF-8 non-ASCII documentation unchanged.

## Non-Goals

- Do not add new policy keys.
- Do not change `.editorconfig` or `.gitattributes` content.
- Do not introduce a formatter.
- Do not change C++ code or build behavior.

## Contract

- `scan_tree` reports text hygiene failures in `.editorconfig`.
- `scan_tree` reports text hygiene failures in `.gitattributes`.
- `python scripts/check_text_encoding.py --dir .` validates both the content
  and text hygiene of repository policy files.

## Verification

- Add focused tests that fail before the files are treated as text candidates.
- Run docs test discovery, text checks, unified docs checks, and whitespace
  checks before commit.
