# nexus_common

`nexus_common` contains shared utilities used by higher-level NexusKit modules. It provides JSON/XML wrappers, string manipulation, time helpers, binary I/O, platform abstractions, math helpers, diagnostic logging, and status factories.

## Current API

### JSON and XML

- `nexus::common::JsonValue`: copyable JSON value handle with object, array, string, integer, boolean, and null support.
- `nexus::common::parse_json`: parses text and returns `nexus::Result<JsonValue>`.
- `JsonValue::set`: assigns object fields and returns `nexus::Status`.
- `JsonValue::at`: reads an object field and returns `nexus::Result<JsonValue>`.
- `JsonValue::as_string`, `JsonValue::as_int64`, `JsonValue::as_bool`: typed accessors.
- `JsonValue::dump`: serializes compact JSON text.
- `nexus::common::XmlDocument`: copyable XML document handle with root lookup and compact serialization.
- `nexus::common::XmlNode`: XML node handle with name, text, attribute, child, and mutation helpers.
- `nexus::common::parse_xml`: parses text and returns `nexus::Result<XmlDocument>`.

### String Utilities (`nexus/common/string.h`)

- `starts_with`, `ends_with`, `contains`: `std::string_view` substring checks.
- `contains_space`: whitespace detection.
- `equals_ignore_case`: case-insensitive comparison.
- `trim`, `trim_left`, `trim_right`: whitespace trimming returning `std::string_view`.
- `split`: single-character delimiter split returning `std::vector<std::string>`.
- `char_to_string`: null-safe `const char*` to `std::string` conversion.
- `wide_to_utf8`, `utf8_to_wide`: cross-platform wide/UTF-8 conversion (Windows: `WideCharToMultiByte`, POSIX: `wcsrtombs`).
- `percent_encode`, `percent_decode`: RFC 3986 percent-encoding.

### Time Utilities (`nexus/common/time.h`)

- `steady_timestamp_ms`: monotonic millisecond timestamp.
- `system_timestamp_ms`: wall-clock epoch millisecond timestamp.

### Binary Utilities (`nexus/common/binary.h`)

- `write_le16`, `write_le32`: write little-endian integers to `std::ostream`.
- `append_le16`, `append_le32`: append little-endian integers to `std::vector<std::uint8_t>`.
- `read_le16`, `read_le32`: read little-endian integers from byte buffers.
- `hex_encode`, `hex_decode`: hex string to/from byte conversion.

### Platform Abstraction (`nexus/common/platform.h`)

- `NEXUS_PLATFORM_WINDOWS` / `NEXUS_PLATFORM_LINUX` macros.
- Public platform detection without including native platform SDK headers.

### Math Utilities (`nexus/common/math.h`)

- `min`, `max`: variadic template over `std::initializer_list<T>`.
- `clamp`: constrain a value to [lo, hi].
- `absolute_difference`: unsigned distance between two values.

### Diagnostic Logging (`nexus/common/logging.h`)

- `diagnostic_log`: writes a log message through `nexus::log`.

### Thread Utilities (`nexus/common/thread.h`)

- `nexus::common::Event`: cross-platform manual-reset waitable event.
  - `emit()` signals the event, waking all waiters.
  - `reset()` returns the event to unsignaled state.
  - `wait(timeout_ms)` blocks until signaled, with optional timeout.
- `nexus::common::Thread`: move-only background thread with start/stop lifecycle.
  - `start(callback, interval)`: runs the callback on a new thread. When `interval` is non-zero, the callback repeats with a sleep between invocations.
  - `stop()`: synchronous stop; signals stop and joins the thread.
  - `request_stop()`: asynchronous stop request without joining.
  - `is_running()`, `stop_requested()`: state queries.
  - `run_async(callback)`: fire-and-forget on a detached thread.

### Status Factories (`nexus/common/status.h`)

- `failed_precondition`, `unavailable`, `invalid_argument`, `not_found`, `internal_error`: Status factory helpers with message.

All public types and functions are annotated with Doxygen `///` comments following `docs/api-style.md`.

## Backend

The current JSON backend is nlohmann_json. The current XML backend is pugixml. Both are included only from implementation files, so users of `nexus_common` depend on NexusKit headers instead of third-party headers.

Group 1 utilities (string, time, binary, platform, math) depend only on C++17. Group 2 utilities (logging, status) depend on `nexus_core` and `nexus_log`.

## Error Model

- Invalid JSON input returns `StatusCode::kInvalidArgument`.
- Invalid XML input returns `StatusCode::kInvalidArgument`.
- Missing object keys return `StatusCode::kNotFound`.
- Missing XML attributes or child elements return `StatusCode::kNotFound`.
- Lookups or writes on non-object values return `StatusCode::kFailedPrecondition`.
- Operations on empty XML nodes return `StatusCode::kFailedPrecondition`.
- Typed accessors on incompatible values return `StatusCode::kFailedPrecondition`.
