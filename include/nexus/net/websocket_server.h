#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/net/export.h>

/// WebSocket server backed by websocketpp.
///
/// Backend headers are private; users do not need to link against websocketpp.
namespace nexus::net {

namespace detail {
class WebSocketServerStorage;
}

/// Unique identifier for a connected client.
using WebSocketClientId = std::uint64_t;

/// Timeout configuration for `WebSocketServer`.
struct WebSocketServerOptions {
    /// Close handshake timeout for individual client connections.
    std::chrono::milliseconds close_timeout{1000};
    /// Send timeout for each `send_text` call. 0 means no explicit timeout.
    std::chrono::milliseconds write_timeout{0};
};

/// Move-only RAII WebSocket server with sync and async I/O.
///
/// Created via `create(options)`.  Use `set_on_connect`, `set_on_message`,
/// and `set_on_disconnect` to register event handlers before calling
/// `listen(port)` or `async_listen(port, handler)`.
///
/// Supports text frames only.  Sync methods block; async methods deliver
/// results via callbacks on websocketpp's internal event-loop thread.
class NEXUS_NET_API WebSocketServer {
public:
    using ClientId = WebSocketClientId;

    using ConnectHandler    = std::function<void(ClientId)>;
    using DisconnectHandler = std::function<void(ClientId)>;
    using MessageHandler    = std::function<void(ClientId, Result<std::string>)>;
    using SendHandler       = std::function<void(Status)>;
    using ListenHandler     = std::function<void(Status)>;
    using CloseHandler      = std::function<void(Status)>;

    /// Creates a server with a running event-loop thread (no listening).
    ///
    /// Register callbacks via `set_on_*` before calling `listen`.
    ///
    /// @retval kInternal when the worker thread fails to start.
    static Result<WebSocketServer> create(WebSocketServerOptions options = {});

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;
    WebSocketServer(WebSocketServer&& other) noexcept;
    WebSocketServer& operator=(WebSocketServer&& other) noexcept;
    /// Closes all connections and stops the event loop.
    ~WebSocketServer();

    /// Sets the handler invoked when a new client connects.
    void set_on_connect(ConnectHandler h);

    /// Sets the handler invoked when a client disconnects.
    void set_on_disconnect(DisconnectHandler h);

    /// Sets the handler invoked when a text message arrives.
    void set_on_message(MessageHandler h);

    /// True while the server is listening for connections.
    bool is_listening() const;

    /// Number of currently connected clients.
    std::size_t client_count() const;

    /// Starts listening on a port.  May block briefly.
    ///
    /// @retval kInvalidArgument when `port` is zero.
    /// @retval kUnavailable when the listen fails.
    Status listen(std::uint16_t port);

    /// Asynchronously starts listening on a port.
    void async_listen(std::uint16_t port, ListenHandler h);

    /// Sends a text frame to a client.  May block.
    ///
    /// @retval kFailedPrecondition when the server is not listening.
    /// @retval kNotFound when `client` is not a known client ID.
    /// @retval kUnavailable on send error or write timeout.
    Status send_text(ClientId client, std::string_view message);

    /// Asynchronously sends a text frame to a client.
    void async_send_text(ClientId client, std::string_view message, SendHandler h);

    /// Closes a single client connection synchronously.
    Status close_client(ClientId client);

    /// Closes a single client connection asynchronously.
    void async_close_client(ClientId client, CloseHandler h);

    /// Stops listening and closes all connections synchronously.  Idempotent.
    Status close();

    /// Stops listening and closes all connections asynchronously.  Idempotent.
    void async_close(CloseHandler h);

private:
    explicit WebSocketServer(std::shared_ptr<detail::WebSocketServerStorage> storage);

    std::shared_ptr<detail::WebSocketServerStorage> storage_;
};

} // namespace nexus::net
