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

/// TCP client socket backed by Asio.
///
/// Backend headers are private; users do not need to link against Asio.
namespace nexus::net {

namespace detail {
class TcpClientStorage;
}

class TcpListener;

/// A TCP endpoint (host and port).
struct TcpEndpoint {
    /// Hostname or IP address.
    std::string host;
    /// Port number.
    std::uint16_t port = 0;
};

/// Options for `TcpClient::connect`.
///
/// Set `timeout` to 0 (default) for no explicit timeout — the connect call
/// will block until the OS-level TCP handshake succeeds or fails.
struct TcpConnectOptions {
    /// Connection timeout.  0 means no explicit timeout.
    std::chrono::milliseconds timeout{0};
};

/// Options for TcpClient read and write operations.
///
/// Set `timeout` to 0 (default) for no explicit timeout — the I/O call
/// will block until data is transferred or the socket errors.
struct TcpIoOptions {
    /// I/O timeout.  0 means no explicit timeout.
    std::chrono::milliseconds timeout{0};
};

/// Move-only RAII TCP client with sync and async I/O.
///
/// Created via `create()` (starts worker, no connection) or
/// `connect(endpoint)` (creates, starts worker, connects synchronously).
/// Copy is deleted; move transfers ownership.  A moved-from client is closed
/// and `is_open()` returns false.
///
/// Sync methods block the caller until the operation completes.
/// Async methods post work to an internal `asio::io_context` and deliver
/// results via callbacks on the background worker thread.
class NEXUS_NET_API TcpClient {
public:
    using ConnectHandler = std::function<void(Status)>;
    using WriteHandler   = std::function<void(Status)>;
    using ReadHandler    = std::function<void(Result<std::string>)>;
    using CloseHandler   = std::function<void(Status)>;

    /// Creates a client with a running worker thread (no connection).
    ///
    /// Use `async_connect` to establish a connection asynchronously.
    ///
    /// @retval kInternal when the worker thread fails to start.
    static Result<TcpClient> create();

    /// Opens a TCP connection to the endpoint with no explicit timeout.
    ///
    /// May block until the connection is established or fails.
    ///
    /// @return An open client on success.
    /// @retval kInvalidArgument when the endpoint is empty or port is zero.
    /// @retval kUnavailable when the connection fails.
    static Result<TcpClient> connect(const TcpEndpoint& endpoint);

    /// Opens a TCP connection with an explicit timeout.
    ///
    /// When `options.timeout` is non-zero, the connect will fail with
    /// `kUnavailable` if the TCP handshake does not complete within the
    /// given duration.
    ///
    /// @return An open client on success.
    /// @retval kInvalidArgument when the endpoint is empty or port is zero.
    /// @retval kUnavailable when the connection fails or times out.
    static Result<TcpClient> connect(
        const TcpEndpoint& endpoint,
        const TcpConnectOptions& options);

    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;
    TcpClient(TcpClient&& other) noexcept;
    TcpClient& operator=(TcpClient&& other) noexcept;
    /// Stops the worker and closes the socket if open.
    ~TcpClient();

    /// True when the socket is connected.
    bool is_open() const;

    /// Asynchronously connects to `ep` with no explicit timeout.
    ///
    /// @retval kInvalidArgument when the endpoint is empty or port is zero.
    /// @retval kUnavailable when the connection fails.
    void async_connect(const TcpEndpoint& ep, ConnectHandler h);

    /// Asynchronously connects to `ep` with an explicit timeout.
    void async_connect(const TcpEndpoint& ep,
                       const TcpConnectOptions& options,
                       ConnectHandler h);

    /// Writes all bytes from `data` to the socket.
    ///
    /// May block.  Retries until all bytes are sent or the socket errors.
    ///
    /// @retval kFailedPrecondition when the client is closed.
    /// @retval kUnavailable on socket error.
    Status write_all(std::string_view data);

    /// Writes all bytes from `data` to the socket with an I/O timeout.
    Status write_all(std::string_view data, const TcpIoOptions& options);

    /// Reads up to `max_bytes` from the socket.
    ///
    /// May block until data arrives or the socket errors.
    ///
    /// @return The received bytes on success.  Returns an empty string when the
    /// remote side has closed gracefully.
    /// @retval kFailedPrecondition when the client is closed.
    /// @retval kUnavailable on socket error.
    Result<std::string> read_some(std::size_t max_bytes);

    /// Reads up to `max_bytes` from the socket with an I/O timeout.
    Result<std::string> read_some(std::size_t max_bytes, const TcpIoOptions& options);

    /// Writes all bytes from `data` to the socket with no explicit timeout.
    ///
    /// @retval kFailedPrecondition when the client is closed.
    /// @retval kUnavailable on socket error.
    void async_write_all(std::string_view data, WriteHandler h);

    /// Writes all bytes with an I/O timeout.
    void async_write_all(std::string_view data,
                         const TcpIoOptions& options,
                         WriteHandler h);

    /// Reads up to `max_bytes` from the socket with no explicit timeout.
    ///
    /// The handler receives an empty string when the remote side has
    /// closed gracefully.
    ///
    /// @retval kFailedPrecondition when the client is closed.
    /// @retval kUnavailable on socket error.
    void async_read_some(std::size_t max_bytes, ReadHandler h);

    /// Reads up to `max_bytes` with an I/O timeout.
    void async_read_some(std::size_t max_bytes,
                         const TcpIoOptions& options,
                         ReadHandler h);

    /// Closes the connection synchronously.  Idempotent.
    Status close();

    /// Closes the connection asynchronously.  Idempotent.
    ///
    /// The handler is always invoked (even if already closed).
    void async_close(CloseHandler h);

private:
    explicit TcpClient(std::shared_ptr<detail::TcpClientStorage> storage);

    static Result<TcpClient> connect_impl(
        const TcpEndpoint& endpoint,
        const TcpConnectOptions& options);

    friend class TcpListener;

    std::shared_ptr<detail::TcpClientStorage> storage_;
};

} // namespace nexus::net
