# Phase 14C Roadmap Guard Design

## Problem

Phase 14B now protects public `Result<T>` and non-lifecycle `Status` error
contracts. The near-term roadmap still lists Phase 14B as future work, which can
make new sessions repeat completed public API contract cleanup.

## Scope

- Treat Phase 14B as completed roadmap work.
- Update `docs/iteration.md` so the near-term roadmap points at a narrower next
  phase.
- Extend release documentation checks to reject Phase 14B entries in the
  near-term roadmap.

## Non-Goals

- Do not change public APIs, C++ behavior, build behavior, or module docs.
- Do not enforce the next phase yet.
- Do not rename existing Phase 14B spec or plan files.

## Contract

- `docs/iteration.md` must list completed Phase 14B error contract checks under
  completed recent work.
- The near-term roadmap must not list Phase 14B as future work.
- If Phase 14B appears as a bullet under `## Near-Term Roadmap`,
  `python scripts/check_release_docs.py --dir .` reports it as stale.

## Verification

- Add a focused failing release-docs test for Phase 14B listed in the near-term
  roadmap.
- Implement the stale roadmap check.
- Update `docs/iteration.md` to list a Phase 14C follow-up instead.
- Run release-doc tests, unified docs checks, text checks, and whitespace checks
  before commit.
