#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#include <asio.hpp>

#include <nexus/common/thread.h>
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

TEST_CASE("UdpSocket receive_from with zero timeout behaves like no-timeout overload") {
    LocalUdpEchoServer server;

    auto result = nexus::net::UdpSocket::bind({"127.0.0.1", 0});
    REQUIRE(result.ok());
    auto socket = std::move(result).value();

    socket.send_to("hello", {"127.0.0.1", server.port()});

    nexus::net::UdpReceiveOptions options;
    options.timeout = std::chrono::milliseconds{0};
    auto datagram = socket.receive_from(1024, options);
    REQUIRE(datagram.ok());
    CHECK(datagram.value().data == "hello");
}

TEST_CASE("UdpSocket receive_from times out when no datagram arrives") {
    auto result = nexus::net::UdpSocket::bind({"127.0.0.1", 0});
    REQUIRE(result.ok());
    auto socket = std::move(result).value();

    nexus::net::UdpReceiveOptions options;
    options.timeout = std::chrono::milliseconds{100};
    auto datagram = socket.receive_from(1024, options);
    REQUIRE_FALSE(datagram.ok());
    CHECK(datagram.status().code() == nexus::StatusCode::kUnavailable);
}

TEST_CASE("UdpSocket receive_from with options rejects invalid size") {
    auto result = nexus::net::UdpSocket::bind({"127.0.0.1", 0});
    REQUIRE(result.ok());
    auto socket = std::move(result).value();

    nexus::net::UdpReceiveOptions options;
    options.timeout = std::chrono::milliseconds{100};
    auto datagram = socket.receive_from(0, options);
    REQUIRE_FALSE(datagram.ok());
    CHECK(datagram.status().code() == nexus::StatusCode::kInvalidArgument);
}

// --- async tests ---

