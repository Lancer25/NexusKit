#include <nexus/net/udp.h>

#include <exception>
#include <string>
#include <utility>

#include <asio.hpp>
#include <nexus/log/logger.h>

namespace nexus::net {

namespace detail {

class UdpSocketStorage {
public:
    asio::io_context io;
    asio::ip::udp::socket socket{io};
};

} // namespace detail

namespace {

Status asio_status(const std::exception& error) {
    return Status(StatusCode::kUnavailable, error.what());
}

void diagnostic_log(log::Level level, const std::string& message) {
    log::write(level, message);
}

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

std::unique_ptr<detail::UdpSocketStorage> make_storage() {
    return std::make_unique<detail::UdpSocketStorage>();
}

asio::ip::udp::endpoint to_asio_endpoint(const UdpEndpoint& endpoint) {
    return asio::ip::udp::endpoint(asio::ip::make_address(endpoint.host), endpoint.port);
}

UdpEndpoint from_asio_endpoint(const asio::ip::udp::endpoint& endpoint) {
    return UdpEndpoint{endpoint.address().to_string(), endpoint.port()};
}

} // namespace

UdpSocket::UdpSocket(UdpSocket&& other) noexcept = default;

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept = default;

UdpSocket::~UdpSocket() = default;

Result<UdpSocket> UdpSocket::bind(const UdpEndpoint& local) {
    const auto status = validate_bind_endpoint(local);
    if (!status.ok()) {
        return status;
    }

    try {
        auto storage = make_storage();
        storage->socket.open(asio::ip::udp::v4());
        storage->socket.bind(to_asio_endpoint(local));
        diagnostic_log(log::Level::info, "UDP bound " + local.host + ":" + std::to_string(local.port));
        return UdpSocket(std::move(storage));
    } catch (const std::exception& error) {
        diagnostic_log(log::Level::warn, std::string("UDP bind failed: ") + error.what());
        return asio_status(error);
    }
}

bool UdpSocket::is_open() const {
    return storage_ && storage_->socket.is_open();
}

Result<std::size_t> UdpSocket::send_to(std::string_view data, const UdpEndpoint& remote) {
    if (!is_open()) {
        return Status(StatusCode::kFailedPrecondition, "UDP socket is not open");
    }

    const auto status = validate_remote_endpoint(remote);
    if (!status.ok()) {
        return status;
    }

    try {
        const auto endpoint = to_asio_endpoint(remote);
        const auto bytes_sent = storage_->socket.send_to(asio::buffer(data.data(), data.size()), endpoint);
        diagnostic_log(
            log::Level::debug,
            "UDP send_to " + remote.host + ":" + std::to_string(remote.port) +
                " bytes=" + std::to_string(bytes_sent));
        return bytes_sent;
    } catch (const std::exception& error) {
        diagnostic_log(log::Level::warn, std::string("UDP send_to failed: ") + error.what());
        return asio_status(error);
    }
}

Result<UdpDatagram> UdpSocket::receive_from(std::size_t max_bytes) {
    if (!is_open()) {
        return Status(StatusCode::kFailedPrecondition, "UDP socket is not open");
    }

    if (max_bytes == 0) {
        return Status::invalid_argument("UDP receive size must be greater than zero");
    }

    try {
        std::string buffer(max_bytes, '\0');
        asio::ip::udp::endpoint remote;
        const auto bytes_read = storage_->socket.receive_from(
            asio::buffer(buffer.data(), buffer.size()),
            remote);
        buffer.resize(bytes_read);
        const auto endpoint = from_asio_endpoint(remote);
        diagnostic_log(
            log::Level::debug,
            "UDP receive_from " + endpoint.host + ":" + std::to_string(endpoint.port) +
                " bytes=" + std::to_string(bytes_read));
        return UdpDatagram{std::move(buffer), endpoint};
    } catch (const std::exception& error) {
        diagnostic_log(log::Level::warn, std::string("UDP receive_from failed: ") + error.what());
        return asio_status(error);
    }
}

Status UdpSocket::close() {
    if (!storage_) {
        return Status::ok_status();
    }

    try {
        asio::error_code ignored;
        storage_->socket.close(ignored);
        diagnostic_log(log::Level::debug, "UDP close");
        return Status::ok_status();
    } catch (const std::exception& error) {
        diagnostic_log(log::Level::warn, std::string("UDP close failed: ") + error.what());
        return asio_status(error);
    }
}

UdpSocket::UdpSocket(std::unique_ptr<detail::UdpSocketStorage> storage)
    : storage_(std::move(storage)) {}

} // namespace nexus::net
