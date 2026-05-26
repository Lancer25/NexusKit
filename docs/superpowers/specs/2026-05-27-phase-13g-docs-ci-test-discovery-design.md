# Phase 13G Docs CI Test Discovery Design

## Problem

The documentation CI job hardcodes every `tests.scripts` test module in the
workflow. That works today, but it creates a maintenance trap: new documentation
checker tests can be added locally without being executed by CI unless the
workflow list is updated by hand.

## Scope

- Change the docs workflow test command to use unittest discovery for
  `tests/scripts/test_*.py`.
- Add a repository test that verifies the docs workflow keeps using discovery.
- Keep the documentation checker behavior unchanged.
- Keep release build jobs and non-doc CI behavior unchanged.

## Non-Goals

- Do not introduce a YAML parser dependency.
- Do not rewrite the full GitHub Actions workflow.
- Do not add new CI jobs.
- Do not change C++ build or runtime behavior.

## Contract

- The docs workflow runs all Python files matching `tests/scripts/test_*.py`.
- Adding a new docs checker test file under `tests/scripts` should not require
  editing `.github/workflows/ci.yml`.
- The workflow self-check fails if the docs job returns to a hardcoded module
  list.

## Verification

- Add the workflow self-check first and confirm it fails against the current
  hardcoded workflow command.
- Update `.github/workflows/ci.yml`.
- Run unittest discovery, documentation checks, text checks, and whitespace
  checks before commit.
