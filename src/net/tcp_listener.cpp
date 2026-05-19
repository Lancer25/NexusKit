#include <nexus/net/tcp_listener.h>

#include <atomic>
#include <memory>
#include <string>
#include <utility>

#include <asio.hpp>

#include <nexus/common/thread.h>

#include "tcp_internal.h"

// asio::basic_socket::release() is deprecated before Windows 8.1.
// We target Windows 10+, where the function works correctly.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

namespace nexus::net {

namespace detail {

class TcpListenerStorage {
public:
    asio::io_context io;
    asio::ip::tcp::acceptor acceptor;
    asio::executor_work_guard<asio::io_context::executor_type> work;
    nexus::common::Thread worker;
    std::atomic<bool> listening{false};
    std::atomic<bool> stopped{false};
    std::uint16_t port = 0;

    TcpListenerStorage()
        : acceptor(io), work(asio::make_work_guard(io)) {}
};

} // namespace detail

void TcpListener::do_accept(
    std::shared_ptr<detail::TcpListenerStorage> storage,
    AcceptHandler handler) {
    std::weak_ptr<detail::TcpListenerStorage> weak = storage;
    auto socket = std::make_shared<asio::ip::tcp::socket>(storage->io);

    storage->acceptor.async_accept(*socket,
        [weak, socket, handler = std::move(handler)](
            const asio::error_code& ec) mutable {
            auto st = weak.lock();
            if (!st || st->stopped.load()) {
                handler(Status(StatusCode::kFailedPrecondition, "Listener is closed"));
                return;
            }
            if (ec) {
                if (ec == asio::error::operation_aborted) {
                    handler(Status(StatusCode::kFailedPrecondition, "Listener is closed"));
                } else {
                    handler(Status(StatusCode::kUnavailable, ec.message()));
                }
                return;
            }

            auto native = socket->release();
            auto client_result = TcpClient::create();
            if (!client_result.ok()) {
                handler(client_result.status());
                return;
            }

            auto client = std::move(client_result).value();
            asio::error_code assign_ec;
            client.storage_->socket.assign(asio::ip::tcp::v4(), native, assign_ec);
            if (assign_ec) {
                handler(Status(StatusCode::kInternal, assign_ec.message()));
                return;
            }
            client.storage_->open.store(true);
            handler(std::move(client));
        });
}

Result<TcpListener> TcpListener::create(
    const TcpEndpoint& endpoint,
    const TcpListenOptions& options) {
    if (endpoint.host.empty()) {
        return Status::invalid_argument("TCP endpoint host must not be empty");
    }

    auto storage = std::make_shared<detail::TcpListenerStorage>();
    auto* raw = storage.get();

    asio::error_code ec;
    asio::ip::tcp::endpoint asio_ep(
        asio::ip::make_address(endpoint.host, ec),
        endpoint.port);

    if (ec) {
        asio::ip::tcp::resolver resolver(raw->io);
        auto results = resolver.resolve(endpoint.host,
            std::to_string(endpoint.port), ec);
        if (ec) {
            return Status(StatusCode::kUnavailable, ec.message());
        }
        asio_ep = results->endpoint();
    }

    raw->acceptor.open(asio_ep.protocol(), ec);
    if (ec) {
        return Status(StatusCode::kUnavailable, ec.message());
    }

    if (options.reuse_address) {
        raw->acceptor.set_option(
            asio::ip::tcp::acceptor::reuse_address(true), ec);
        if (ec) {
            return Status(StatusCode::kUnavailable, ec.message());
        }
    }

    raw->acceptor.bind(asio_ep, ec);
    if (ec) {
        return Status(StatusCode::kUnavailable, ec.message());
    }

    raw->acceptor.listen(
        options.backlog > 0 ? options.backlog
                            : asio::socket_base::max_listen_connections,
        ec);
    if (ec) {
        return Status(StatusCode::kUnavailable, ec.message());
    }

    raw->port = raw->acceptor.local_endpoint().port();

    if (!raw->worker.start([raw] { raw->io.run(); })) {
        return Status(StatusCode::kInternal,
            "TCP listener worker thread failed to start");
    }

    raw->listening.store(true);
    return TcpListener(std::move(storage));
}

TcpListener::TcpListener(std::shared_ptr<detail::TcpListenerStorage> storage)
    : storage_(std::move(storage)) {}

TcpListener::TcpListener(TcpListener&& other) noexcept = default;

TcpListener& TcpListener::operator=(TcpListener&& other) noexcept = default;

TcpListener::~TcpListener() {
    close();
}

bool TcpListener::is_listening() const {
    return storage_ && storage_->listening.load() && !storage_->stopped.load();
}

std::uint16_t TcpListener::port() const {
    if (!storage_) return 0;
    return storage_->port;
}

void TcpListener::async_accept(AcceptHandler handler) {
    if (!storage_ || storage_->stopped.load()) {
        handler(Status(StatusCode::kFailedPrecondition, "Listener is closed"));
        return;
    }
    do_accept(storage_, std::move(handler));
}

Status TcpListener::close() {
    if (!storage_ || storage_->stopped.load()) {
        return Status::ok_status();
    }

    storage_->stopped.store(true);
    storage_->listening.store(false);

    nexus::common::Event done;
    Status result;

    std::weak_ptr<detail::TcpListenerStorage> weak = storage_;
    storage_->io.post([weak, &done, &result]() {
        auto st = weak.lock();
        if (st) {
            st->acceptor.close();
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

} // namespace nexus::net

#ifdef _MSC_VER
#pragma warning(pop)
#endif
