# Phase 14C Roadmap Guard Plan

## Goal

Prevent completed Phase 14B public error contract work from remaining listed as
future roadmap work.

## Steps

1. Add a failing release-docs test that rejects `- Phase 14B:` under
   `## Near-Term Roadmap`.
2. Extend the release-doc stale roadmap checker to report Phase 14B bullets.
3. Update `docs/iteration.md` so completed work includes Phase 14B and the
   near-term roadmap points at Phase 14C.
4. Run focused release-doc tests and full script-level documentation tests.
5. Run unified docs checks, text encoding checks, and whitespace checks.
6. Clean generated Python cache directories.
7. Commit and push to `master`.
