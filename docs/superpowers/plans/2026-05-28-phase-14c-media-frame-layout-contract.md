# Phase 14C Media Frame Layout Contract Plan

## Scope

Add a documentation contract check for `MediaFrame` layout semantics.

## Steps

1. Add a focused failing unit test with a temporary `media.h` whose
   `MediaFrame` comment omits packed/planar audio and backend-native video
   guidance.
2. Implement a `check_media_frame_layout_contracts` helper in
   `scripts/check_public_header_contracts.py`.
3. Add a real-repository guard test that calls the helper.
4. Run focused public header contract tests and repository hygiene checks.
5. Commit and push to `master`.

## Verification

```powershell
python -m unittest tests.scripts.test_check_public_header_contracts tests.scripts.test_public_header_contracts -v
python scripts\check_public_header_contracts.py --dir .
python -m unittest discover -s tests/scripts -p "test_*.py" -v
python scripts\check_docs.py --dir .
python scripts\check_text_encoding.py --dir .
git diff --check
```
