# nexus_common

`nexus_common` contains shared utilities used by higher-level NexusKit modules. The first component is a JSON value wrapper that keeps nlohmann_json out of public headers.

## Current API

- `nexus::common::JsonValue`: copyable JSON value handle with object, array, string, integer, boolean, and null support.
- `nexus::common::parse_json`: parses text and returns `nexus::Result<JsonValue>`.
- `JsonValue::set`: assigns object fields and returns `nexus::Status`.
- `JsonValue::at`: reads an object field and returns `nexus::Result<JsonValue>`.
- `JsonValue::as_string`, `JsonValue::as_int64`, `JsonValue::as_bool`: typed accessors.
- `JsonValue::dump`: serializes compact JSON text.

## Backend

The current backend is nlohmann_json. It is included only from the implementation file, so users of `nexus_common` depend on NexusKit headers instead of third-party JSON headers.

## Error Model

- Invalid JSON input returns `StatusCode::kInvalidArgument`.
- Missing object keys return `StatusCode::kNotFound`.
- Lookups or writes on non-object values return `StatusCode::kFailedPrecondition`.
- Typed accessors on incompatible values return `StatusCode::kFailedPrecondition`.
