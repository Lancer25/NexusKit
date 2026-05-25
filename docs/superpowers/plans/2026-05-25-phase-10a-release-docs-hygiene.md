# Phase 10A Release Documentation Hygiene Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Clean up the 0.1.0 release documentation and record the next maintenance phases.

**Architecture:** This is a docs-only maintenance phase. Existing project docs remain the source of truth; the change only fixes wording, release state, and roadmap guidance.

**Tech Stack:** Markdown, CMake option audit, PowerShell validation commands.

---

### Task 1: Release State And Encoding Cleanup

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`
- Modify: `docs/architecture.md`
- Modify: `docs/iteration.md`
- Modify: `docs/api-style.md`
- Modify: `docs/modules/hid.md`
- Modify: `docs/modules/audio.md`
- Modify: `docs/modules/camera.md`

- [ ] **Step 1: Search for stale release and encoding text**

Run:

```powershell
$badRelease = 'Un' + 'released'
rg -n $badRelease README.md CHANGELOG.md docs
```

Expected before the cleanup: matches in release-facing docs.

- [ ] **Step 2: Update the release heading**

Change the top `CHANGELOG.md` release heading from its unreleased form to:

```markdown
## 0.1.0 - 2026-05-20
```

- [ ] **Step 3: Replace mojibake with ASCII punctuation**

Replace visible encoding artifacts in user-facing docs with `-` or clearer words. Preserve the meaning of each sentence.

- [ ] **Step 4: Re-run the search**

Run:

```powershell
$badRelease = 'Un' + 'released'
rg -n $badRelease README.md CHANGELOG.md docs
```

Expected after the cleanup: no matches.

### Task 2: Build Documentation Consistency

**Files:**
- Modify: `README.md`
- Modify: `docs/build.md`
- Modify: `docs/architecture.md`

- [ ] **Step 1: Inspect CMake defaults**

Run:

```powershell
rg -n "option\\(NEXUS_ENABLE_|option\\(NEXUS_BUILD_" CMakeLists.txt
```

Expected: `NEXUS_ENABLE_LOG`, `NEXUS_ENABLE_COMMON`, and `NEXUS_ENABLE_NET` default to `ON`; optional modules such as `SCREEN`, `USB`, `HID`, `MEDIA`, `AUDIO`, and `CAMERA` default to `OFF`.

- [ ] **Step 2: Correct optional module wording and commands**

Ensure `docs/build.md` states that `nexus_screen` is enabled with `NEXUS_ENABLE_SCREEN=ON`, not enabled by default. Ensure the full-build command includes `-DNEXUS_ENABLE_SCREEN=ON`. Use PowerShell backticks for PowerShell multi-line commands.

- [ ] **Step 3: Correct networking module wording**

Ensure `docs/architecture.md` describes the public HTTP API as an HTTP client. Keep WebSocket server and TCP listener wording, because those APIs are public.

- [ ] **Step 4: Re-check default wording**

Run:

```powershell
$httpServerPhrase = 'HTTP client' + '/server'
rg -n "enabled by default|NEXUS_ENABLE_SCREEN" docs/build.md docs/architecture.md README.md
rg -n $httpServerPhrase docs/architecture.md
```

Expected: only default modules are described as default; optional screen guidance says it is enabled with the option; no public HTTP server wording remains.

### Task 3: Roadmap Handoff

**Files:**
- Modify: `docs/iteration.md`

- [ ] **Step 1: Add post-release phases**

Add a concise `Post-0.1.0 Roadmap` section covering:

- Phase 13A: CI/build matrix and packaging checks.
- Phase 13B: example coverage.
- Phase 13C: public API comment and contract audit.

- [ ] **Step 2: Preserve the HID hotplug decision**

Confirm the USB/HID follow-up still says HID-level hotplug is not planned unless the product requirement changes.
Update `docs/modules/hid.md` so it no longer describes HID hotplug as planned work, and make audio/camera follow-up wording refer to endpoint/device change notifications instead of generic hotplug.

- [ ] **Step 3: Validate the roadmap text**

Run:

```powershell
rg -n "Phase 13A|Phase 13B|Phase 13C|HID-level hotplug" docs/iteration.md
```

Expected: all four topics are present.

### Task 4: Final Documentation Verification

**Files:**
- Read: `README.md`
- Read: `CHANGELOG.md`
- Read: `docs/build.md`
- Read: `docs/architecture.md`
- Read: `docs/iteration.md`
- Read: `docs/api-style.md`
- Read: `docs/modules/hid.md`
- Read: `docs/modules/audio.md`
- Read: `docs/modules/camera.md`

- [ ] **Step 1: Run the final text checks**

Run:

```powershell
$badRelease = 'Un' + 'released'
rg -n $badRelease README.md CHANGELOG.md docs
$screenDefaultPhrase = 'enabled by default through `NEXUS_ENABLE_SCREEN=ON`'
$httpServerPhrase = 'HTTP client' + '/server'
rg -n $screenDefaultPhrase docs/build.md docs/architecture.md
rg -n $httpServerPhrase docs/architecture.md
git diff --check
```

Expected: no matches for the first two commands, and `git diff --check` exits successfully.

- [ ] **Step 2: Review the changed files**

Run:

```powershell
git diff -- README.md CHANGELOG.md docs/architecture.md docs/build.md docs/iteration.md docs/api-style.md docs/modules/hid.md docs/modules/audio.md docs/modules/camera.md docs/superpowers/specs/2026-05-25-phase-10a-release-docs-hygiene-design.md docs/superpowers/plans/2026-05-25-phase-10a-release-docs-hygiene.md
```

Expected: only documentation updates related to Phase 10A.

- [ ] **Step 3: Commit and push**

Run:

```powershell
git status --short
git add README.md CHANGELOG.md docs/architecture.md docs/build.md docs/iteration.md docs/api-style.md docs/modules/hid.md docs/modules/audio.md docs/modules/camera.md docs/superpowers/specs/2026-05-25-phase-10a-release-docs-hygiene-design.md docs/superpowers/plans/2026-05-25-phase-10a-release-docs-hygiene.md
git commit -m "docs: clean up 0.1.0 release guidance"
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
git push
```

Expected: commit is created on the active branch and pushed to its upstream.
