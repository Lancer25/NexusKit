#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include <nexus/common/thread.h>
#include <nexus/core/status.h>
#include <nexus/net/websocket_client.h>
#include <nexus/net/websocket_server.h>

TEST_CASE("WebSocketServer listen sends and receives") {
    auto server = nexus::net::WebSocketServer::create();
    REQUIRE(server.ok());

    nexus::common::Event connected;
    nexus::common::Event message_received;
    nexus::net::WebSocketClientId server_client_id = 0;
    std::string server_received;

    server.value().set_on_connect([&](nexus::net::WebSocketClientId id) {
        server_client_id = id;
        connected.emit();
    });

    server.value().set_on_message([&](nexus::net::WebSocketClientId id,
                                       nexus::Result<std::string> r) {
        REQUIRE(id == server_client_id);
        REQUIRE(r.ok());
        server_received = std::move(r).value();
        message_received.emit();
    });

    std::uint16_t port = 19876;
    REQUIRE(server.value().listen(port).ok());
    CHECK(server.value().is_listening());

    auto client = nexus::net::WebSocketClient::connect("ws://127.0.0.1:" + std::to_string(port));
    REQUIRE(client.ok());
    CHECK(client.value().is_open());

    CHECK(connected.wait(2000));
    REQUIRE(server_client_id != 0);
    CHECK(server.value().client_count() >= 1);

    REQUIRE(client.value().send_text("hello server").ok());

    CHECK(message_received.wait(2000));
    CHECK(server_received == "hello server");

    // Server echoes back.
    REQUIRE(server.value().send_text(server_client_id, "hello client").ok());

    const auto response = client.value().receive_text();
    REQUIRE(response.ok());
    CHECK(response.value() == "hello client");

    CHECK(client.value().close().ok());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    CHECK(server.value().close().ok());
    CHECK_FALSE(server.value().is_listening());
}

TEST_CASE("WebSocketServer handles multiple clients") {
    auto server = nexus::net::WebSocketServer::create();
    REQUIRE(server.ok());

    std::atomic<int> connect_count{0};
    std::vector<nexus::net::WebSocketClientId> client_ids;

    server.value().set_on_connect([&](nexus::net::WebSocketClientId id) {
        client_ids.push_back(id);
        connect_count.fetch_add(1);
    });

    std::uint16_t port = 19877;
    REQUIRE(server.value().listen(port).ok());

    auto client1 = nexus::net::WebSocketClient::connect("ws://127.0.0.1:" + std::to_string(port));
    REQUIRE(client1.ok());

    auto client2 = nexus::net::WebSocketClient::connect("ws://127.0.0.1:" + std::to_string(port));
    REQUIRE(client2.ok());

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    CHECK(connect_count.load() >= 2);
    CHECK(server.value().client_count() >= 2);
    CHECK(client_ids.size() >= 2);
    CHECK(client_ids[0] != client_ids[1]);

    // Send to each client.
    REQUIRE(server.value().send_text(client_ids[0], "msg1").ok());
    REQUIRE(server.value().send_text(client_ids[1], "msg2").ok());

    auto r1 = client1.value().receive_text();
    REQUIRE(r1.ok());
    CHECK(r1.value() == "msg1");

    auto r2 = client2.value().receive_text();
    REQUIRE(r2.ok());
    CHECK(r2.value() == "msg2");

    // Close one client.
    REQUIRE(server.value().close_client(client_ids[0]).ok());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(server.value().client_count() >= 1);

    CHECK(server.value().close().ok());
}

