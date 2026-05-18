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

class SilentTcpServer {
public:
    SilentTcpServer()
        : acceptor_(io_, asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0)),
          port_(acceptor_.local_endpoint().port()) {
        thread_ = std::thread([this]() {
            ready_.store(true);
            asio::ip::tcp::socket socket(io_);
            acceptor_.accept(socket);
            // Accept and hold — never sends data.
            while (!stop_.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

        while (!ready_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    SilentTcpServer(const SilentTcpServer&) = delete;
    SilentTcpServer& operator=(const SilentTcpServer&) = delete;

    ~SilentTcpServer() {
        stop_.store(true);
        io_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::uint16_t port() const {
        return port_;
    }

private:
    asio::io_context io_;
    asio::ip::tcp::acceptor acceptor_;
    std::uint16_t port_;
    std::atomic<bool> ready_{false};
    std::atomic<bool> stop_{false};
    std::thread thread_;
};

class LoopTcpEchoServer {
public:
    LoopTcpEchoServer()
        : endpoint_(asio::ip::make_address("127.0.0.1"), 0),
          acceptor_(io_, endpoint_),
          port_(acceptor_.local_endpoint().port()) {
        thread_ = std::thread([this]() { run(); });
        while (!ready_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    LoopTcpEchoServer(const LoopTcpEchoServer&) = delete;
    LoopTcpEchoServer& operator=(const LoopTcpEchoServer&) = delete;

    ~LoopTcpEchoServer() {
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
        std::array<char, 4096> buffer{};
        for (;;) {
            asio::error_code ec;
            const auto length = socket.read_some(asio::buffer(buffer), ec);
            if (ec) break;
            asio::write(socket, asio::buffer(buffer.data(), length), ec);
            if (ec) break;
        }
    }

    asio::io_context io_;
    asio::ip::tcp::endpoint endpoint_;
    asio::ip::tcp::acceptor acceptor_;
    std::uint16_t port_ = 0;
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

TEST_CASE("TcpClient connects with explicit timeout") {
    LocalTcpEchoServer server;

    nexus::net::TcpConnectOptions options;
    options.timeout = std::chrono::seconds(5);
    auto client = nexus::net::TcpClient::connect({"127.0.0.1", server.port()}, options);
    REQUIRE(client.ok());
    CHECK(client.value().is_open());

    REQUIRE(client.value().write_all("hello").ok());
    const auto response = client.value().read_some(16);
    REQUIRE(response.ok());
    CHECK(response.value() == "hello");
}

TEST_CASE("TcpClient timeout on non-routable address") {
    nexus::net::TcpConnectOptions options;
    options.timeout = std::chrono::seconds(1);

    const auto client = nexus::net::TcpClient::connect({"10.255.255.1", 12345}, options);
    REQUIRE_FALSE(client.ok());
    CHECK(client.status().code() == nexus::StatusCode::kUnavailable);
}

TEST_CASE("TcpClient connect with options validates endpoints") {
    nexus::net::TcpConnectOptions options;
    options.timeout = std::chrono::seconds(5);

    const auto client = nexus::net::TcpClient::connect({"", 0}, options);
    REQUIRE_FALSE(client.ok());
    CHECK(client.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("TcpClient write and read with io timeout") {
    LocalTcpEchoServer server;

    auto client = nexus::net::TcpClient::connect({"127.0.0.1", server.port()});
    REQUIRE(client.ok());

    nexus::net::TcpIoOptions io_opts;
    io_opts.timeout = std::chrono::seconds(5);

    REQUIRE(client.value().write_all("timeout-test", io_opts).ok());

    const auto response = client.value().read_some(32, io_opts);
    REQUIRE(response.ok());
    CHECK(response.value() == "timeout-test");
}

TEST_CASE("TcpClient read timeout on silent server") {
    SilentTcpServer server;

    auto client = nexus::net::TcpClient::connect({"127.0.0.1", server.port()});
    REQUIRE(client.ok());

    nexus::net::TcpIoOptions io_opts;
    io_opts.timeout = std::chrono::milliseconds(200);

    const auto response = client.value().read_some(32, io_opts);
    REQUIRE_FALSE(response.ok());
    CHECK(response.status().code() == nexus::StatusCode::kUnavailable);
}

// --- async tests ---

TEST_CASE("TcpClient async connects sends and receives") {
    LoopTcpEchoServer server;

    auto client_result = nexus::net::TcpClient::create();
    REQUIRE(client_result.ok());
    auto& client = client_result.value();

    nexus::net::TcpEndpoint ep;
    ep.host = "127.0.0.1";
    ep.port = server.port();

    nexus::common::Event connected;
    nexus::common::Event write_done;
    nexus::common::Event read_done;
    nexus::Status connect_status(nexus::StatusCode::kInternal, "unset");
    nexus::Status write_status(nexus::StatusCode::kInternal, "unset");
    bool read_ok = false;
    std::string read_data;

    client.async_connect(ep, [&](nexus::Status s) {
        connect_status = s;
        connected.emit();
    });
    CHECK(connected.wait(2000));
    REQUIRE(connect_status.ok());
    CHECK(client.is_open());

    client.async_write_all("hello async tcp", [&](nexus::Status s) {
        write_status = s;
        write_done.emit();
    });
    CHECK(write_done.wait(2000));
    REQUIRE(write_status.ok());

    client.async_read_some(256, [&](nexus::Result<std::string> r) {
        read_ok = r.ok();
        if (r.ok()) {
            read_data = std::move(r).value();
        }
        read_done.emit();
    });
    CHECK(read_done.wait(2000));
    REQUIRE(read_ok);
    CHECK(read_data == "hello async tcp");

    bool closed = false;
    client.async_close([&](nexus::Status) { closed = true; });
    for (int i = 0; i < 100 && !closed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(closed);
    CHECK_FALSE(client.is_open());
}

TEST_CASE("TcpClient async connect timeout fires") {
    auto client = nexus::net::TcpClient::create();
    REQUIRE(client.ok());

    nexus::net::TcpEndpoint ep;
    ep.host = "10.255.255.1";
    ep.port = 12345;

    nexus::net::TcpConnectOptions opts;
    opts.timeout = std::chrono::milliseconds(200);

    bool timed_out = false;
    client.value().async_connect(ep, opts, [&](nexus::Status s) {
        CHECK_FALSE(s.ok());
        if (s.code() == nexus::StatusCode::kUnavailable) {
            timed_out = true;
        }
    });

    for (int i = 0; i < 300 && !timed_out; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(timed_out);
}

TEST_CASE("TcpClient async rejects invalid endpoint") {
    auto client = nexus::net::TcpClient::create();
    REQUIRE(client.ok());

    nexus::net::TcpEndpoint ep;
    ep.host = "";
    ep.port = 0;

    bool invalid = false;
    client.value().async_connect(ep, [&](nexus::Status s) {
        CHECK_FALSE(s.ok());
        if (s.code() == nexus::StatusCode::kInvalidArgument) {
            invalid = true;
        }
    });

    for (int i = 0; i < 100 && !invalid; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(invalid);
}

TEST_CASE("TcpClient async ops on closed client return error") {
    auto client = nexus::net::TcpClient::create();
    REQUIRE(client.ok());
    client.value().async_close(nullptr);

    // Wait for close to complete.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    bool write_failed = false;
    client.value().async_write_all("data", [&](nexus::Status s) {
        CHECK_FALSE(s.ok());
        CHECK(s.code() == nexus::StatusCode::kFailedPrecondition);
        write_failed = true;
    });

    bool read_failed = false;
    client.value().async_read_some(256, [&](nexus::Result<std::string> r) {
        CHECK_FALSE(r.ok());
        CHECK(r.status().code() == nexus::StatusCode::kFailedPrecondition);
        read_failed = true;
    });

    for (int i = 0; i < 200 && (!write_failed || !read_failed); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(write_failed);
    CHECK(read_failed);
}

TEST_CASE("TcpClient async move transfers ownership") {
    auto r1 = nexus::net::TcpClient::create();
    REQUIRE(r1.ok());
    auto c1 = std::move(r1).value();

    auto c2 = std::move(c1);
    CHECK_FALSE(c1.is_open());

    bool closed = false;
    c2.async_close([&](nexus::Status) { closed = true; });

    for (int i = 0; i < 100 && !closed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(closed);
}

TEST_CASE("TcpClient async write timeout fires") {
    LoopTcpEchoServer server;

    auto client = nexus::net::TcpClient::create();
    REQUIRE(client.ok());

    nexus::net::TcpEndpoint ep;
    ep.host = "127.0.0.1";
    ep.port = server.port();

    bool connect_done = false;
    client.value().async_connect(ep, [&](nexus::Status s) {
        REQUIRE(s.ok());
        connect_done = true;
    });
    for (int i = 0; i < 100 && !connect_done; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(connect_done);

    nexus::net::TcpIoOptions opts;
    opts.timeout = std::chrono::milliseconds(100);

    std::string big_data(1024 * 1024, 'x'); // 1 MB

    bool write_done = false;
    client.value().async_write_all(big_data, opts, [&](nexus::Status /*s*/) {
        write_done = true;
    });

    for (int i = 0; i < 300 && !write_done; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(write_done);

    client.value().async_close(nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_CASE("TcpClient async destroy with pending read does not hang") {
    LoopTcpEchoServer server;

    auto destroy_start = std::chrono::steady_clock::now();
    {
        auto client = nexus::net::TcpClient::create();
        REQUIRE(client.ok());

        nexus::net::TcpEndpoint ep;
        ep.host = "127.0.0.1";
        ep.port = server.port();

        bool connected = false;
        client.value().async_connect(ep, [&](nexus::Status s) {
            REQUIRE(s.ok());
            connected = true;
        });
        for (int i = 0; i < 100 && !connected; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        REQUIRE(connected);

        // Start a read that will hang.
        client.value().async_read_some(256, [](nexus::Result<std::string> /*r*/) {});

        destroy_start = std::chrono::steady_clock::now();
        // client goes out of scope here — destructor must not hang
    }
    auto elapsed = std::chrono::steady_clock::now() - destroy_start;
    CHECK(elapsed < std::chrono::milliseconds(1000));
}

TEST_CASE("TcpClient async close from callback does not deadlock") {
    LoopTcpEchoServer server;

    auto client = nexus::net::TcpClient::create();
    REQUIRE(client.ok());

    nexus::net::TcpEndpoint ep;
    ep.host = "127.0.0.1";
    ep.port = server.port();

    std::atomic<bool> close_callback_fired{false};
    std::atomic<bool> connect_done{false};

    client.value().async_connect(ep, [&](nexus::Status s) {
        REQUIRE(s.ok());
        connect_done.store(true);
    });
    for (int i = 0; i < 100 && !connect_done.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(connect_done.load());

    client.value().async_write_all("close-me", [&](nexus::Status s) {
        REQUIRE(s.ok());
        client.value().async_close([&](nexus::Status /*s2*/) {
            close_callback_fired.store(true);
        });
    });

    for (int i = 0; i < 100 && !close_callback_fired.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(close_callback_fired.load());
    CHECK_FALSE(client.value().is_open());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
