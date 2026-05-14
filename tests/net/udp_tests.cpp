#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#include <asio.hpp>

#include <nexus/core/status.h>
#include <nexus/net/udp.h>

namespace {

class LocalUdpEchoServer {
public:
    LocalUdpEchoServer()
        : socket_(io_, asio::ip::udp::endpoint(asio::ip::make_address("127.0.0.1"), 0)),
          port_(socket_.local_endpoint().port()) {
        thread_ = std::thread([this]() {
            run();
        });

        while (!ready_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    LocalUdpEchoServer(const LocalUdpEchoServer&) = delete;
    LocalUdpEchoServer& operator=(const LocalUdpEchoServer&) = delete;

    ~LocalUdpEchoServer() {
        io_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::uint16_t port() const {
        return port_;
    }

private:
    void run() {
        ready_.store(true);

        std::array<char, 256> buffer{};
        asio::ip::udp::endpoint remote;
        const auto length = socket_.receive_from(asio::buffer(buffer), remote);
        socket_.send_to(asio::buffer(buffer.data(), length), remote);
    }

    asio::io_context io_;
    asio::ip::udp::socket socket_;
    std::uint16_t port_;
    std::atomic<bool> ready_{false};
    std::thread thread_;
};

} // namespace

TEST_CASE("UdpSocket sends and receives datagrams") {
    LocalUdpEchoServer server;

    auto socket = nexus::net::UdpSocket::bind({"127.0.0.1", 0});
    REQUIRE(socket.ok());
    CHECK(socket.value().is_open());

    const auto sent = socket.value().send_to("ping", {"127.0.0.1", server.port()});
    REQUIRE(sent.ok());
    CHECK(sent.value() == 4);

    const auto datagram = socket.value().receive_from(16);
    REQUIRE(datagram.ok());
    CHECK(datagram.value().data == "ping");
    CHECK(datagram.value().remote.host == "127.0.0.1");
    CHECK(datagram.value().remote.port == server.port());

    CHECK(socket.value().close().ok());
    CHECK_FALSE(socket.value().is_open());
}

TEST_CASE("UdpSocket reports invalid endpoints") {
    const auto socket = nexus::net::UdpSocket::bind({"", 0});

    REQUIRE_FALSE(socket.ok());
    CHECK(socket.status().code() == nexus::StatusCode::kInvalidArgument);
}
