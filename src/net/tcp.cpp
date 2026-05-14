#include <nexus/net/tcp.h>

#include <array>
#include <exception>
#include <string>
#include <utility>

#include <asio.hpp>

namespace nexus::net {

namespace detail {

class TcpClientStorage {
public:
    asio::io_context io;
    asio::ip::tcp::socket socket{io};
};

} // namespace detail

namespace {

Status asio_status(const std::exception& error) {
    return Status(StatusCode::kUnavailable, error.what());
}

Status validate_endpoint(const TcpEndpoint& endpoint) {
    if (endpoint.host.empty()) {
        return Status::invalid_argument("TCP endpoint host must not be empty");
    }

    if (endpoint.port == 0) {
        return Status::invalid_argument("TCP endpoint port must not be zero");
    }

    return Status::ok_status();
}

std::unique_ptr<detail::TcpClientStorage> make_storage() {
    return std::make_unique<detail::TcpClientStorage>();
}

} // namespace

TcpClient::TcpClient(TcpClient&& other) noexcept = default;

TcpClient& TcpClient::operator=(TcpClient&& other) noexcept = default;

TcpClient::~TcpClient() = default;

Result<TcpClient> TcpClient::connect(const TcpEndpoint& endpoint) {
    const auto status = validate_endpoint(endpoint);
    if (!status.ok()) {
        return status;
    }

    try {
        auto storage = make_storage();
        asio::ip::tcp::resolver resolver(storage->io);
        const auto endpoints = resolver.resolve(endpoint.host, std::to_string(endpoint.port));
        asio::connect(storage->socket, endpoints);
        return TcpClient(std::move(storage));
    } catch (const std::exception& error) {
        return asio_status(error);
    }
}

bool TcpClient::is_open() const {
    return storage_ && storage_->socket.is_open();
}

Status TcpClient::write_all(std::string_view data) {
    if (!is_open()) {
        return Status(StatusCode::kFailedPrecondition, "TCP client is not open");
    }

    try {
        asio::write(storage_->socket, asio::buffer(data.data(), data.size()));
        return Status::ok_status();
    } catch (const std::exception& error) {
        return asio_status(error);
    }
}

Result<std::string> TcpClient::read_some(std::size_t max_bytes) {
    if (!is_open()) {
        return Status(StatusCode::kFailedPrecondition, "TCP client is not open");
    }

    if (max_bytes == 0) {
        return Status::invalid_argument("TCP read size must be greater than zero");
    }

    try {
        std::string buffer(max_bytes, '\0');
        const auto bytes_read = storage_->socket.read_some(asio::buffer(buffer.data(), buffer.size()));
        buffer.resize(bytes_read);
        return buffer;
    } catch (const std::exception& error) {
        return asio_status(error);
    }
}

Status TcpClient::close() {
    if (!storage_) {
        return Status::ok_status();
    }

    try {
        asio::error_code ignored;
        storage_->socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
        storage_->socket.close(ignored);
        return Status::ok_status();
    } catch (const std::exception& error) {
        return asio_status(error);
    }
}

TcpClient::TcpClient(std::unique_ptr<detail::TcpClientStorage> storage)
    : storage_(std::move(storage)) {}

} // namespace nexus::net
