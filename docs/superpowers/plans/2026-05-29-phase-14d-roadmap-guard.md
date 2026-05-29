# Phase 14D Roadmap Guard Plan

## Steps

1. Add this design spec and implementation plan.
2. Add a failing release-docs unit test that expects a Near-Term Roadmap
   `- Phase 14C:` bullet to be rejected.
3. Update `scripts/check_release_docs.py` to flag completed Phase 14C roadmap
   bullets.
4. Update `docs/iteration.md` so Phase 14C is listed as completed recent work
   and the Near-Term Roadmap points to Phase 14D/15A follow-ups.
5. Run focused release-docs tests and repository documentation/text checks.

## Verification

```powershell
python -m unittest tests.scripts.test_check_release_docs -v
python scripts\check_docs.py --dir .
python scripts\check_text_encoding.py --dir .
git diff --check
```
