# Phase 14C Screen Region Contract Plan

## Scope

Add an automated public header contract check for `ScreenCaptureRegion`.

## Steps

1. Add a failing checker unit test with a temporary `screen.h` whose
   `ScreenCaptureRegion` comment omits display-relative, primary-display, and
   positive-dimension wording.
2. Implement `check_screen_region_contracts` in
   `scripts/check_public_header_contracts.py`.
3. Add a real-repository guard test that calls the new helper.
4. Run focused and full script-level verification.
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
