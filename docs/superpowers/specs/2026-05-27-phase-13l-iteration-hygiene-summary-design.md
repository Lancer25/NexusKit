# Phase 13L Iteration Hygiene Summary Design

## Problem

`docs/iteration.md` is the handoff document for future NexusKit sessions, but it
does not yet summarize the recent Phase 13 documentation, CI, and repository
text-hygiene baseline. New sessions can therefore rediscover completed work or
choose stale roadmap items.

## Scope

- Add a concise Phase 13 summary to `docs/iteration.md`.
- Add a release-docs invariant that requires the iteration guide to mention the
  Phase 13 repository text and documentation hygiene baseline.
- Keep module behavior, build behavior, and public APIs unchanged.

## Non-Goals

- Do not rewrite the full roadmap.
- Do not list every individual hygiene commit in detail.
- Do not change C++ code or CI behavior.
- Do not add new feature roadmap items.

## Contract

- `docs/iteration.md` records that Phase 13C-13D covered public header
  documentation and contract checks.
- `docs/iteration.md` records that Phase 13E-13K established repository text
  and documentation hygiene checks.
- `python scripts/check_release_docs.py --dir .` fails if the Phase 13E-13K
  repository hygiene summary is removed.

## Verification

- Add a release-docs unit test that fails before the invariant exists.
- Update the release docs checker and iteration guide.
- Run docs test discovery, release docs checks, unified docs checks, text checks,
  and whitespace checks before commit.
