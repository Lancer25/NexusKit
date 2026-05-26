# Phase 13M Iteration Branch Name Design

## Problem

`docs/iteration.md` still tells future sessions to continue on and push to
`main`, while the repository's active branch and remote tracking branch are
`master`. That mismatch can send future work down the wrong branch path.

## Scope

- Update `docs/iteration.md` so the default workflow and project rules name
  `master`.
- Add a release-docs invariant that rejects `main` as the default direct-work
  branch in `docs/iteration.md`.
- Keep GitHub Actions triggers unchanged because CI can continue to accept both
  branch names.

## Non-Goals

- Do not rename the repository branch.
- Do not change CI trigger branches.
- Do not change source code, build behavior, or public APIs.

## Contract

- `docs/iteration.md` tells future sessions to commit and push to `master`.
- `docs/iteration.md` tells future sessions to continue directly on `master`
  unless the user requests a branch.
- `python scripts/check_release_docs.py --dir .` fails if the iteration guide
  reintroduces direct-work instructions for `main`.

## Verification

- Add a release-docs unit test that fails before the invariant exists.
- Update the release docs checker and iteration guide.
- Run docs test discovery, release docs checks, unified docs checks, text checks,
  and whitespace checks before commit.
