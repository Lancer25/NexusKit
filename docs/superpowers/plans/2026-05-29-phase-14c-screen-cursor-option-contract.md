# Phase 14C Screen Cursor Option Contract Plan

## Scope

Add a focused public header contract check for
`ScreenCaptureOptions::include_cursor`.

## Steps

1. Add a failing checker unit test with a temporary `screen.h` whose
   `include_cursor` comment omits Windows best-effort and non-failing capture
   semantics.
2. Implement `check_screen_cursor_option_contracts` in
   `scripts/check_public_header_contracts.py`.
3. Add a real-repository guard test that calls the new helper.
4. Run focused and full script-level verification.
5. Integrate with the parallel Phase 14D roadmap cleanup.

## Verification

```powershell
python -m unittest tests.scripts.test_check_public_header_contracts tests.scripts.test_public_header_contracts -v
python scripts\check_public_header_contracts.py --dir .
python -m unittest discover -s tests/scripts -p "test_*.py" -v
python scripts\check_docs.py --dir .
python scripts\check_text_encoding.py --dir .
git diff --check
```
