#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#include <nexus/common/thread.h>
#include <nexus/core/status.h>
#include <nexus/net/tcp.h>
#include <nexus/net/tcp_listener.h>

namespace {

class EchoResponder {
public:
    EchoResponder() {
        thread_ = std::thread([this]() { run(); });
        while (!ready_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    EchoResponder(const EchoResponder&) = delete;
    EchoResponder& operator=(const EchoResponder&) = delete;

    ~EchoResponder() {
        stop_.store(true);
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::uint16_t port() const { return port_; }

private:
    void run() {
        auto listener = nexus::net::TcpListener::create({"127.0.0.1", 0});
        if (!listener.ok()) return;

        port_ = listener.value().port();
        ready_.store(true);

        while (!stop_.load()) {
            // Accept and echo once, then exit loop
        }
    }

    std::atomic<bool> ready_{false};
    std::atomic<bool> stop_{false};
    std::thread thread_;
    std::uint16_t port_ = 0;
};

} // namespace

TEST_CASE("TcpListener creates and listens on an available port") {
    auto listener = nexus::net::TcpListener::create({"127.0.0.1", 0});
    REQUIRE(listener.ok());
    CHECK(listener.value().is_listening());
}

TEST_CASE("TcpListener rejects invalid endpoint") {
    auto listener = nexus::net::TcpListener::create({"", 0});
    REQUIRE_FALSE(listener.ok());
    CHECK(listener.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("TcpListener accepts an incoming connection") {
    auto listener = nexus::net::TcpListener::create({"127.0.0.1", 0});
    REQUIRE(listener.ok());

    // Connect a client in the background.
    auto port = listener.value().port();
    std::atomic<bool> connect_done{false};
    nexus::Status connect_status(nexus::StatusCode::kInternal, "unset");
    std::thread connector([&]() {
        auto client = nexus::net::TcpClient::connect({"127.0.0.1", port});
        connect_done.store(true);
        if (client.ok()) {
            connect_status = nexus::Status::ok_status();
            client.value().close();
        } else {
            connect_status = client.status();
        }
    });

    // Accept the connection.
    nexus::common::Event accepted;
    nexus::Status accept_status(nexus::StatusCode::kInternal, "unset");
    bool has_client = false;

    listener.value().async_accept(
        [&](nexus::Result<nexus::net::TcpClient> client) {
            accept_status = client.ok()
                ? nexus::Status::ok_status()
                : client.status();
            has_client = client.ok();
            accepted.emit();
        });

    CHECK(accepted.wait(2000));
    CHECK(accept_status.ok());
    CHECK(has_client);

    connector.join();
}

TEST_CASE("TcpListener accepts and communicates over accepted client") {
    auto listener = nexus::net::TcpListener::create({"127.0.0.1", 0});
    REQUIRE(listener.ok());
    auto port = listener.value().port();

    // Start accepting.
    nexus::common::Event accepted;
    nexus::Result<nexus::net::TcpClient> accepted_client(
        nexus::Status(nexus::StatusCode::kInternal, "unset"));

    listener.value().async_accept(
        [&](nexus::Result<nexus::net::TcpClient> client) {
            accepted_client = std::move(client);
            accepted.emit();
        });

    // Connect and send data.
    auto client = nexus::net::TcpClient::connect({"127.0.0.1", port});
    REQUIRE(client.ok());

    CHECK(accepted.wait(2000));
    REQUIRE(accepted_client.ok());

    // Send from connecting client to accepted client.
    REQUIRE(client.value().write_all("hello listener").ok());

    auto received = accepted_client.value().read_some(64);
    REQUIRE(received.ok());
    CHECK(received.value() == "hello listener");

    // Send back from accepted client.
    REQUIRE(accepted_client.value().write_all("hello connector").ok());

    auto response = client.value().read_some(64);
    REQUIRE(response.ok());
    CHECK(response.value() == "hello connector");
}

TEST_CASE("TcpListener close stops listening") {
    auto listener = nexus::net::TcpListener::create({"127.0.0.1", 0});
    REQUIRE(listener.ok());
    CHECK(listener.value().is_listening());

    CHECK(listener.value().close().ok());
    CHECK_FALSE(listener.value().is_listening());
}

TEST_CASE("TcpListener close is idempotent") {
    auto listener = nexus::net::TcpListener::create({"127.0.0.1", 0});
    REQUIRE(listener.ok());

    CHECK(listener.value().close().ok());
    CHECK(listener.value().close().ok());
    CHECK_FALSE(listener.value().is_listening());
}

TEST_CASE("TcpListener async_accept on closed listener returns error") {
    auto listener = nexus::net::TcpListener::create({"127.0.0.1", 0});
    REQUIRE(listener.ok());
    CHECK(listener.value().close().ok());

    nexus::common::Event done;
    bool error_returned = false;

    listener.value().async_accept(
        [&](nexus::Result<nexus::net::TcpClient> client) {
            error_returned = !client.ok();
            done.emit();
        });

    CHECK(done.wait(1000));
    CHECK(error_returned);
}

TEST_CASE("TcpListener move transfers ownership") {
    auto r1 = nexus::net::TcpListener::create({"127.0.0.1", 0});
    REQUIRE(r1.ok());
    auto l1 = std::move(r1).value();
    CHECK(l1.is_listening());

    auto l2 = std::move(l1);
    CHECK_FALSE(l1.is_listening());
    CHECK(l2.is_listening());

    CHECK(l2.close().ok());
    CHECK_FALSE(l2.is_listening());
}

TEST_CASE("TcpListener reuse address allows quick restart") {
    auto l1 = nexus::net::TcpListener::create({"127.0.0.1", 0});
    REQUIRE(l1.ok());
    auto port = l1.value().port();
    l1.value().close();

    nexus::net::TcpListenOptions options;
    options.reuse_address = true;
    auto l2 = nexus::net::TcpListener::create({"127.0.0.1", port}, options);
    REQUIRE(l2.ok());
    CHECK(l2.value().is_listening());
}
