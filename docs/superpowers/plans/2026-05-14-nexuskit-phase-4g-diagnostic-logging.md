# NexusKit Phase 4G: Diagnostic Logging

## Goal

Add a library-level diagnostic logging path so NexusKit modules can emit useful troubleshooting events without exposing third-party logging types or forcing every API to accept a logger parameter.

## Scope

- Add a process-wide default logger API to `nexus_log`.
- Keep diagnostic writes as no-ops when no default logger is installed.
- Route `nexus_net` HTTP, TCP, and UDP key events through the default logger.
- Document that `nexus_net` requires `nexus_log`.

## Verification

- Add failing tests for default logger installation and HTTP diagnostic output.
- Build targeted log and net test binaries.
- Run targeted diagnostic tests.
- Run full configure, build, test, install, and diff checks before committing.
