#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

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
    using Server = websocketpp::server<websocketpp::config::asio>;

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
