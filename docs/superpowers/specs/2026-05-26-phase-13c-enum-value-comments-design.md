# Phase 13C Enum Value Comments Design

## Problem

Public enum declarations describe broad categories, but some enum values still
require callers to infer exact semantics from implementation details or tests.
That makes new API review less consistent and leaves generated documentation
with uneven detail.

## Scope

- Document selected public enum values in core, log, media, and camera headers.
- Add a lightweight public-header contract test so these enums keep value-level
  Doxygen comments.
- Keep public headers platform-neutral and free of private backend types.
- Do not change enum names, numeric values, ABI layout, or runtime behavior.

## Contract

- Each tracked enum value has a nearby preceding `///` comment.
- The contract test reports the header, line, enum name, and value when a
  tracked enum value is missing documentation.
- Existing callback typedef documentation checks keep their current behavior.

## Verification

- Run the public header contract test red before adding comments.
- Run public documentation checks after comments are added.
- Run whitespace checks before commit.
