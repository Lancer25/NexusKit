# nexus_net

`nexus_net` contains cross-platform networking utilities. It keeps cpp-httplib, Asio, and websocketpp out of public headers and routes internal diagnostics through `nexus_log`.

## Current API

- `nexus::net::HttpHeader`: request or response header name/value pair.
- `nexus::net::HttpQueryParameter`: query parameter name/value pair.
- `nexus::net::HttpResponse`: HTTP status code, body, response headers, 2xx status helper, and case-insensitive header lookup.
- `nexus::net::HttpClientOptions`: connection, read, and write timeout values.
- `nexus::net::HttpClient`: copyable HTTP client handle with sync and async methods.
- `nexus::net::HttpClient::create`: validates a base URL and returns `nexus::Result<HttpClient>`.
- `HttpClient::get`: performs an HTTP GET with optional request headers and returns `nexus::Result<HttpResponse>`.
- `HttpClient::post`: performs an HTTP POST with a body, content type, and optional request headers.
- `HttpClient::put`: performs an HTTP PUT with a body, content type, and optional request headers.
- `HttpClient::del`: performs an HTTP DELETE with optional request headers.
- `HttpClient::async_get`, `async_post`, `async_put`, `async_del`: asynchronous HTTP methods delivering `Result<HttpResponse>` via callback on a background worker thread.
- `HttpClient::async_close`: asynchronous close delivering `Status` via callback.
- `nexus::net::build_query_path`: appends percent-encoded query parameters to a request path.
- `nexus::net::TcpEndpoint`: TCP host and port.
- `nexus::net::TcpConnectOptions`: TCP connect timeout.
- `nexus::net::TcpIoOptions`: TCP read and write timeout.
- `nexus::net::TcpClient`: move-only RAII TCP client handle with sync (`connect`, `write_all`, `read_some`, `close`) and async (`async_connect`, `async_write_all`, `async_read_some`, `async_close`) operations.
- `nexus::net::UdpEndpoint`: UDP host and port.
- `nexus::net::UdpDatagram`: UDP payload and remote endpoint.
- `nexus::net::UdpReceiveOptions`: UDP receive timeout.
- `nexus::net::UdpSocket`: move-only RAII UDP socket with sync (`bind`, `send_to`, `receive_from`, `close`) and async (`async_bind`, `async_send_to`, `async_receive_from`, `async_close`) operations, plus `local_port()` query.
- `nexus::net::WebSocketClientOptions`: WebSocket timeout configuration including connect, receive, close, and write timeouts.
- `nexus::net::WebSocketClient`: move-only RAII WebSocket client with sync (`connect`, `send_text`, `receive_text`, `close`) and async (`async_connect`, `async_send_text`, `async_receive_text`, `async_close`) operations.
- `nexus::net::WebSocketClientId`: unique numeric identifier for a connected WebSocket client.
- `nexus::net::WebSocketServerOptions`: server timeout configuration including close and write timeouts.
- `nexus::net::WebSocketServer`: move-only RAII WebSocket server with sync/async listen, per-client send, connect/disconnect/message callbacks, client disconnect, and close operations.
- `nexus::net::TcpListenOptions`: TCP listener configuration (reuse_address, backlog).
- `nexus::net::TcpListener`: move-only RAII TCP listener that binds to an endpoint and accepts incoming connections via async callback.

## Scope

The HTTP client supports `http://` and `https://` (when built with OpenSSL) base URLs without a path, GET, POST, PUT, DELETE, request headers, response headers, and query path construction. All I/O types provide both sync (blocking) and async (callback-driven via private Asio io_context workers) operations: `TcpClient`, `UdpSocket`, `HttpClient`, `WebSocketClient`, and `WebSocketServer`. `TcpListener` provides asynchronous accept via background worker thread, delivering connected `TcpClient` instances through an `AcceptHandler` callback. WebSocket supports `ws://` and `wss://` (when built with OpenSSL). The `WebSocketServer` provides multi-client listen/accept, per-client send, and connection lifecycle callbacks. HTTP redirects are supported via `HttpClientOptions::follow_redirects`. HTTP streaming is planned follow-up work.

## Example

When `NEXUS_BUILD_EXAMPLES=ON` and `NEXUS_ENABLE_NET=ON`, NexusKit builds `nexus_example_net_tcp_echo`.

```powershell
nexus_example_net_tcp_echo
```

The example binds a local TCP listener to `127.0.0.1:0`, connects a `TcpClient` to the assigned port, echoes a short message through the accepted socket, and exits without external network access.

## Backend

The current HTTP backend is cpp-httplib (with optional OpenSSL for TLS). The current TCP and UDP backend is standalone Asio. The current WebSocket backend is websocketpp on standalone Asio (with optional OpenSSL for wss://). These are included only from implementation and test files, so users of `nexus_net` depend on NexusKit headers instead of third-party networking headers.

## Diagnostics

HTTP requests, TCP connection/read/write/close operations, UDP bind/send/receive/close operations, and WebSocket connection/send/receive/close operations write diagnostic messages through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Without a default logger, diagnostics are silent. Async operations deliver results via callbacks on internal worker threads.

## Error Model

- Unsupported or malformed base URLs return `StatusCode::kInvalidArgument`.
- Transport failures return `StatusCode::kUnavailable` where possible.
- Missing response headers return `StatusCode::kNotFound`.
- Invalid TCP endpoints return `StatusCode::kInvalidArgument`.
- TCP connect, read, and write timeouts return `StatusCode::kUnavailable`.
- TCP operations on closed clients return `StatusCode::kFailedPrecondition`.
- Invalid UDP endpoints return `StatusCode::kInvalidArgument`.
- UDP receive operations on closed sockets return `StatusCode::kFailedPrecondition`.
- UDP receive timeouts return `StatusCode::kUnavailable`.
- Invalid WebSocket URLs return `StatusCode::kInvalidArgument`.
- WebSocket transport failures and write timeouts return `StatusCode::kUnavailable`.
- WebSocket client operations on closed clients return `StatusCode::kFailedPrecondition`.
- WebSocket server operations on stopped servers return `StatusCode::kFailedPrecondition`.
- WebSocket server send to unknown client returns `StatusCode::kNotFound`.
- Async TCP/UDP operations on closed or moved-from objects return `StatusCode::kFailedPrecondition`.
- Async TCP/UDP connect/bind and receive timeouts return `StatusCode::kUnavailable`.
- Async TCP/UDP send buffer lifetime: callers own data until the write callback fires.
- HTTP status codes such as 404 are returned as successful `HttpResponse` values because the request completed at the protocol level.
- TCP listener create with empty host returns `StatusCode::kInvalidArgument`.
- TCP listener create with bind/address-resolution failure returns `StatusCode::kUnavailable`.
- TCP listener accept on a closed listener returns `StatusCode::kFailedPrecondition`.
- TCP listener accept when the acceptor is closed returns `StatusCode::kUnavailable`.
