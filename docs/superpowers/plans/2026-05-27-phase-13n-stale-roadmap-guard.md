# Phase 13N Stale Roadmap Guard Plan

## Goal

Prevent completed Phase 13C work from remaining listed as future roadmap work.

## Steps

1. Add a failing release-docs test for a future `Phase 13C:` roadmap entry.
2. Implement a release-docs invariant that catches the stale future entry while
   allowing completed-work references.
3. Update `docs/iteration.md` with a non-stale next API contract follow-up.
4. Run focused release docs tests, docs test discovery, release docs checks,
   unified docs checks, text checks, and whitespace checks.
5. Clean generated Python cache directories.
6. Commit and push to `master`.
