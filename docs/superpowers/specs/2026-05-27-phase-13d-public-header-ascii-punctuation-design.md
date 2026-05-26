# Phase 13D Public Header ASCII Punctuation Design

## Problem

Public headers are often read in terminals, compiler diagnostics, generated
documentation, and package consumers' editors. Non-ASCII dash punctuation in
English comments has repeatedly displayed poorly in Windows console output,
making valid UTF-8 files look corrupted.

## Scope

- Add a focused public-header contract test for confusing dash punctuation in
  `include/nexus/**/*.h`.
- Replace reported dash punctuation with ASCII wording.
- Keep UTF-8 as the project encoding; this is not an ASCII-only repository
  rule.
- Keep API shape, ABI, and runtime behavior unchanged.

## Non-Goals

- Do not rewrite Chinese documentation.
- Do not reject all non-ASCII characters everywhere.
- Do not change implementation files in this phase.

## Contract

- Public headers must not contain em dash or en dash punctuation.
- The contract test reports header and line number for any occurrence.

## Verification

- Add the contract test and confirm it fails on existing public header dash
  punctuation.
- Replace reported punctuation and confirm the contract test passes.
- Run the unified documentation check and whitespace check before commit.
