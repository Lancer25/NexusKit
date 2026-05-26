# Phase 13E Repository Text Policy Plan

## Goal

Make the UTF-8 and whitespace policy explicit for editors and enforce it through
the existing documentation hygiene checks.

## Steps

1. Add tests for `.editorconfig` text policy validation.
2. Run tests and confirm they fail before implementation.
3. Add `.editorconfig` with UTF-8/LF/final-newline/trailing-whitespace rules.
4. Implement `check_text_policy` in `scripts/check_text_encoding.py` and include
   it in the script's command-line check.
5. Update `check_docs.py` tests to account for the new text policy check.
6. Run text checker tests, unified documentation checks, and whitespace checks.
7. Commit and push to `master`.
