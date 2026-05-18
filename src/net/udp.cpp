#include <nexus/net/udp.h>

#include <atomic>
#include <memory>
#include <string>
#include <utility>

#include <asio.hpp>

#include <nexus/common/thread.h>

namespace nexus::net {

namespace detail {

class UdpSocketStorage {
public:
    asio::io_context io;
    asio::executor_work_guard<asio::io_context::executor_type> work;
    nexus::common::Thread worker;
    asio::ip::udp::socket socket;
    std::atomic<bool> open{false};
    std::atomic<bool> stopped{false};
    std::uint16_t local_port = 0;

    UdpSocketStorage()
        : work(asio::make_work_guard(io)), socket(io) {}
};

} // namespace detail

namespace {

Status validate_bind_endpoint(const UdpEndpoint& endpoint) {
    if (endpoint.host.empty()) {
        return Status::invalid_argument("UDP endpoint host must not be empty");
    }
    return Status::ok_status();
}

Status validate_remote_endpoint(const UdpEndpoint& endpoint) {
    if (endpoint.host.empty()) {
        return Status::invalid_argument("UDP remote host must not be empty");
    }
    if (endpoint.port == 0) {
        return Status::invalid_argument("UDP remote port must not be zero");
    }
    return Status::ok_status();
}

void post_send(std::shared_ptr<detail::UdpSocketStorage> storage,
               std::string_view data,
               const UdpEndpoint& remote,
               UdpSocket::SendHandler handler) {
    if (!storage || storage->stopped.load()) {
        handler(Status(StatusCode::kFailedPrecondition, "UDP socket is closed"));
        return;
    }
    if (!storage->open.load()) {
        handler(Status(StatusCode::kFailedPrecondition, "UDP socket is not bound"));
        return;
    }
    if (remote.host.empty() || remote.port == 0) {
        handler(Status(StatusCode::kInvalidArgument, "UDP remote endpoint is empty or port is zero"));
        return;
    }

    std::weak_ptr<detail::UdpSocketStorage> weak = storage;
    auto buffer = std::make_shared<std::string>(data);
    storage->io.post([weak, buffer, remote, handler = std::move(handler)]() mutable {
        auto st = weak.lock();
        if (!st || st->stopped.load()) {
            handler(Status(StatusCode::kFailedPrecondition, "UDP socket is closed"));
            return;
        }

        asio::ip::udp::endpoint asio_remote(
            asio::ip::make_address(remote.host), remote.port);

        st->socket.async_send_to(asio::buffer(*buffer), asio_remote,
            [weak, buffer, handler = std::move(handler)](
                const asio::error_code& ec, std::size_t bytes_sent) mutable {
                auto st2 = weak.lock();
                if (!st2 || st2->stopped.load()) {
                    handler(Status(StatusCode::kFailedPrecondition, "UDP socket is closed"));
                    return;
                }
                if (ec) {
                    handler(Status(StatusCode::kUnavailable, ec.message()));
                    return;
                }
                handler(static_cast<std::size_t>(bytes_sent));
            });
    });
}

void post_receive(std::shared_ptr<detail::UdpSocketStorage> storage,
                  std::size_t max_bytes,
                  const UdpReceiveOptions& options,
                  UdpSocket::ReceiveHandler handler) {
    if (!storage || storage->stopped.load()) {
        handler(Status(StatusCode::kFailedPrecondition, "UDP socket is closed"));
        return;
    }
    if (!storage->open.load()) {
        handler(Status(StatusCode::kFailedPrecondition, "UDP socket is not bound"));
        return;
    }

    std::weak_ptr<detail::UdpSocketStorage> weak = storage;
    storage->io.post([weak, max_bytes, timeout = options.timeout,
                      handler = std::move(handler)]() mutable {
        auto st = weak.lock();
        if (!st || st->stopped.load()) {
            handler(Status(StatusCode::kFailedPrecondition, "UDP socket is closed"));
            return;
        }

        auto buf = std::make_shared<std::vector<char>>(max_bytes);
        auto remote = std::make_shared<asio::ip::udp::endpoint>();

        if (timeout.count() > 0) {
            auto timer = std::make_shared<asio::steady_timer>(st->io, timeout);
            timer->async_wait([weak, timer](const asio::error_code& ec) {
                if (!ec) {
                    auto st2 = weak.lock();
                    if (st2) st2->socket.close();
                }
            });

            st->socket.async_receive_from(asio::buffer(*buf), *remote,
                [weak, timer, buf, remote, handler = std::move(handler)](
                    const asio::error_code& ec, std::size_t len) mutable {
                timer->cancel();
                auto st2 = weak.lock();
                if (!st2 || st2->stopped.load()) {
                    handler(Status(StatusCode::kFailedPrecondition, "UDP socket is closed"));
                    return;
                }
                if (ec) {
                    if (ec == asio::error::operation_aborted) {
                        handler(Status(StatusCode::kUnavailable, "UDP receive timed out"));
                    } else {
                        handler(Status(StatusCode::kUnavailable, ec.message()));
                    }
                    return;
                }
                UdpDatagram dg;
                dg.data = std::string(buf->data(), len);
                dg.remote.host = remote->address().to_string();
                dg.remote.port = remote->port();
                handler(std::move(dg));
            });
        } else {
            st->socket.async_receive_from(asio::buffer(*buf), *remote,
                [weak, buf, remote, handler = std::move(handler)](
                    const asio::error_code& ec, std::size_t len) mutable {
                auto st2 = weak.lock();
                if (!st2 || st2->stopped.load()) {
                    handler(Status(StatusCode::kFailedPrecondition, "UDP socket is closed"));
                    return;
                }
                if (ec) {
                    handler(Status(StatusCode::kUnavailable, ec.message()));
                    return;
                }
                UdpDatagram dg;
                dg.data = std::string(buf->data(), len);
                dg.remote.host = remote->address().to_string();
                dg.remote.port = remote->port();
                handler(std::move(dg));
            });
        }
    });
}

} // namespace

// --- construct / destruct ---

Result<UdpSocket> UdpSocket::create() {
    auto storage = std::make_shared<detail::UdpSocketStorage>();
    auto* raw = storage.get();
    if (!raw->worker.start([raw] { raw->io.run(); })) {
        return Status(StatusCode::kInternal,
                      "UDP socket worker thread failed to start");
    }
    return UdpSocket(std::move(storage));
}

Result<UdpSocket> UdpSocket::bind(const UdpEndpoint& local) {
    const auto status = validate_bind_endpoint(local);
    if (!status.ok()) return status;

    auto storage = std::make_shared<detail::UdpSocketStorage>();
    auto* raw = storage.get();
    if (!raw->worker.start([raw] { raw->io.run(); })) {
        return Status(StatusCode::kInternal,
                      "UDP socket worker thread failed to start");
    }

    nexus::common::Event done;
    Status bind_status(StatusCode::kInternal, "unset");

    std::weak_ptr<detail::UdpSocketStorage> weak = storage;
    storage->io.post([weak, local, &done, &bind_status]() mutable {
        auto st = weak.lock();
        if (!st) {
            bind_status = Status(StatusCode::kFailedPrecondition, "UDP socket is closed");
            done.emit();
            return;
        }

        asio::ip::udp::endpoint asio_ep;
        if (local.host == "0.0.0.0") {
            asio_ep = asio::ip::udp::endpoint(asio::ip::udp::v4(), local.port);
        } else {
            asio::ip::udp::resolver resolver(st->io);
            asio::error_code resolve_ec;
            auto endpoints = resolver.resolve(
                asio::ip::udp::v4(), local.host,
                std::to_string(local.port), resolve_ec);
            if (resolve_ec) {
                bind_status = Status(StatusCode::kUnavailable, resolve_ec.message());
                done.emit();
                return;
            }
            asio_ep = *endpoints.begin();
        }

        asio::error_code ec;
        st->socket.open(asio_ep.protocol(), ec);
        if (ec) {
            bind_status = Status(StatusCode::kUnavailable, ec.message());
            done.emit();
            return;
        }
        st->socket.bind(asio_ep, ec);
        if (ec) {
            bind_status = Status(StatusCode::kUnavailable, ec.message());
            done.emit();
            return;
        }
        st->local_port = st->socket.local_endpoint().port();
        st->open.store(true);
        bind_status = Status::ok_status();
        done.emit();
    });

    if (!done.wait(5000)) {
        storage->work.reset();
        storage->io.stop();
        storage->worker.stop();
        return Status(StatusCode::kUnavailable, "UDP bind timed out");
    }

    if (!bind_status.ok()) {
        storage->work.reset();
        storage->io.stop();
        storage->worker.stop();
        return bind_status;
    }

    return UdpSocket(std::move(storage));
}

UdpSocket::UdpSocket(std::shared_ptr<detail::UdpSocketStorage> storage)
    : storage_(std::move(storage)) {}

UdpSocket::UdpSocket(UdpSocket&& other) noexcept = default;

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept {
    if (this != &other) {
        close();
        storage_ = std::move(other.storage_);
    }
    return *this;
}

UdpSocket::~UdpSocket() {
    close();
}

// --- state ---

bool UdpSocket::is_open() const {
    return storage_ && storage_->open.load() && !storage_->stopped.load();
}

std::uint16_t UdpSocket::local_port() const {
    if (!storage_ || !storage_->open.load()) return 0;
    return storage_->local_port;
}

// --- async bind ---

void UdpSocket::async_bind(const UdpEndpoint& local, BindHandler h) {
    if (!storage_ || storage_->stopped.load()) {
        if (h) h(Status(StatusCode::kFailedPrecondition, "UDP socket is closed"));
        return;
    }

    const auto status = validate_bind_endpoint(local);
    if (!status.ok()) {
        if (h) h(status);
        return;
    }

    std::weak_ptr<detail::UdpSocketStorage> weak = storage_;
    storage_->io.post([weak, local, h = std::move(h)]() mutable {
        auto st = weak.lock();
        if (!st || st->stopped.load()) {
            if (h) h(Status(StatusCode::kFailedPrecondition, "UDP socket is closed"));
            return;
        }

        asio::ip::udp::endpoint asio_ep;
        if (local.host == "0.0.0.0") {
            asio_ep = asio::ip::udp::endpoint(asio::ip::udp::v4(), local.port);
        } else {
            asio::ip::udp::resolver resolver(st->io);
            asio::error_code resolve_ec;
            auto endpoints = resolver.resolve(
                asio::ip::udp::v4(), local.host,
                std::to_string(local.port), resolve_ec);
            if (resolve_ec) {
                if (h) h(Status(StatusCode::kUnavailable, resolve_ec.message()));
                return;
            }
            asio_ep = *endpoints.begin();
        }

        asio::error_code ec;
        st->socket.open(asio_ep.protocol(), ec);
        if (ec) {
            if (h) h(Status(StatusCode::kUnavailable, ec.message()));
            return;
        }
        st->socket.bind(asio_ep, ec);
        if (ec) {
            if (h) h(Status(StatusCode::kUnavailable, ec.message()));
            return;
        }
        st->local_port = st->socket.local_endpoint().port();
        st->open.store(true);
        if (h) h(Status::ok_status());
    });
}

// --- sync send ---

Result<std::size_t> UdpSocket::send_to(std::string_view data, const UdpEndpoint& remote) {
    if (!is_open()) {
        return Status(StatusCode::kFailedPrecondition, "UDP socket is not open");
    }

    const auto status = validate_remote_endpoint(remote);
    if (!status.ok()) return status;

    nexus::common::Event done;
    Result<std::size_t> result(Status(StatusCode::kInternal, "unset"));

    async_send_to(data, remote, [&](Result<std::size_t> r) {
        result = std::move(r);
        done.emit();
    });

    done.wait(5000);

    if (!result.ok() && result.status().code() == StatusCode::kInternal) {
        return Status(StatusCode::kUnavailable, "UDP send_to timed out");
    }

    return result;
}

// --- async send ---

void UdpSocket::async_send_to(std::string_view data,
                               const UdpEndpoint& remote,
                               SendHandler h) {
    if (!h) return;
    post_send(storage_, data, remote, std::move(h));
}

// --- sync receive ---

Result<UdpDatagram> UdpSocket::receive_from(std::size_t max_bytes) {
    return receive_from(max_bytes, UdpReceiveOptions{});
}

Result<UdpDatagram> UdpSocket::receive_from(std::size_t max_bytes,
                                             const UdpReceiveOptions& options) {
    if (!is_open()) {
        return Status(StatusCode::kFailedPrecondition, "UDP socket is not open");
    }

    if (max_bytes == 0) {
        return Status::invalid_argument("UDP receive size must be greater than zero");
    }

    nexus::common::Event done;
    Result<UdpDatagram> result(Status(StatusCode::kInternal, "unset"));

    async_receive_from(max_bytes, options, [&](Result<UdpDatagram> r) {
        result = std::move(r);
        done.emit();
    });

    done.wait(5000);

    if (!result.ok() && result.status().code() == StatusCode::kInternal) {
        return Status(StatusCode::kUnavailable, "UDP receive_from timed out");
    }

    return result;
}

// --- async receive ---

void UdpSocket::async_receive_from(std::size_t max_bytes, ReceiveHandler h) {
    if (!h) return;
    post_receive(storage_, max_bytes, UdpReceiveOptions{}, std::move(h));
}

void UdpSocket::async_receive_from(std::size_t max_bytes,
                                    const UdpReceiveOptions& options,
                                    ReceiveHandler h) {
    if (!h) return;
    post_receive(storage_, max_bytes, options, std::move(h));
}

// --- sync close ---

Status UdpSocket::close() {
    if (!storage_ || storage_->stopped.load()) {
        return Status::ok_status();
    }

    storage_->stopped.store(true);
    storage_->open.store(false);

    nexus::common::Event done;
    Status result;

    std::weak_ptr<detail::UdpSocketStorage> weak = storage_;
    storage_->io.post([weak, &done, &result]() {
        auto st = weak.lock();
        if (st) {
            asio::error_code ignored;
            st->socket.close(ignored);
            st->io.stop();
            st->work.reset();
        }
        result = Status::ok_status();
        done.emit();
    });

    done.wait();
    storage_->worker.stop();
    return result;
}

// --- async close ---

void UdpSocket::async_close(CloseHandler h) {
    if (!storage_ || storage_->stopped.load()) {
        if (h) h(Status::ok_status());
        return;
    }

    storage_->stopped.store(true);
    storage_->open.store(false);

    std::weak_ptr<detail::UdpSocketStorage> weak = storage_;
    storage_->io.post([weak, h = std::move(h)]() mutable {
        auto st = weak.lock();
        if (st) {
            asio::error_code ignored;
            st->socket.close(ignored);
            st->work.reset();
        }
        if (h) h(Status::ok_status());
    });
}

} // namespace nexus::net
