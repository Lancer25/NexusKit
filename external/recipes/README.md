# Third-Party Dependency Recipes

NexusKit keeps dependency decisions centralized. CMake recipes live under `cmake/deps`, while this directory documents dependency versions, source locations, and platform notes.

## Current Recipes

| Dependency | Version | Source revision | CMake recipe | Purpose |
| --- | --- | --- | --- | --- |
| Catch2 | v3.5.4 | `abb467ecd60fae9a727afca033c1eb5d20af2c12` | `cmake/deps/Catch2.cmake` | Unit tests |

## Rules

- A dependency recipe must honor `NEXUS_BUILD_DEPS`.
- When `NEXUS_BUILD_DEPS=ON`, the recipe may fetch or build the dependency from source.
- When `NEXUS_BUILD_DEPS=OFF`, the recipe must use `find_package` or another explicit user-provided path.
- Git-based recipes should use `NEXUS_GIT_CONFIG_ARGS`.
- Git-based recipes should pin immutable commit IDs, not mutable branch or tag names.
- Production dependencies must be documented here before they are used by a module.
