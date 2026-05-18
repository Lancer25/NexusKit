#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#ifdef NEXUS_NET_HAS_TLS
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
using WsppServerConfig = websocketpp::config::asio_tls;
#else
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
using WsppServerConfig = websocketpp::config::asio;
#endif

#include <nexus/common/thread.h>
#include <nexus/core/status.h>
#include <nexus/net/websocket_client.h>

namespace {

class LocalWebSocketEchoServer {
public:
    LocalWebSocketEchoServer() {
        server_.clear_access_channels(websocketpp::log::alevel::all);
        server_.clear_error_channels(websocketpp::log::elevel::all);
        server_.init_asio();
        server_.set_reuse_addr(true);
        server_.set_message_handler([this](
                                        websocketpp::connection_hdl handle,
                                        Server::message_ptr message) {
            websocketpp::lib::error_code error;
            server_.send(handle, message->get_payload(), websocketpp::frame::opcode::text, error);
        });

        websocketpp::lib::error_code error;
        server_.listen(asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0), error);
        REQUIRE_FALSE(error);
        const auto endpoint = server_.get_local_endpoint(error);
        REQUIRE_FALSE(error);
        port_ = endpoint.port();
        server_.start_accept();

        thread_ = std::thread([this]() {
            ready_.store(true);
            server_.run();
        });

        while (!ready_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    LocalWebSocketEchoServer(const LocalWebSocketEchoServer&) = delete;
    LocalWebSocketEchoServer& operator=(const LocalWebSocketEchoServer&) = delete;

    ~LocalWebSocketEchoServer() {
        server_.stop_listening();
        server_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::string url() const {
        return "ws://127.0.0.1:" + std::to_string(port_);
    }

private:
    using Server = websocketpp::server<WsppServerConfig>;

    Server server_;
    std::uint16_t port_ = 0;
    std::atomic<bool> ready_{false};
    std::thread thread_;
};

} // namespace

TEST_CASE("WebSocketClient connects sends and receives text") {
    LocalWebSocketEchoServer server;

    auto client = nexus::net::WebSocketClient::connect(server.url());
    REQUIRE(client.ok());
    CHECK(client.value().is_open());

    REQUIRE(client.value().send_text("hello websocket").ok());

    const auto message = client.value().receive_text();
    REQUIRE(message.ok());
    CHECK(message.value() == "hello websocket");

    CHECK(client.value().close().ok());
    CHECK_FALSE(client.value().is_open());
}

TEST_CASE("WebSocketClient rejects unsupported URLs") {
    const auto client = nexus::net::WebSocketClient::connect("http://127.0.0.1:80");

    REQUIRE_FALSE(client.ok());
    CHECK(client.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("WebSocketClientOptions write_timeout defaults to zero") {
    nexus::net::WebSocketClientOptions options;
    CHECK(options.write_timeout == std::chrono::milliseconds{0});
}

TEST_CASE("WebSocketClient async connects sends and receives") {
    LocalWebSocketEchoServer server;

    auto client_result = nexus::net::WebSocketClient::create();
    REQUIRE(client_result.ok());
    auto& client = client_result.value();

    nexus::common::Event connected;
    nexus::common::Event write_done;
    nexus::common::Event read_done;
    nexus::Status connect_status(nexus::StatusCode::kInternal, "unset");
    nexus::Status write_status(nexus::StatusCode::kInternal, "unset");
    bool read_ok = false;
    std::string read_data;

    client.async_connect(server.url(), [&](nexus::Status s) {
        connect_status = s;
        connected.emit();
    });
    CHECK(connected.wait(2000));
    REQUIRE(connect_status.ok());
    CHECK(client.is_open());

    client.async_send_text("hello async ws", [&](nexus::Status s) {
        write_status = s;
        write_done.emit();
    });
    CHECK(write_done.wait(2000));
    REQUIRE(write_status.ok());

    client.async_receive_text([&](nexus::Result<std::string> r) {
        read_ok = r.ok();
        if (r.ok()) read_data = std::move(r).value();
        read_done.emit();
    });
    CHECK(read_done.wait(2000));
    REQUIRE(read_ok);
    CHECK(read_data == "hello async ws");

    bool closed = false;
    client.async_close([&](nexus::Status) { closed = true; });
    for (int i = 0; i < 100 && !closed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(closed);
    CHECK_FALSE(client.is_open());
}

TEST_CASE("WebSocketClient async connect rejects invalid URL") {
    auto client = nexus::net::WebSocketClient::create();
    REQUIRE(client.ok());

    bool invalid = false;
    client.value().async_connect("http://bad", [&](nexus::Status s) {
        CHECK_FALSE(s.ok());
        if (s.code() == nexus::StatusCode::kInvalidArgument) invalid = true;
    });
    for (int i = 0; i < 100 && !invalid; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(invalid);
}

TEST_CASE("WebSocketClient async ops on closed client return error") {
    auto client = nexus::net::WebSocketClient::create();
    REQUIRE(client.ok());

    // Close immediately to set stopped flag.
    client.value().close();

    bool send_failed = false;
    client.value().async_send_text("data", [&](nexus::Status s) {
        CHECK_FALSE(s.ok());
        CHECK(s.code() == nexus::StatusCode::kFailedPrecondition);
        send_failed = true;
    });
    CHECK(send_failed);

    bool recv_failed = false;
    client.value().async_receive_text([&](nexus::Result<std::string> r) {
        CHECK_FALSE(r.ok());
        CHECK(r.status().code() == nexus::StatusCode::kFailedPrecondition);
        recv_failed = true;
    });
    CHECK(recv_failed);
}

TEST_CASE("WebSocketClient destroy with pending receive does not hang") {
    LocalWebSocketEchoServer server;

    auto destroy_start = std::chrono::steady_clock::now();
    {
        auto client = nexus::net::WebSocketClient::create();
        REQUIRE(client.ok());

        bool connected = false;
        client.value().async_connect(server.url(), [&](nexus::Status s) {
            REQUIRE(s.ok());
            connected = true;
        });
        for (int i = 0; i < 100 && !connected; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        REQUIRE(connected);

        // Start a receive that won't complete (server awaits our send).
        client.value().async_receive_text([](nexus::Result<std::string> /*r*/) {});

        destroy_start = std::chrono::steady_clock::now();
        // client goes out of scope here — destructor must not hang
    }
    auto elapsed = std::chrono::steady_clock::now() - destroy_start;
    CHECK(elapsed < std::chrono::milliseconds(1000));
}

TEST_CASE("WebSocketClient close from callback does not deadlock") {
    LocalWebSocketEchoServer server;

    auto client = nexus::net::WebSocketClient::create();
    REQUIRE(client.ok());

    std::atomic<bool> close_callback_fired{false};
    std::atomic<bool> connect_done{false};

    client.value().async_connect(server.url(), [&](nexus::Status s) {
        REQUIRE(s.ok());
        connect_done.store(true);
    });
    for (int i = 0; i < 100 && !connect_done.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(connect_done.load());

    // Send text, then close from the send callback.
    client.value().async_send_text("close-me", [&](nexus::Status s) {
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
