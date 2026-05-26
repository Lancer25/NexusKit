# Phase 13I Git Attributes Policy Design

## Problem

The repository text policy now validates `.editorconfig`, but Git line-ending
normalization is controlled by `.gitattributes`. If `.gitattributes` is removed
or changed, contributors can still get inconsistent line endings even when their
editor settings are correct.

## Scope

- Extend `scripts/check_text_encoding.py` text policy validation to require the
  repository `.gitattributes` file.
- Require the project-wide text rule and the CRLF overrides for Windows script
  files.
- Keep UTF-8, BOM, final-newline, trailing-whitespace, and `.editorconfig`
  checks unchanged.
- Do not change the current `.gitattributes` contents unless validation exposes
  a mismatch.

## Non-Goals

- Do not build a full `.gitattributes` parser.
- Do not enforce attributes for every file type.
- Do not change Git history or renormalize files.
- Do not change C++ behavior.

## Contract

- `.gitattributes` exists at the repository root.
- It contains:
  - `* text=auto eol=lf`
  - `*.bat text eol=crlf`
  - `*.cmd text eol=crlf`
  - `*.ps1 text eol=crlf`
- `python scripts/check_text_encoding.py --dir .` reports missing or incorrect
  Git text normalization rules.

## Verification

- Add a focused unit test that fails before `.gitattributes` policy validation
  exists.
- Run docs test discovery, text checks, unified docs checks, and whitespace
  checks before commit.
