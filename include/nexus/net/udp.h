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

/// UDP socket backed by Asio.
///
/// Backend headers are private; users do not need to link against Asio.
namespace nexus::net {

namespace detail {
class UdpSocketStorage;
}

/// A UDP endpoint (host and port).
struct UdpEndpoint {
    /// Hostname or IP address.
    std::string host;
    /// Port number.
    std::uint16_t port = 0;
};

/// A received UDP datagram with sender metadata.
struct UdpDatagram {
    /// Datagram payload bytes.
    std::string data;
    /// Sender address.
    UdpEndpoint remote;
};

/// Options for UdpSocket receive operations.
struct UdpReceiveOptions {
    /// Receive timeout. 0 means no explicit timeout.
    std::chrono::milliseconds timeout{0};
};

/// Move-only RAII UDP socket with sync and async I/O.
///
/// Created via `bind(local)` (sync factory) or `create()` followed by
/// `async_bind(local, handler)`.  Copy is deleted; move transfers ownership.
/// A moved-from socket is closed and `is_open()` returns false.
///
/// Sync methods block. Async methods normally complete on the internal worker
/// thread, while validation or immediate precondition failures may invoke
/// callbacks synchronously before the async method returns.
class NEXUS_NET_API UdpSocket {
public:
    /// Completion callback for `async_bind`.
    ///
    /// Invoked once per async bind operation. Normal completion is delivered
    /// on the internal UDP worker thread; immediate closed or invalid-endpoint
    /// failures may be delivered synchronously before `async_bind` returns.
    /// Receives OK after the socket is bound, or an error Status for invalid
    /// endpoints, closed sockets, or bind failures.
    using BindHandler    = std::function<void(Status)>;
    /// Completion callback for `async_send_to`.
    ///
    /// Invoked once per async send operation when a non-empty handler is
    /// supplied. Immediate closed, not-bound, or invalid-remote failures may be
    /// delivered synchronously before `async_send_to` returns; normal send
    /// completion is delivered on the internal UDP worker thread. The datagram
    /// payload is copied before the call returns.
    using SendHandler    = std::function<void(Result<std::size_t>)>;
    /// Completion callback for `async_receive_from`.
    ///
    /// Invoked once per async receive operation when a non-empty handler is
    /// supplied. Immediate closed or not-bound failures may be delivered
    /// synchronously before `async_receive_from` returns; normal receive
    /// completion is delivered on the internal UDP worker thread. Successful
    /// datagrams contain copied payload bytes and sender endpoint metadata.
    using ReceiveHandler = std::function<void(Result<UdpDatagram>)>;
    /// Completion callback for `async_close`.
    ///
    /// Invoked at most once when a handler is supplied. If the socket is
    /// already closed, the handler may be invoked synchronously before
    /// `async_close` returns; otherwise it is posted to the internal UDP worker
    /// thread. Close is idempotent and currently reports OK.
    using CloseHandler   = std::function<void(Status)>;

    /// Creates a UDP socket with a running worker thread (no bind).
    ///
    /// Use `async_bind` to bind the socket asynchronously.
    ///
    /// @retval kInternal when the worker thread fails to start.
    static Result<UdpSocket> create();

    /// Binds a UDP socket to a local endpoint synchronously.
    ///
    /// @return A bound socket on success.
    /// @retval kInvalidArgument when the endpoint port is zero.
    /// @retval kUnavailable when binding fails.
    static Result<UdpSocket> bind(const UdpEndpoint& local);

    /// UDP sockets are move-only and cannot be copied.
    UdpSocket(const UdpSocket&) = delete;
    /// UDP sockets are move-only and cannot be copy-assigned.
    UdpSocket& operator=(const UdpSocket&) = delete;
    /// Moves a socket handle.
    UdpSocket(UdpSocket&& other) noexcept;
    /// Moves a socket handle.
    UdpSocket& operator=(UdpSocket&& other) noexcept;
    /// Closes the socket if open.
    ~UdpSocket();

    /// True when the socket is bound.
    bool is_open() const;

    /// The local port this socket is bound to, or 0 if not bound.
    std::uint16_t local_port() const;

    /// Asynchronously binds the socket to a local endpoint.
    ///
    /// @retval kInvalidArgument when the endpoint port is zero.
    /// @retval kUnavailable when binding fails.
    void async_bind(const UdpEndpoint& local, BindHandler h);

    /// Sends a datagram to a remote endpoint.  May block.
    ///
    /// @return The number of bytes sent on success.
    /// @retval kFailedPrecondition when the socket is closed.
    /// @retval kUnavailable on send error.
    Result<std::size_t> send_to(std::string_view data, const UdpEndpoint& remote);

    /// Asynchronously sends a datagram to a remote endpoint.
    ///
    /// Callers own data until the handler fires.
    void async_send_to(std::string_view data,
                       const UdpEndpoint& remote,
                       SendHandler h);

    /// Receives a datagram, up to `max_bytes`.  May block.
    ///
    /// @retval kFailedPrecondition when the socket is closed.
    /// @retval kUnavailable on receive error.
    Result<UdpDatagram> receive_from(std::size_t max_bytes);

    /// Receives a datagram with an explicit receive timeout.  May block.
    Result<UdpDatagram> receive_from(std::size_t max_bytes,
                                     const UdpReceiveOptions& options);

    /// Asynchronously receives up to `max_bytes` with no explicit timeout.
    void async_receive_from(std::size_t max_bytes, ReceiveHandler h);

    /// Asynchronously receives up to `max_bytes` with a receive timeout.
    void async_receive_from(std::size_t max_bytes,
                            const UdpReceiveOptions& options,
                            ReceiveHandler h);

    /// Closes the socket synchronously.  Idempotent.
    Status close();

    /// Closes the socket asynchronously.  Idempotent.
    void async_close(CloseHandler h);

private:
    explicit UdpSocket(std::shared_ptr<detail::UdpSocketStorage> storage);

    std::shared_ptr<detail::UdpSocketStorage> storage_;
};

} // namespace nexus::net
