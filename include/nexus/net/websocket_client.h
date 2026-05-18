#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/net/export.h>

/// WebSocket client backed by websocketpp.
///
/// Backend headers are private; users do not need to link against websocketpp.
namespace nexus::net {

namespace detail {
class WebSocketClientStorage;
}

/// Connection and I/O timeouts for `WebSocketClient`.
struct WebSocketClientOptions {
    /// WebSocket handshake timeout.
    std::chrono::milliseconds connect_timeout{5000};
    /// Receive timeout for each `receive_text` call.
    std::chrono::milliseconds receive_timeout{5000};
    /// Close handshake timeout.
    std::chrono::milliseconds close_timeout{1000};
    /// Send timeout for each `send_text` call. 0 means no explicit timeout.
    ///
    /// websocketpp's send is queue-based and non-blocking in normal operation.
    /// This timeout guards the rare case where the underlying write buffer is
    /// full and `send_text` blocks.
    std::chrono::milliseconds write_timeout{0};
};

/// Move-only RAII WebSocket client with sync and async I/O.
///
/// Created via `connect(url)` (sync factory) or `create(options)` followed by
/// `async_connect(url, handler)`.  Copy is deleted; move transfers ownership.
/// A moved-from client is closed and `is_open()` returns false.
///
/// Supports text frames only.  Sync methods block; async methods deliver
/// results via callbacks on websocketpp's internal event-loop thread.
class NEXUS_NET_API WebSocketClient {
public:
    using ConnectHandler = std::function<void(Status)>;
    using SendHandler    = std::function<void(Status)>;
    using MessageHandler = std::function<void(Result<std::string>)>;
    using CloseHandler   = std::function<void(Status)>;

    /// Creates a client with a running event-loop thread (no connection).
    ///
    /// Use `async_connect` to establish a connection asynchronously.
    ///
    /// @retval kInternal when the worker thread fails to start.
    static Result<WebSocketClient> create(WebSocketClientOptions options = {});

    /// Connects to a WebSocket server synchronously.
    ///
    /// May block on TCP connect and WebSocket handshake.
    ///
    /// @param url WebSocket URL (e.g. "ws://localhost:9000/ws").
    /// @return A connected client on success.
    /// @retval kInvalidArgument when `url` is empty or malformed.
    /// @retval kUnavailable on connection failure or timeout.
    static Result<WebSocketClient> connect(
        std::string url,
        WebSocketClientOptions options = {});

    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;
    WebSocketClient(WebSocketClient&& other) noexcept;
    WebSocketClient& operator=(WebSocketClient&& other) noexcept;
    /// Closes the connection if open.
    ~WebSocketClient();

    /// True when the WebSocket connection is open.
    bool is_open() const;

    /// Asynchronously connects to `url`.
    ///
    /// @retval kInvalidArgument when `url` is empty or malformed.
    /// @retval kUnavailable on connection failure or timeout.
    void async_connect(std::string url, ConnectHandler h);

    /// Sends a text frame.  May block.
    ///
    /// @retval kFailedPrecondition when the client is closed.
    /// @retval kUnavailable on send error or write timeout.
    Status send_text(std::string_view message);

    /// Asynchronously sends a text frame.
    ///
    /// @retval kFailedPrecondition when the client is closed.
    /// @retval kUnavailable on send error or write timeout.
    void async_send_text(std::string_view message, SendHandler h);

    /// Receives the next text frame.  May block up to `receive_timeout`.
    ///
    /// @return The message text on success.
    /// @retval kFailedPrecondition when the client is closed.
    /// @retval kUnavailable on timeout or receive error.
    Result<std::string> receive_text();

    /// Receives the next text frame asynchronously.
    ///
    /// Handler fires exactly once for the next message.  To keep receiving,
    /// call `async_receive_text` again from within the handler.
    ///
    /// @retval kFailedPrecondition when the client is closed.
    /// @retval kUnavailable on receive error.
    void async_receive_text(MessageHandler h);

    /// Closes the WebSocket connection synchronously.  Idempotent.
    Status close();

    /// Closes the WebSocket connection asynchronously.  Idempotent.
    ///
    /// The handler is always invoked (even if already closed).
    void async_close(CloseHandler h);

private:
    explicit WebSocketClient(std::shared_ptr<detail::WebSocketClientStorage> storage);

    static Result<WebSocketClient> connect_impl(
        std::string url,
        WebSocketClientOptions options);

    std::shared_ptr<detail::WebSocketClientStorage> storage_;
};

} // namespace nexus::net
