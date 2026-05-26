# Phase 13E Repository Text Policy Design

## Problem

The repository already checks that text-like files decode as UTF-8, and Git
attributes normalize text files. Editors still need a lightweight, discoverable
policy for charset, line endings, final newlines, and trailing whitespace.

## Scope

- Add `.editorconfig` with project-wide UTF-8 and whitespace defaults.
- Extend `scripts/check_text_encoding.py` with a repository text policy check
  that validates the required `.editorconfig` keys.
- Include that policy check in the existing text encoding checker and unified
  documentation gate.
- Keep source code behavior unchanged.

## Non-Goals

- Do not reformat the repository.
- Do not require ASCII-only text files.
- Do not add editor-specific project files.

## Contract

- `.editorconfig` exists at the repository root.
- The top-level setting contains:
  - `root = true`
- The `[*]` section contains:
  - `charset = utf-8`
  - `end_of_line = lf`
  - `insert_final_newline = true`
  - `trim_trailing_whitespace = true`
- The `[*.{bat,cmd,ps1}]` section contains:
  - `end_of_line = crlf`
- `python scripts/check_text_encoding.py --dir .` reports missing or incorrect
  policy settings.

## Verification

- Add tests that fail before the policy checker exists.
- Add `.editorconfig` and implement the checker.
- Run the unified documentation check and whitespace check before commit.
