# NexusKit Phase 2C FFmpeg Targets Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans and TDD-style red/green verification.

**Goal:** Make the locally built FFmpeg usable by future NexusKit modules through stable CMake imported targets.

## Tasks

- [ ] Add an opt-in FFmpeg probe example that includes FFmpeg headers and links public FFmpeg targets.
- [ ] Verify the probe fails before targets are provided.
- [ ] Add imported targets for the source-built FFmpeg libraries.
- [ ] Document the exported target names and runtime output location.
- [ ] Verify default build/tests still pass and the FFmpeg probe builds after the source dependency target.
