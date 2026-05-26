# Phase 13F Text File Hygiene Design

## Problem

The repository now declares a UTF-8 and whitespace policy through
`.editorconfig`, but the checker only verifies that the policy exists. CI should
also reject text files that violate the practical parts of that policy.

## Scope

- Extend `scripts/check_text_encoding.py` so `scan_tree` rejects text-like files
  that are missing a final newline.
- Reject text-like files that contain trailing spaces or tabs before a line
  ending or at end of file.
- Keep UTF-8 validation, replacement-character validation, skip directories, and
  binary-extension behavior unchanged.
- Keep CRLF-capable files valid when they otherwise satisfy the hygiene rules.

## Non-Goals

- Do not reformat the repository in this phase.
- Do not add a formatter or automatic fixer.
- Do not enforce ASCII-only text.
- Do not change public APIs or C++ behavior.

## Contract

- Empty text files are allowed.
- Non-empty text files must end with `\n`.
- Any line ending in spaces or tabs fails with a line-specific message.
- `python scripts/check_text_encoding.py --dir .` reports text hygiene failures
  alongside UTF-8 and `.editorconfig` policy failures.

## Verification

- Add unit tests that fail before implementation.
- Run focused text checker tests.
- Run unified documentation checks and whitespace checks before commit.
