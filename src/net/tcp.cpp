#include <nexus/net/tcp.h>

#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <asio.hpp>

#include <nexus/common/thread.h>

#include "tcp_internal.h"

namespace nexus::net {

namespace {

Status validate_endpoint(const TcpEndpoint& endpoint) {
    if (endpoint.host.empty()) {
        return Status::invalid_argument("TCP endpoint host must not be empty");
    }
    if (endpoint.port == 0) {
        return Status::invalid_argument("TCP endpoint port must not be zero");
    }
    return Status::ok_status();
}

void post_connect(
    std::shared_ptr<detail::TcpClientStorage> storage,
    const TcpEndpoint& ep,
    const TcpConnectOptions& options,
    TcpClient::ConnectHandler handler) {
    std::weak_ptr<detail::TcpClientStorage> weak = storage;
    storage->io.post([weak, ep, timeout = options.timeout,
                      handler = std::move(handler)]() mutable {
        auto st = weak.lock();
        if (!st || st->stopped.load()) {
            handler(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
            return;
        }

        if (ep.host.empty() || ep.port == 0) {
            handler(Status(StatusCode::kInvalidArgument, "TCP endpoint is empty or port is zero"));
            return;
        }

        auto resolver = std::make_shared<asio::ip::tcp::resolver>(st->io);

        if (timeout.count() > 0) {
            auto timer = std::make_shared<asio::steady_timer>(st->io, timeout);
            timer->async_wait([weak, timer, resolver](const asio::error_code& ec) {
                if (!ec) {
                    resolver->cancel();
                    auto st2 = weak.lock();
                    if (st2) {
                        st2->socket.close();
                    }
                }
            });

            resolver->async_resolve(
                ep.host, std::to_string(ep.port),
                [weak, timer, handler = std::move(handler)](
                    const asio::error_code& ec,
                    asio::ip::tcp::resolver::results_type endpoints) mutable {
                    auto st2 = weak.lock();
                    if (!st2 || st2->stopped.load()) {
                        handler(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
                        return;
                    }
                    if (ec) {
                        if (ec == asio::error::operation_aborted) {
                            handler(Status(StatusCode::kUnavailable, "TCP connect timed out"));
                        } else {
                            handler(Status(StatusCode::kUnavailable, ec.message()));
                        }
                        return;
                    }

                    asio::async_connect(st2->socket, endpoints,
                        [weak, timer, handler = std::move(handler)](
                            const asio::error_code& ec2,
                            const asio::ip::tcp::endpoint& /*unused*/) mutable {
                            timer->cancel();
                            auto st3 = weak.lock();
                            if (!st3 || st3->stopped.load()) {
                                handler(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
                                return;
                            }
                            if (ec2) {
                                if (ec2 == asio::error::operation_aborted) {
                                    handler(Status(StatusCode::kUnavailable, "TCP connect timed out"));
                                } else {
                                    handler(Status(StatusCode::kUnavailable, ec2.message()));
                                }
                                return;
                            }
                            st3->open.store(true);
                            handler(Status::ok_status());
                        });
                });
        } else {
            resolver->async_resolve(
                ep.host, std::to_string(ep.port),
                [weak, resolver, handler = std::move(handler)](
                    const asio::error_code& ec,
                    asio::ip::tcp::resolver::results_type endpoints) mutable {
                    auto st2 = weak.lock();
                    if (!st2 || st2->stopped.load()) {
                        handler(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
                        return;
                    }
                    if (ec) {
                        handler(Status(StatusCode::kUnavailable, ec.message()));
                        return;
                    }

                    asio::async_connect(st2->socket, endpoints,
                        [weak, handler = std::move(handler)](
                            const asio::error_code& ec2,
                            const asio::ip::tcp::endpoint& /*unused*/) mutable {
                            auto st3 = weak.lock();
                            if (!st3 || st3->stopped.load()) {
                                handler(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
                                return;
                            }
                            if (ec2) {
                                handler(Status(StatusCode::kUnavailable, ec2.message()));
                                return;
                            }
                            st3->open.store(true);
                            handler(Status::ok_status());
                        });
                });
        }
    });
}

void post_write(
    std::shared_ptr<detail::TcpClientStorage> storage,
    std::string_view data,
    const TcpIoOptions& options,
    TcpClient::WriteHandler handler) {
    std::weak_ptr<detail::TcpClientStorage> weak = storage;
    auto buffer = std::make_shared<std::vector<char>>(data.begin(), data.end());
    storage->io.post([weak, buffer, timeout = options.timeout,
                      handler = std::move(handler)]() mutable {
        auto st = weak.lock();
        if (!st || st->stopped.load()) {
            handler(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
            return;
        }
        if (!st->open.load()) {
            handler(Status(StatusCode::kFailedPrecondition, "TCP client is not connected"));
            return;
        }

        if (timeout.count() > 0) {
            auto timer = std::make_shared<asio::steady_timer>(st->io, timeout);
            timer->async_wait([weak, timer](const asio::error_code& ec) {
                if (!ec) {
                    auto st2 = weak.lock();
                    if (st2) {
                        st2->socket.close();
                    }
                }
            });

            asio::async_write(st->socket, asio::buffer(*buffer),
                [weak, buffer, timer, handler = std::move(handler)](
                    const asio::error_code& ec, std::size_t /*unused*/) mutable {
                    timer->cancel();
                    auto st2 = weak.lock();
                    if (!st2 || st2->stopped.load()) {
                        handler(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
                        return;
                    }
                    if (ec) {
                        if (ec == asio::error::operation_aborted) {
                            handler(Status(StatusCode::kUnavailable, "TCP write timed out"));
                        } else {
                            handler(Status(StatusCode::kUnavailable, ec.message()));
                        }
                        return;
                    }
                    handler(Status::ok_status());
                });
        } else {
            asio::async_write(st->socket, asio::buffer(*buffer),
                [weak, buffer, handler = std::move(handler)](
                    const asio::error_code& ec, std::size_t /*unused*/) mutable {
                    auto st2 = weak.lock();
                    if (!st2 || st2->stopped.load()) {
                        handler(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
                        return;
                    }
                    if (ec) {
                        handler(Status(StatusCode::kUnavailable, ec.message()));
                        return;
                    }
                    handler(Status::ok_status());
                });
        }
    });
}

void post_read(
    std::shared_ptr<detail::TcpClientStorage> storage,
    std::size_t max_bytes,
    const TcpIoOptions& options,
    TcpClient::ReadHandler handler) {
    std::weak_ptr<detail::TcpClientStorage> weak = storage;
    storage->io.post([weak, max_bytes, timeout = options.timeout,
                      handler = std::move(handler)]() mutable {
        auto st = weak.lock();
        if (!st || st->stopped.load()) {
            handler(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
            return;
        }
        if (!st->open.load()) {
            handler(Status(StatusCode::kFailedPrecondition, "TCP client is not connected"));
            return;
        }

        auto buf = std::make_shared<std::vector<char>>(max_bytes);

        if (timeout.count() > 0) {
            auto timer = std::make_shared<asio::steady_timer>(st->io, timeout);
            timer->async_wait([weak, timer](const asio::error_code& ec) {
                if (!ec) {
                    auto st2 = weak.lock();
                    if (st2) {
                        st2->socket.close();
                    }
                }
            });

            st->socket.async_read_some(asio::buffer(*buf),
                [weak, timer, buf, handler = std::move(handler)](
                    const asio::error_code& ec, std::size_t len) mutable {
                    timer->cancel();
                    auto st2 = weak.lock();
                    if (!st2 || st2->stopped.load()) {
                        handler(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
                        return;
                    }
                    if (ec) {
                        if (ec == asio::error::operation_aborted) {
                            handler(Status(StatusCode::kUnavailable, "TCP read timed out"));
                        } else if (ec == asio::error::eof) {
                            handler(std::string());
                        } else {
                            handler(Status(StatusCode::kUnavailable, ec.message()));
                        }
                        return;
                    }
                    handler(std::string(buf->data(), len));
                });
        } else {
            st->socket.async_read_some(asio::buffer(*buf),
                [weak, buf, handler = std::move(handler)](
                    const asio::error_code& ec, std::size_t len) mutable {
                    auto st2 = weak.lock();
                    if (!st2 || st2->stopped.load()) {
                        handler(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
                        return;
                    }
                    if (ec) {
                        if (ec == asio::error::eof) {
                            handler(std::string());
                        } else {
                            handler(Status(StatusCode::kUnavailable, ec.message()));
                        }
                        return;
                    }
                    handler(std::string(buf->data(), len));
                });
        }
    });
}

} // namespace

Result<TcpClient> TcpClient::create() {
    auto storage = std::make_shared<detail::TcpClientStorage>();
    auto* raw = storage.get();
    if (!raw->worker.start([raw] { raw->io.run(); })) {
        return Status(StatusCode::kInternal, "TCP worker thread failed to start");
    }
    return TcpClient(std::move(storage));
}

Result<TcpClient> TcpClient::connect(const TcpEndpoint& endpoint) {
    return connect_impl(endpoint, TcpConnectOptions{});
}

Result<TcpClient> TcpClient::connect(
    const TcpEndpoint& endpoint,
    const TcpConnectOptions& options) {
    return connect_impl(endpoint, options);
}

Result<TcpClient> TcpClient::connect_impl(
    const TcpEndpoint& endpoint,
    const TcpConnectOptions& options) {
    const auto status = validate_endpoint(endpoint);
    if (!status.ok()) {
        return status;
    }

    auto storage = std::make_shared<detail::TcpClientStorage>();
    auto* raw = storage.get();
    if (!raw->worker.start([raw] { raw->io.run(); })) {
        return Status(StatusCode::kInternal, "TCP worker thread failed to start");
    }

    nexus::common::Event done;
    Status connect_result;

    post_connect(storage, endpoint, options, [&](Status s) {
        connect_result = s;
        done.emit();
    });

    done.wait();

    if (!connect_result.ok()) {
        raw->stopped.store(true);
        raw->io.stop();
        raw->work.reset();
        raw->worker.stop();
        return connect_result;
    }

    return TcpClient(std::move(storage));
}

TcpClient::TcpClient(std::shared_ptr<detail::TcpClientStorage> storage)
    : storage_(std::move(storage)) {}

TcpClient::TcpClient(TcpClient&& other) noexcept = default;

TcpClient& TcpClient::operator=(TcpClient&& other) noexcept = default;

TcpClient::~TcpClient() {
    if (storage_) {
        storage_->stopped.store(true);
        storage_->socket.close();
        storage_->work.reset();
        storage_->worker.stop();
    }
}

bool TcpClient::is_open() const {
    return storage_ && storage_->open.load() && !storage_->stopped.load();
}

void TcpClient::async_connect(const TcpEndpoint& ep, ConnectHandler h) {
    async_connect(ep, TcpConnectOptions{}, std::move(h));
}

void TcpClient::async_connect(const TcpEndpoint& ep,
                              const TcpConnectOptions& options,
                              ConnectHandler h) {
    if (!storage_ || storage_->stopped.load()) {
        h(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
        return;
    }
    if (ep.host.empty() || ep.port == 0) {
        h(Status(StatusCode::kInvalidArgument, "TCP endpoint is empty or port is zero"));
        return;
    }
    post_connect(storage_, ep, options, std::move(h));
}

Status TcpClient::write_all(std::string_view data) {
    return write_all(data, TcpIoOptions{});
}

Status TcpClient::write_all(std::string_view data, const TcpIoOptions& options) {
    if (!storage_ || storage_->stopped.load()) {
        return Status(StatusCode::kFailedPrecondition, "TCP client is closed");
    }
    if (!storage_->open.load()) {
        return Status(StatusCode::kFailedPrecondition, "TCP client is not connected");
    }

    nexus::common::Event done;
    Status result;
    async_write_all(data, options, [&](Status s) {
        result = s;
        done.emit();
    });
    done.wait();
    return result;
}

Result<std::string> TcpClient::read_some(std::size_t max_bytes) {
    return read_some(max_bytes, TcpIoOptions{});
}

Result<std::string> TcpClient::read_some(std::size_t max_bytes, const TcpIoOptions& options) {
    if (!storage_ || storage_->stopped.load()) {
        return Status(StatusCode::kFailedPrecondition, "TCP client is closed");
    }
    if (!storage_->open.load()) {
        return Status(StatusCode::kFailedPrecondition, "TCP client is not connected");
    }
    if (max_bytes == 0) {
        return Status::invalid_argument("TCP read size must be greater than zero");
    }

    nexus::common::Event done;
    Result<std::string> result = std::string("");
    async_read_some(max_bytes, options, [&](Result<std::string> r) {
        result = std::move(r);
        done.emit();
    });
    done.wait();
    return result;
}

void TcpClient::async_write_all(std::string_view data, WriteHandler h) {
    async_write_all(data, TcpIoOptions{}, std::move(h));
}

void TcpClient::async_write_all(std::string_view data,
                                const TcpIoOptions& options,
                                WriteHandler h) {
    if (!storage_ || storage_->stopped.load()) {
        h(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
        return;
    }
    if (!storage_->open.load()) {
        h(Status(StatusCode::kFailedPrecondition, "TCP client is not connected"));
        return;
    }
    post_write(storage_, data, options, std::move(h));
}

void TcpClient::async_read_some(std::size_t max_bytes, ReadHandler h) {
    async_read_some(max_bytes, TcpIoOptions{}, std::move(h));
}

void TcpClient::async_read_some(std::size_t max_bytes,
                                const TcpIoOptions& options,
                                ReadHandler h) {
    if (!storage_ || storage_->stopped.load()) {
        h(Status(StatusCode::kFailedPrecondition, "TCP client is closed"));
        return;
    }
    if (!storage_->open.load()) {
        h(Status(StatusCode::kFailedPrecondition, "TCP client is not connected"));
        return;
    }
    post_read(storage_, max_bytes, options, std::move(h));
}

Status TcpClient::close() {
    if (!storage_ || storage_->stopped.load()) {
        return Status::ok_status();
    }

    storage_->stopped.store(true);
    storage_->open.store(false);

    nexus::common::Event done;
    Status result;

    std::weak_ptr<detail::TcpClientStorage> weak = storage_;
    storage_->io.post([weak, &done, &result]() {
        auto st = weak.lock();
        if (st) {
            st->socket.close();
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

void TcpClient::async_close(CloseHandler h) {
    if (!storage_ || storage_->stopped.load()) {
        if (h) h(Status::ok_status());
        return;
    }

    storage_->stopped.store(true);
    storage_->open.store(false);

    std::weak_ptr<detail::TcpClientStorage> weak = storage_;
    storage_->io.post([weak, handler = std::move(h)]() mutable {
        auto st = weak.lock();
        if (st) {
            st->socket.close();
            st->io.stop();
            st->work.reset();
        }
        if (handler) handler(Status::ok_status());
    });
}

} // namespace nexus::net
