# NexusKit Phase 3C Common XML Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use TDD for public XML behavior and verification-before-completion before committing.

**Goal:** Extend `nexus_common` with XML document and node utilities.

## Scope

- Add pugixml as a private backend dependency of `nexus_common`.
- Keep pugixml out of public headers.
- Provide a minimal XML document/node API based on `nexus::Status` and `nexus::Result<T>`.
- Add unit tests for parsing, invalid input, document construction, attributes, children, and missing data.
- Document the module.

## Public API

- `nexus::common::XmlDocument`
- `nexus::common::XmlNode`
- `nexus::common::parse_xml`

## Verification

- Configure and build with MSVC 2022.
- Run all CTest tests.
- Install the package to confirm target export still works.
