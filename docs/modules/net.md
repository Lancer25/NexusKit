# nexus_net

`nexus_net` contains cross-platform networking utilities. The first component is an HTTP client wrapper that keeps cpp-httplib out of public headers.

## Current API

- `nexus::net::HttpHeader`: request or response header name/value pair.
- `nexus::net::HttpQueryParameter`: query parameter name/value pair.
- `nexus::net::HttpResponse`: HTTP status code, body, response headers, 2xx status helper, and case-insensitive header lookup.
- `nexus::net::HttpClientOptions`: connection, read, and write timeout values.
- `nexus::net::HttpClient`: copyable HTTP client handle.
- `nexus::net::HttpClient::create`: validates a base URL and returns `nexus::Result<HttpClient>`.
- `HttpClient::get`: performs an HTTP GET with optional request headers and returns `nexus::Result<HttpResponse>`.
- `HttpClient::post`: performs an HTTP POST with a body, content type, and optional request headers.
- `HttpClient::put`: performs an HTTP PUT with a body, content type, and optional request headers.
- `HttpClient::del`: performs an HTTP DELETE with optional request headers.
- `nexus::net::build_query_path`: appends percent-encoded query parameters to a request path.
- `nexus::net::TcpEndpoint`: TCP host and port.
- `nexus::net::TcpClient`: synchronous TCP client handle with connect, write, read, close, and open-state checks.
- `nexus::net::UdpEndpoint`: UDP host and port.
- `nexus::net::UdpDatagram`: UDP payload and remote endpoint.
- `nexus::net::UdpSocket`: synchronous UDP socket handle with bind, send, receive, close, and open-state checks.

## Scope

The initial HTTP client supports `http://` base URLs without a path, GET, POST, PUT, DELETE, request headers, response headers, query path construction, and whole-body responses. The initial TCP and UDP clients are synchronous and intended as low-level building blocks. TLS, redirects, HTTP streaming, async transports, and WebSocket support are planned follow-up work.

## Backend

The current HTTP backend is cpp-httplib. The current TCP backend is standalone Asio. Both are included only from implementation and test files, so users of `nexus_net` depend on NexusKit headers instead of third-party networking headers.

## Error Model

- Unsupported or malformed base URLs return `StatusCode::kInvalidArgument`.
- Transport failures return `StatusCode::kUnavailable` where possible.
- Missing response headers return `StatusCode::kNotFound`.
- Invalid TCP endpoints return `StatusCode::kInvalidArgument`.
- TCP operations on closed clients return `StatusCode::kFailedPrecondition`.
- Invalid UDP endpoints return `StatusCode::kInvalidArgument`.
- UDP operations on closed sockets return `StatusCode::kFailedPrecondition`.
- HTTP status codes such as 404 are returned as successful `HttpResponse` values because the request completed at the protocol level.
