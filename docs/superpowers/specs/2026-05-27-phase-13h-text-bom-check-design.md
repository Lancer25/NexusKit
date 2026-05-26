# Phase 13H Text BOM Check Design

## Problem

NexusKit now enforces UTF-8 text and editor policy, but UTF-8 files with a byte
order mark can still enter the repository. A BOM is unnecessary for UTF-8 and
can confuse simple scripts, generated output, and C/C++ tooling that expects the
first byte to be source text.

## Scope

- Extend `scripts/check_text_encoding.py` so text-like files with a UTF-8 BOM
  fail the existing `scan_tree` check.
- Keep binary-extension skipping unchanged.
- Keep ordinary UTF-8 text, including non-ASCII documentation, valid.
- Keep `.editorconfig` policy checks unchanged.

## Non-Goals

- Do not add an automatic BOM remover.
- Do not enforce ASCII-only text.
- Do not rewrite or normalize the repository.
- Do not change C++ source behavior.

## Contract

- Any text candidate whose bytes start with `EF BB BF` fails with a clear
  message.
- Invalid UTF-8, replacement characters, missing final newlines, and trailing
  whitespace continue to be reported as before.
- `python scripts/check_text_encoding.py --dir .` catches BOM violations.

## Verification

- Add a focused unit test that fails before the BOM check exists.
- Run docs test discovery, text checks, unified docs checks, and whitespace
  checks before commit.
