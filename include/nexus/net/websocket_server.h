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
/// Supports text frames only.  Sync methods block. Network event paths run on
/// websocketpp's internal event-loop thread; validation and immediate
/// completion paths may invoke callbacks synchronously before the async method
/// returns.
class NEXUS_NET_API WebSocketServer {
public:
    using ClientId = WebSocketClientId;

    /// Event callback for newly connected clients.
    ///
    /// Invoked on websocketpp's event-loop thread once for each successfully
    /// opened client connection. Handshake failures that never open do not call
    /// this handler. The ClientId remains valid for server operations until the
    /// client is closed, disconnected, or removed.
    using ConnectHandler    = std::function<void(ClientId)>;
    /// Event callback for disconnected clients.
    ///
    /// Invoked on websocketpp's event-loop thread when a known client close
    /// event is observed and removed from the server's client map. The ClientId
    /// is for diagnostics only after this callback returns.
    using DisconnectHandler = std::function<void(ClientId)>;
    /// Event callback for incoming text messages.
    ///
    /// Invoked on websocketpp's event-loop thread for each received message.
    /// Receives the client id and a Result whose successful value owns a copy
    /// of the payload. Unknown-client messages are ignored rather than
    /// delivered as failed Results.
    using MessageHandler    = std::function<void(ClientId, Result<std::string>)>;
    /// Completion callback for `async_send_text`.
    ///
    /// Invoked exactly once after websocketpp accepts or rejects queueing the
    /// frame; the current implementation invokes it synchronously before
    /// `async_send_text` returns. Receives OK when queued, `kFailedPrecondition`
    /// when not listening, `kNotFound` for unknown clients, or `kUnavailable`
    /// for send errors. Message bytes are copied before the call returns.
    using SendHandler       = std::function<void(Status)>;
    /// Completion callback for `async_listen`.
    ///
    /// Invoked exactly once. Closed servers or port zero may invoke the handler
    /// synchronously before `async_listen` returns; otherwise the posted listen
    /// operation completes on websocketpp's event-loop thread. Receives OK when
    /// listening starts, `kInvalidArgument` for port zero,
    /// `kFailedPrecondition` when closed, or `kUnavailable` when listen fails.
    using ListenHandler     = std::function<void(Status)>;
    /// Completion callback for async close operations.
    ///
    /// Invoked exactly once in the current implementation, synchronously before
    /// the async close method returns. Close operations are idempotent and
    /// report OK for already-closed servers or unknown client ids.
    using CloseHandler      = std::function<void(Status)>;

    /// Creates a server with a running event-loop thread (no listening).
    ///
    /// Register callbacks via `set_on_*` before calling `listen`.
    ///
    /// @retval kInternal when the worker thread fails to start.
    static Result<WebSocketServer> create(WebSocketServerOptions options = {});

    /// WebSocket servers are move-only and cannot be copied.
    WebSocketServer(const WebSocketServer&) = delete;
    /// WebSocket servers are move-only and cannot be copy-assigned.
    WebSocketServer& operator=(const WebSocketServer&) = delete;
    /// Moves a server handle.
    WebSocketServer(WebSocketServer&& other) noexcept;
    /// Moves a server handle.
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
