# Phase 13M Iteration Branch Name Plan

## Goal

Align future-session branch guidance with the repository's actual `master`
branch.

## Steps

1. Add a failing release-docs test for `docs/iteration.md` default branch
   guidance that still names `main`.
2. Implement a release-docs invariant for direct-work branch wording.
3. Update `docs/iteration.md` to use `master`.
4. Run focused release docs tests, docs test discovery, release docs checks,
   unified docs checks, text checks, and whitespace checks.
5. Clean generated Python cache directories.
6. Commit and push to `master`.
