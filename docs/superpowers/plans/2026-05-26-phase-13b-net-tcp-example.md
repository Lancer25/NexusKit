# Phase 13B Net TCP Example Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a loopback TCP echo example for `nexus_net`.

**Architecture:** The example lives under `examples/net_tcp_echo`, links only `nexus::net`, and uses `TcpListener::create`, `TcpListener::async_accept`, `TcpClient::connect`, `write_all`, `read_some`, and `close`. A Python contract test enforces public API usage and CMake gating.

**Tech Stack:** C++17, CMake, Python unittest.

---

### Task 1: Example Contract Test

**Files:**
- Modify: `tests/scripts/test_examples_contracts.py`

- [ ] **Step 1: Write the failing test**

Add a test that asserts:

- `examples/net_tcp_echo/main.cpp` exists.
- `examples/net_tcp_echo/CMakeLists.txt` exists.
- The source includes `<nexus/net/tcp.h>` and `<nexus/net/tcp_listener.h>`.
- The source does not mention `asio`, `httplib`, or `websocketpp`.
- The CMake file links `nexus::net` and does not mention private backend targets.
- `examples/CMakeLists.txt` gates the example with `if(TARGET nexus::net)`.

Run:

```powershell
python -m unittest tests.scripts.test_examples_contracts -v
```

Expected before implementation: failure because `examples/net_tcp_echo` does not exist.

### Task 2: TCP Echo Example

**Files:**
- Create: `examples/net_tcp_echo/main.cpp`
- Create: `examples/net_tcp_echo/CMakeLists.txt`
- Modify: `examples/CMakeLists.txt`

- [ ] **Step 1: Implement the example**

The example must:

- bind to `127.0.0.1:0`,
- start `async_accept` before connecting the client,
- connect a local client to the assigned port,
- send `"hello nexus"` from client to accepted server socket,
- echo the same payload back,
- print `echo=hello nexus`,
- close sockets before exit.

- [ ] **Step 2: Re-run the contract test**

```powershell
python -m unittest tests.scripts.test_examples_contracts -v
```

Expected: pass.

### Task 3: Documentation Update

**Files:**
- Modify: `README.md`
- Modify: `docs/modules/net.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Add example references**

Mention `nexus_example_net_tcp_echo` in README and `docs/modules/net.md`.

- [ ] **Step 2: Run docs checks**

```powershell
python scripts/check_release_docs.py --dir .
python scripts/check_public_comments.py --dir .
```

Expected: pass.

### Task 4: Final Verification

**Files:**
- Read: all modified files.

- [ ] **Step 1: Run script tests**

```powershell
python -m unittest tests.scripts.test_check_release_docs tests.scripts.test_examples_contracts -v
```

- [ ] **Step 2: Run CMake example smoke when possible**

```powershell
cmake --preset windows-msvc-release -B build/phase13b-net-tcp-example -DNEXUS_BUILD_EXAMPLES=ON -DNEXUS_BUILD_TESTS=OFF
cmake --build build/phase13b-net-tcp-example --config Release --target nexus_example_net_tcp_echo
```

- [ ] **Step 3: Commit and push**

```powershell
git add examples README.md docs/modules/net.md CHANGELOG.md tests/scripts/test_examples_contracts.py docs/superpowers/specs/2026-05-26-phase-13b-net-tcp-example-design.md docs/superpowers/plans/2026-05-26-phase-13b-net-tcp-example.md
git commit -m "docs: add net tcp echo example guidance"
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
git push
```
