# Phase 13C Text Encoding Checks Design

## Problem

NexusKit documentation and source files should be stored as UTF-8. Manual
inspection can confuse console display issues with actual file encoding, and
new non-UTF-8 text files would not currently be caught by CI.

## Scope

- Add a repository text encoding checker that verifies tracked text-like files
  decode as UTF-8.
- Keep binary files and generated/build directories out of the scan.
- Add unit tests for valid UTF-8 text, invalid byte sequences, and ignored
  binary file extensions.
- Run the checker in the existing documentation CI job.

## Non-Goals

- Do not rewrite files that already decode as UTF-8.
- Do not require ASCII-only content; Chinese documentation is allowed when
  stored as UTF-8.
- Do not add heavyweight encoding detection dependencies.

## Contract

- Text-like files with common source, build, script, config, and documentation
  extensions must decode using UTF-8.
- Files containing the Unicode replacement character are reported as suspicious,
  because that usually means text was decoded with loss before being committed.
- The checker prints actionable relative paths and exits non-zero on failure.

## Verification

- First add tests that fail because the checker module does not exist.
- Implement the checker and confirm the tests pass.
- Run the checker against the repository and keep the existing documentation
  checks green.
