#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/net/export.h>
#include <nexus/net/tcp.h>

/// TCP listener that accepts incoming connections, backed by Asio.
namespace nexus::net {

namespace detail {
class TcpListenerStorage;
}

/// Options for `TcpListener::create`.
struct TcpListenOptions {
    /// Whether to set SO_REUSEADDR on the listening socket.
    bool reuse_address = true;
    /// Maximum length of the backlog of pending connections.
    int backlog = 0; // 0 uses the OS default
};

/// Move-only RAII TCP listener.
///
/// Created via `create(endpoint)`.  Binds to the given endpoint and starts a
/// background worker.  Use `async_accept` to accept incoming connections.
///
/// Copy is deleted; move transfers ownership.  A moved-from listener is closed
/// and `is_listening()` returns false.
class NEXUS_NET_API TcpListener {
public:
    /// Callback type for accepted connections.
    ///
    /// Invoked exactly once on the listener worker thread for each
    /// `async_accept` call. Receives a connected `TcpClient` on success or an
    /// error Result when the listener is closed or the accept operation fails.
    using AcceptHandler = std::function<void(Result<TcpClient>)>;

    /// Creates a listener bound to `endpoint`.
    ///
    /// @return A listening socket on success.
    /// @retval kInvalidArgument when the endpoint is empty or port is zero.
    /// @retval kUnavailable when bind fails (port in use, permission denied).
    static Result<TcpListener> create(
        const TcpEndpoint& endpoint,
        const TcpListenOptions& options = {});

    /// TCP listeners are move-only and cannot be copied.
    TcpListener(const TcpListener&) = delete;
    /// TCP listeners are move-only and cannot be copy-assigned.
    TcpListener& operator=(const TcpListener&) = delete;
    /// Moves a listener handle.
    TcpListener(TcpListener&& other) noexcept;
    /// Moves a listener handle.
    TcpListener& operator=(TcpListener&& other) noexcept;
    /// Stops the worker and closes the acceptor.
    ~TcpListener();

    /// True while the listener is accepting connections.
    bool is_listening() const;

    /// Returns the port the listener is bound to.
    ///
    /// When created with port 0, this returns the OS-assigned port.
    /// Returns 0 when the listener has been moved-from or closed.
    std::uint16_t port() const;

    /// Asynchronously accepts the next incoming connection.
    ///
    /// The handler receives a connected `TcpClient` on success.
    /// Call `async_accept` again from the handler to accept the next
    /// connection.
    ///
    /// @retval kFailedPrecondition when the listener is closed.
    void async_accept(AcceptHandler handler);

    /// Closes the listener.  Idempotent.
    Status close();

private:
    explicit TcpListener(std::shared_ptr<detail::TcpListenerStorage> storage);

    static void do_accept(
        std::shared_ptr<detail::TcpListenerStorage> storage,
        AcceptHandler handler);

    std::shared_ptr<detail::TcpListenerStorage> storage_;
};

} // namespace nexus::net
