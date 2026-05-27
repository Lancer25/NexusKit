# Phase 14B Roadmap Guard Design

## Problem

Phase 14A has been split into focused public API contract checks for callback
threading, zero timeout fields, and move-only ownership semantics. The near-term
roadmap still lists Phase 14A as future work, which can send new sessions back
to already completed contract work.

## Scope

- Treat Phase 14A as completed roadmap work.
- Update `docs/iteration.md` so the near-term roadmap points at a narrower
  next contract phase.
- Extend release documentation checks to reject Phase 14A entries in the
  near-term roadmap.

## Non-Goals

- Do not change public APIs, C++ behavior, build behavior, or module docs.
- Do not enforce the next contract phase in public headers yet.
- Do not rename existing Phase 14A spec or plan files.

## Contract

- `docs/iteration.md` must list the completed Phase 14A public contract checks
  under completed recent work.
- The near-term roadmap must not list Phase 14A as future work.
- If Phase 14A appears as a bullet under `## Near-Term Roadmap`,
  `python scripts/check_release_docs.py --dir .` reports it as stale.

## Verification

- Add a focused failing release-docs test for Phase 14A listed in the near-term
  roadmap.
- Implement the stale roadmap check.
- Update `docs/iteration.md` to list a Phase 14B follow-up instead.
- Run release-doc tests, unified docs checks, text checks, and whitespace
  checks before commit.
