# nexus_common

`nexus_common` contains shared utilities used by higher-level NexusKit modules. The first components are JSON and XML wrappers that keep third-party implementation headers out of public NexusKit APIs.

## Current API

- `nexus::common::JsonValue`: copyable JSON value handle with object, array, string, integer, boolean, and null support.
- `nexus::common::parse_json`: parses text and returns `nexus::Result<JsonValue>`.
- `JsonValue::set`: assigns object fields and returns `nexus::Status`.
- `JsonValue::at`: reads an object field and returns `nexus::Result<JsonValue>`.
- `JsonValue::as_string`, `JsonValue::as_int64`, `JsonValue::as_bool`: typed accessors.
- `JsonValue::dump`: serializes compact JSON text.
- `nexus::common::XmlDocument`: copyable XML document handle with root lookup and compact serialization.
- `nexus::common::XmlNode`: XML node handle with name, text, attribute, child, and mutation helpers.
- `nexus::common::parse_xml`: parses text and returns `nexus::Result<XmlDocument>`.

## Backend

The current JSON backend is nlohmann_json. The current XML backend is pugixml. Both are included only from implementation files, so users of `nexus_common` depend on NexusKit headers instead of third-party headers.

## Error Model

- Invalid JSON input returns `StatusCode::kInvalidArgument`.
- Invalid XML input returns `StatusCode::kInvalidArgument`.
- Missing object keys return `StatusCode::kNotFound`.
- Missing XML attributes or child elements return `StatusCode::kNotFound`.
- Lookups or writes on non-object values return `StatusCode::kFailedPrecondition`.
- Operations on empty XML nodes return `StatusCode::kFailedPrecondition`.
- Typed accessors on incompatible values return `StatusCode::kFailedPrecondition`.
