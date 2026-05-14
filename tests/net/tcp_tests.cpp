#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#include <asio.hpp>

#include <nexus/core/status.h>
#include <nexus/net/tcp.h>

namespace {

class LocalTcpEchoServer {
public:
    LocalTcpEchoServer()
        : acceptor_(io_, asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0)),
          port_(acceptor_.local_endpoint().port()) {
        thread_ = std::thread([this]() {
            run();
        });

        while (!ready_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    LocalTcpEchoServer(const LocalTcpEchoServer&) = delete;
    LocalTcpEchoServer& operator=(const LocalTcpEchoServer&) = delete;

    ~LocalTcpEchoServer() {
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
        asio::ip::tcp::socket socket(io_);
        acceptor_.accept(socket);

        std::array<char, 256> buffer{};
        const auto length = socket.read_some(asio::buffer(buffer));
        asio::write(socket, asio::buffer(buffer.data(), length));
    }

    asio::io_context io_;
    asio::ip::tcp::acceptor acceptor_;
    std::uint16_t port_;
    std::atomic<bool> ready_{false};
    std::thread thread_;
};

} // namespace

TEST_CASE("TcpClient connects writes and reads data") {
    LocalTcpEchoServer server;

    auto client = nexus::net::TcpClient::connect({"127.0.0.1", server.port()});
    REQUIRE(client.ok());
    CHECK(client.value().is_open());

    REQUIRE(client.value().write_all("ping").ok());

    const auto response = client.value().read_some(16);
    REQUIRE(response.ok());
    CHECK(response.value() == "ping");

    CHECK(client.value().close().ok());
    CHECK_FALSE(client.value().is_open());
}

TEST_CASE("TcpClient reports invalid endpoints") {
    const auto client = nexus::net::TcpClient::connect({"", 0});

    REQUIRE_FALSE(client.ok());
    CHECK(client.status().code() == nexus::StatusCode::kInvalidArgument);
}
