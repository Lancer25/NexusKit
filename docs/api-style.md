# API Style Guide

This guide defines the public API documentation and boundary rules for NexusKit headers under `include/nexus`.

## Public Header Contract

Public headers are part of the stable user-facing surface. They must stay platform-neutral unless the API is explicitly platform-specific.

- Do not include Windows, Linux, X11, DXGI, FFmpeg, hidapi, Asio, spdlog, nlohmann_json, pugixml, or other backend-private headers from public headers.
- Prefer standard-library types, NexusKit value types, `nexus::Status`, and `nexus::Result<T>`.
- Keep native handles and backend ownership in private storage classes under `detail`.
- Use opaque string ids for platform resources such as windows and displays.
- Keep constructors, move behavior, and ownership semantics explicit for RAII objects.

## Comment Format

Every public type and function in `include/nexus/**/*.h` should have Doxygen-style comments before it is considered complete.

Use `///` comments for short API items. Use a concise paragraph first, followed by tags when useful:

```cpp
/// Opens a TCP connection to the endpoint.
///
/// @param endpoint Host and port to connect to.
/// @return An open client on success. Returns `StatusCode::kInvalidArgument`
/// for invalid endpoint data and `StatusCode::kUnavailable` when the socket
/// backend cannot connect.
static Result<TcpClient> connect(const TcpEndpoint& endpoint);
```

For structs, document fields when their meaning is not obvious, when zero has special meaning, or when values are opaque:

```cpp
/// Capturable visible desktop window metadata.
struct ScreenWindow {
    /// Opaque backend id. Do not persist across runs.
    std::string id;
};
```

## Required Details

Document these details whenever they apply:

- Ownership and lifetime: who owns resources, whether `close()` is idempotent, and whether moved-from objects are closed.
- Blocking behavior: whether calls may block, wait for I/O, capture a frame, or use a timeout.
- Error model: expected `StatusCode` values for invalid input, missing resources, unavailable backends, end-of-stream, and internal failures.
- Platform differences: Windows vs Linux behavior, unsupported backends, and best-effort behavior.
- Data layout: byte order, sample format, pixel format, row contiguity, alpha handling, and units.
- Threading assumptions: whether an object is intended for single-threaded use unless documented otherwise.

## Status and Result Rules

- Recoverable failures return `Status` or `Result<T>`.
- Empty or malformed caller input should return `StatusCode::kInvalidArgument`.
- Missing files, streams, windows, displays, or end-of-stream should return `StatusCode::kNotFound` when the backend is otherwise usable.
- Unsupported runtime state or unavailable optional backends should return `StatusCode::kFailedPrecondition`.
- Temporary absence of a frame or readiness can return `StatusCode::kUnavailable`.
- Unexpected backend failures should return `StatusCode::kInternal`.

## Documentation Sync

Any public API addition or behavior change must update:

- The public header comments.
- The matching `docs/modules/*.md` page.
- `CHANGELOG.md`.
- `README.md` or `docs/architecture.md` when the module-level capability changes.
- Tests that cover at least one success path and the most important failure path.

## Current Documentation Roadmap

The existing public headers predate this guide and are not fully annotated yet. Annotate them in small phases:

1. `nexus_core` and `nexus_common`.
2. `nexus_media` and `nexus_screen`.
3. `nexus_net`, `nexus_usb`, `nexus_hid`, and `nexus_log`.
