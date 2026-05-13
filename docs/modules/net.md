# nexus_net

`nexus_net` contains cross-platform networking utilities. The first component is an HTTP client wrapper that keeps cpp-httplib out of public headers.

## Current API

- `nexus::net::HttpHeader`: request or response header name/value pair.
- `nexus::net::HttpResponse`: HTTP status code, body, and response headers.
- `nexus::net::HttpClientOptions`: connection, read, and write timeout values.
- `nexus::net::HttpClient`: copyable HTTP client handle.
- `nexus::net::HttpClient::create`: validates a base URL and returns `nexus::Result<HttpClient>`.
- `HttpClient::get`: performs an HTTP GET with optional request headers and returns `nexus::Result<HttpResponse>`.
- `HttpClient::post`: performs an HTTP POST with a body, content type, and optional request headers.

## Scope

The initial client supports `http://` base URLs without a path, GET, POST, request headers, response headers, and whole-body responses. TLS, PUT/DELETE helpers, redirects, richer query helpers, streaming, and WebSocket support are planned follow-up work.

## Backend

The current backend is cpp-httplib. It is included only from implementation and test files, so users of `nexus_net` depend on NexusKit headers instead of third-party HTTP headers.

## Error Model

- Unsupported or malformed base URLs return `StatusCode::kInvalidArgument`.
- Transport failures return `StatusCode::kUnavailable` where possible.
- HTTP status codes such as 404 are returned as successful `HttpResponse` values because the request completed at the protocol level.