TEST_CASE("WebSocketServer rejects port zero") {
    auto server = nexus::net::WebSocketServer::create();
    REQUIRE(server.ok());

    auto status = server.value().listen(0);
    REQUIRE_FALSE(status.ok());
    CHECK(status.code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("WebSocketServer send to unknown client returns not found") {
    auto server = nexus::net::WebSocketServer::create();
    REQUIRE(server.ok());

    std::uint16_t port = 19878;
    REQUIRE(server.value().listen(port).ok());

    auto status = server.value().send_text(99999, "data");
    REQUIRE_FALSE(status.ok());
    CHECK(status.code() == nexus::StatusCode::kNotFound);
}

TEST_CASE("WebSocketServer async listen send close") {
    auto server = nexus::net::WebSocketServer::create();
    REQUIRE(server.ok());

    std::atomic<nexus::net::WebSocketClientId> server_client_id{0};

    server.value().set_on_connect([&](nexus::net::WebSocketClientId id) {
        server_client_id.store(id);
    });

    nexus::common::Event listen_done;
    nexus::Status listen_status(nexus::StatusCode::kInternal, "unset");
    std::uint16_t port = 19879;

    server.value().async_listen(port, [&](nexus::Status s) {
        listen_status = s;
        listen_done.emit();
    });
    CHECK(listen_done.wait(2000));
    REQUIRE(listen_status.ok());

    auto client = nexus::net::WebSocketClient::connect("ws://127.0.0.1:" + std::to_string(port));
    REQUIRE(client.ok());

    for (int i = 0; i < 100 && server_client_id.load() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(server_client_id.load() != 0);

    // Async send.
    nexus::common::Event send_done;
    bool send_ok = false;
    server.value().async_send_text(server_client_id.load(), "async hello",
        [&](nexus::Status s) {
            send_ok = s.ok();
            send_done.emit();
        });
    CHECK(send_done.wait(2000));
    CHECK(send_ok);

    auto msg = client.value().receive_text();
    REQUIRE(msg.ok());
    CHECK(msg.value() == "async hello");

    // async_close_client
    nexus::common::Event close_done;
    bool close_ok = false;
    server.value().async_close_client(server_client_id.load(),
        [&](nexus::Status s) {
            close_ok = s.ok();
            close_done.emit();
        });
    CHECK(close_done.wait(2000));
    CHECK(close_ok);

    bool stopped = false;
    server.value().async_close([&](nexus::Status) { stopped = true; });
    for (int i = 0; i < 100 && !stopped; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(stopped);
}

TEST_CASE("WebSocketServer disconnect callback fires") {
    auto server = nexus::net::WebSocketServer::create();
    REQUIRE(server.ok());

    std::atomic<bool> disconnected{false};
    nexus::net::WebSocketClientId disconnected_id = 0;

    server.value().set_on_disconnect([&](nexus::net::WebSocketClientId id) {
        disconnected_id = id;
        disconnected.store(true);
    });

    nexus::common::Event connected;
    nexus::net::WebSocketClientId connected_id = 0;

    server.value().set_on_connect([&](nexus::net::WebSocketClientId id) {
        connected_id = id;
        connected.emit();
    });

    std::uint16_t port = 19880;
    REQUIRE(server.value().listen(port).ok());

    auto client = nexus::net::WebSocketClient::connect("ws://127.0.0.1:" + std::to_string(port));
    REQUIRE(client.ok());

    CHECK(connected.wait(2000));
    REQUIRE(connected_id != 0);

    CHECK(client.value().close().ok());

    for (int i = 0; i < 100 && !disconnected.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(disconnected.load());
    CHECK(disconnected_id == connected_id);

    CHECK(server.value().close().ok());
}

TEST_CASE("WebSocketServer destroy with pending connections does not hang") {
    auto destroy_start = std::chrono::steady_clock::now();
    {
        auto server = nexus::net::WebSocketServer::create();
        REQUIRE(server.ok());

        std::uint16_t port = 19881;
        REQUIRE(server.value().listen(port).ok());

        auto client = nexus::net::WebSocketClient::connect("ws://127.0.0.1:" + std::to_string(port));
        REQUIRE(client.ok());

        destroy_start = std::chrono::steady_clock::now();
        // server goes out of scope here — destructor must not hang
    }
    auto elapsed = std::chrono::steady_clock::now() - destroy_start;
    CHECK(elapsed < std::chrono::milliseconds(2000));
}

TEST_CASE("WebSocketServer close from callback does not deadlock") {
    auto server = nexus::net::WebSocketServer::create();
    REQUIRE(server.ok());

    std::atomic<bool> close_callback_fired{false};

    server.value().set_on_connect([&](nexus::net::WebSocketClientId /*id*/) {
        server.value().async_close([&](nexus::Status /*s*/) {
            close_callback_fired.store(true);
        });
    });

    std::uint16_t port = 19882;
    REQUIRE(server.value().listen(port).ok());

    auto client = nexus::net::WebSocketClient::connect("ws://127.0.0.1:" + std::to_string(port));
    REQUIRE(client.ok());

    for (int i = 0; i < 100 && !close_callback_fired.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(close_callback_fired.load());
    CHECK_FALSE(server.value().is_listening());
}
