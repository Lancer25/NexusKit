# Phase 15A Media Remux Example Plan

## Scope

Add a public API media remux example.

## Steps

1. Add a failing `test_media_remux_uses_public_nexus_media_api` contract test.
2. Add `examples/media_remux/main.cpp` and `CMakeLists.txt`.
3. Register `media_remux` in `examples/CMakeLists.txt` when `nexus::media`
   exists.
4. Update README and `docs/modules/media.md` with usage.
5. Run focused script tests, docs checks, text checks, and an example target
   configure/build smoke when available.
6. Commit and push to `master`.

## Verification

```powershell
python -m unittest tests.scripts.test_examples_contracts -v
python -m unittest discover -s tests/scripts -p "test_*.py" -v
python scripts\check_docs.py --dir .
python scripts\check_text_encoding.py --dir .
cmake --preset windows-msvc-release -B build/phase15a-media-remux-example -DNEXUS_ENABLE_MEDIA=ON -DNEXUS_BUILD_EXAMPLES=ON -DNEXUS_BUILD_TESTS=OFF
cmake --build build/phase15a-media-remux-example --config Release --target nexus_example_media_remux
git diff --check
```
