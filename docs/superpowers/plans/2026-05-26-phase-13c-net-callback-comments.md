# Phase 13C Net Callback Comments Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Doxygen comments for public `nexus_net` callback typedefs.

**Architecture:** A Python contract test checks that all `using *Handler = ...;` declarations in public net headers have nearby `///` comments. Header-only documentation changes then describe thread context, invocation count, status/result semantics, and payload lifetime.

**Tech Stack:** C++ public headers, Python unittest.

---

### Task 1: Callback Typedef Contract Test

**Files:**
- Create: `tests/scripts/test_public_header_contracts.py`

- [ ] **Step 1: Write the failing test**

The test scans `include/nexus/net/*.h` for public `using *Handler = ...;` declarations and requires a preceding `///` Doxygen comment within the nearby comment block.

Run:

```powershell
python -m unittest tests.scripts.test_public_header_contracts -v
```

Expected before header updates: failure listing undocumented callback typedefs.

### Task 2: TCP, TCP Listener, UDP, And HTTP Comments

**Files:**
- Modify: `include/nexus/net/tcp.h`
- Modify: `include/nexus/net/tcp_listener.h`
- Modify: `include/nexus/net/udp.h`
- Modify: `include/nexus/net/http.h`

- [ ] **Step 1: Add comments**

Document worker-thread callback execution, exactly-once completion for async operations, error Status/Result meanings, and string/buffer lifetime requirements for write operations.

- [ ] **Step 2: Run contract test**

```powershell
python -m unittest tests.scripts.test_public_header_contracts -v
```

Expected: TCP/UDP/HTTP handler typedefs pass.

### Task 3: WebSocket Client/Server Comments

**Files:**
- Modify: `include/nexus/net/websocket_client.h`
- Modify: `include/nexus/net/websocket_server.h`

- [ ] **Step 1: Add comments**

Document websocket event-loop callback execution, one-shot operation handlers, repeated event handlers, client id validity, message payload ownership, and error semantics.

- [ ] **Step 2: Run all script checks**

```powershell
python -m unittest tests.scripts.test_public_header_contracts tests.scripts.test_check_release_docs tests.scripts.test_examples_contracts -v
python scripts/check_release_docs.py --dir .
python scripts/check_public_comments.py --dir .
```

Expected: all checks pass.

### Task 4: Commit And Push

**Files:**
- Read: all modified files.

- [ ] **Step 1: Final diff check**

```powershell
git diff --check
```

- [ ] **Step 2: Commit and push**

```powershell
git add include/nexus/net tests/scripts/test_public_header_contracts.py docs/superpowers/specs/2026-05-26-phase-13c-net-callback-comments-design.md docs/superpowers/plans/2026-05-26-phase-13c-net-callback-comments.md
git commit -m "docs: document net callback contracts"
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
git push
```