TEST_CASE("UdpSocket async binds sends and receives") {
    auto server_r = nexus::net::UdpSocket::create();
    REQUIRE(server_r.ok());
    auto& server = server_r.value();

    nexus::net::UdpEndpoint server_ep;
    server_ep.host = "127.0.0.1";
    server_ep.port = 0;

    nexus::common::Event server_bound;
    server.async_bind(server_ep, [&](nexus::Status s) {
        REQUIRE(s.ok());
        server_bound.emit();
    });
    CHECK(server_bound.wait(2000));

    auto client_r = nexus::net::UdpSocket::create();
    REQUIRE(client_r.ok());
    auto& client = client_r.value();

    nexus::net::UdpEndpoint client_ep;
    client_ep.host = "127.0.0.1";
    client_ep.port = 0;

    nexus::common::Event bound;
    nexus::common::Event sent;
    nexus::common::Event echoed;
    nexus::common::Event received;
    nexus::Status bind_status(nexus::StatusCode::kInternal, "unset");
    bool send_ok = false;
    bool echo_recv_ok = false;
    bool recv_ok = false;
    std::string recv_data;

    client.async_bind(client_ep, [&](nexus::Status s) {
        bind_status = s;
        bound.emit();
    });
    CHECK(bound.wait(2000));
    REQUIRE(bind_status.ok());

    server.async_receive_from(256, [&](nexus::Result<nexus::net::UdpDatagram> r) {
        echo_recv_ok = r.ok();
        if (r.ok()) {
            server.async_send_to(r.value().data, r.value().remote,
                [&](nexus::Result<std::size_t> /*sr*/) {});
        }
        echoed.emit();
    });

    nexus::net::UdpEndpoint remote;
    remote.host = "127.0.0.1";
    remote.port = server.local_port();

    client.async_send_to("hello async udp", remote,
        [&](nexus::Result<std::size_t> r) {
            send_ok = r.ok();
            sent.emit();
        });
    CHECK(sent.wait(2000));
    REQUIRE(send_ok);

    CHECK(echoed.wait(2000));
    REQUIRE(echo_recv_ok);

    client.async_receive_from(256, [&](nexus::Result<nexus::net::UdpDatagram> r) {
        recv_ok = r.ok();
        if (r.ok()) {
            recv_data = std::move(r).value().data;
        }
        received.emit();
    });
    CHECK(received.wait(2000));
    REQUIRE(recv_ok);
    CHECK(recv_data == "hello async udp");

    bool closed = false;
    client.async_close([&](nexus::Status) { closed = true; });
    for (int i = 0; i < 100 && !closed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(closed);

    server.async_close(nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_CASE("UdpSocket async receive timeout fires") {
    auto sock = nexus::net::UdpSocket::create();
    REQUIRE(sock.ok());

    nexus::net::UdpEndpoint local;
    local.host = "127.0.0.1";
    local.port = 0;

    bool bind_done = false;
    sock.value().async_bind(local, [&](nexus::Status s) {
        REQUIRE(s.ok());
        bind_done = true;
    });
    for (int i = 0; i < 100 && !bind_done; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(bind_done);

    nexus::net::UdpReceiveOptions opts;
    opts.timeout = std::chrono::milliseconds(200);

    bool timed_out = false;
    sock.value().async_receive_from(256, opts,
        [&](nexus::Result<nexus::net::UdpDatagram> r) {
            CHECK_FALSE(r.ok());
            if (r.status().code() == nexus::StatusCode::kUnavailable) {
                timed_out = true;
            }
        });

    for (int i = 0; i < 300 && !timed_out; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(timed_out);

    sock.value().async_close(nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_CASE("UdpSocket async rejects empty remote endpoint") {
    auto sock = nexus::net::UdpSocket::create();
    REQUIRE(sock.ok());

    nexus::net::UdpEndpoint local;
    local.host = "127.0.0.1";
    local.port = 0;

    bool bound = false;
    sock.value().async_bind(local, [&](nexus::Status s) {
        REQUIRE(s.ok());
        bound = true;
    });
    for (int i = 0; i < 100 && !bound; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(bound);

    nexus::net::UdpEndpoint remote;
    // remote.host is empty, remote.port is 0

    bool invalid = false;
    sock.value().async_send_to("data", remote,
        [&](nexus::Result<std::size_t> r) {
            CHECK_FALSE(r.ok());
            if (r.status().code() == nexus::StatusCode::kInvalidArgument) {
                invalid = true;
            }
        });

    for (int i = 0; i < 100 && !invalid; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(invalid);

    sock.value().async_close(nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_CASE("UdpSocket move transfers ownership") {
    auto r1 = nexus::net::UdpSocket::create();
    REQUIRE(r1.ok());
    auto s1 = std::move(r1).value();

    auto s2 = std::move(s1);
    CHECK_FALSE(s1.is_open());

    bool closed = false;
    s2.async_close([&](nexus::Status) { closed = true; });

    for (int i = 0; i < 100 && !closed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(closed);
}

TEST_CASE("UdpSocket destroy with pending receive does not hang") {
    auto destroy_start = std::chrono::steady_clock::now();
    {
        auto sock = nexus::net::UdpSocket::create();
        REQUIRE(sock.ok());

        nexus::net::UdpEndpoint local;
        local.host = "127.0.0.1";
        local.port = 0;

        std::atomic<bool> bound{false};
        sock.value().async_bind(local, [&](nexus::Status s) {
            if (s.ok()) bound.store(true);
        });
        for (int i = 0; i < 100 && !bound.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        REQUIRE(bound.load());

        // Start a receive that will hang (no one sends to us).
        sock.value().async_receive_from(256,
            [](nexus::Result<nexus::net::UdpDatagram> /*r*/) {});

        destroy_start = std::chrono::steady_clock::now();
        // sock goes out of scope — destructor must not hang
    }
    auto elapsed = std::chrono::steady_clock::now() - destroy_start;
    CHECK(elapsed < std::chrono::milliseconds(1000));
}

TEST_CASE("UdpSocket close from callback does not deadlock") {
    asio::io_context io;
    asio::ip::udp::socket sender(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));

    auto sock_r = nexus::net::UdpSocket::create();
    REQUIRE(sock_r.ok());
    auto& sock = sock_r.value();

    nexus::net::UdpEndpoint local;
    local.host = "127.0.0.1";
    local.port = 0;

    nexus::common::Event bound;
    sock.async_bind(local, [&](nexus::Status s) {
        REQUIRE(s.ok());
        bound.emit();
    });
    REQUIRE(bound.wait(5000));

    nexus::common::Event closed;
    sock.async_receive_from(256,
        [&](nexus::Result<nexus::net::UdpDatagram> /*r*/) {
            sock.async_close([&](nexus::Status /*s*/) {
                closed.emit();
            });
        });

    // Send a probe datagram to trigger the receive callback.
    asio::ip::udp::endpoint target(
        asio::ip::make_address("127.0.0.1"), sock.local_port());
    asio::error_code ec;
    sender.send_to(asio::buffer("x", 1), target, 0, ec);
    REQUIRE_FALSE(ec);

    CHECK(closed.wait(5000));
    CHECK_FALSE(sock.is_open());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
