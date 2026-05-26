# Phase 13N Stale Roadmap Guard Design

## Problem

`docs/iteration.md` now records Phase 13C-13D as completed work, but the
Post-0.1.0 roadmap still lists Phase 13C as future work. That contradiction can
make future sessions repeat completed documentation and public-contract audit
work instead of choosing the next useful stage.

## Scope

- Add a release-docs invariant that rejects a future roadmap item named
  `Phase 13C`.
- Update the roadmap entry to point at the next narrower public API contract
  follow-up instead of the completed Phase 13C audit.
- Keep completed-work history intact.

## Non-Goals

- Do not rewrite the full roadmap.
- Do not claim Phase 13A or Phase 13B are complete.
- Do not change CI, source code, public APIs, or build behavior.

## Contract

- `docs/iteration.md` may mention Phase 13C in completed-work summaries.
- `docs/iteration.md` must not list `Phase 13C:` as an active future roadmap
  bullet.
- `python scripts/check_release_docs.py --dir .` fails if a future roadmap entry
  reintroduces `Phase 13C:`.

## Verification

- Add a release-docs unit test that fails before the invariant exists.
- Update the release docs checker and iteration roadmap.
- Run docs test discovery, release docs checks, unified docs checks, text checks,
  and whitespace checks before commit.
