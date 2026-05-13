#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <thread>

#include <httplib.h>

#include <nexus/core/status.h>
#include <nexus/net/http.h>

namespace {

class LocalHttpServer {
public:
    LocalHttpServer() {
        server_.Get("/health", [](const httplib::Request&, httplib::Response& response) {
            response.status = 200;
            response.set_header("X-Nexus", "ready");
            response.set_content("ok", "text/plain");
        });

        server_.Get("/agent", [](const httplib::Request& request, httplib::Response& response) {
            response.status = 200;
            response.set_content(request.get_header_value("X-Agent"), "text/plain");
        });

        server_.Get("/search", [](const httplib::Request& request, httplib::Response& response) {
            response.status = 200;
            response.set_content(
                request.get_param_value("q") + ":" + request.get_param_value("page"),
                "text/plain");
        });

        server_.Get("/missing", [](const httplib::Request&, httplib::Response& response) {
            response.status = 404;
            response.set_content("missing", "text/plain");
        });

        server_.Post("/echo", [](const httplib::Request& request, httplib::Response& response) {
            response.status = 201;
            response.set_header("X-Trace", request.get_header_value("X-Trace"));
            response.set_content(request.body, request.get_header_value("Content-Type"));
        });

        server_.Put("/resource", [](const httplib::Request& request, httplib::Response& response) {
            response.status = 200;
            response.set_content("put:" + request.body, request.get_header_value("Content-Type"));
        });

        server_.Delete("/resource", [](const httplib::Request& request, httplib::Response& response) {
            response.status = 202;
            response.set_content("delete:" + request.get_header_value("X-Reason"), "text/plain");
        });

        port_ = server_.bind_to_any_port("127.0.0.1");
        REQUIRE(port_ > 0);

        thread_ = std::thread([this]() {
            server_.listen_after_bind();
        });

        while (!server_.is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    LocalHttpServer(const LocalHttpServer&) = delete;
    LocalHttpServer& operator=(const LocalHttpServer&) = delete;

    ~LocalHttpServer() {
        server_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::string base_url() const {
        return "http://127.0.0.1:" + std::to_string(port_);
    }

private:
    httplib::Server server_;
    int port_ = 0;
    std::thread thread_;
};

} // namespace

TEST_CASE("HttpClient GET returns status body and headers") {
    LocalHttpServer server;

    const auto client = nexus::net::HttpClient::create(server.base_url());
    REQUIRE(client.ok());

    const auto response = client.value().get("/health");
    REQUIRE(response.ok());
    CHECK(response.value().status_code == 200);
    CHECK(response.value().body == "ok");
    REQUIRE(response.value().headers.size() >= 1);
}

TEST_CASE("HttpClient GET returns HTTP error responses") {
    LocalHttpServer server;

    const auto client = nexus::net::HttpClient::create(server.base_url());
    REQUIRE(client.ok());

    const auto response = client.value().get("/missing");
    REQUIRE(response.ok());
    CHECK(response.value().status_code == 404);
    CHECK(response.value().body == "missing");
}

TEST_CASE("HttpClient GET sends request headers") {
    LocalHttpServer server;

    const auto client = nexus::net::HttpClient::create(server.base_url());
    REQUIRE(client.ok());

    const auto response = client.value().get("/agent", {{"X-Agent", "nexus-test"}});
    REQUIRE(response.ok());
    CHECK(response.value().status_code == 200);
    CHECK(response.value().body == "nexus-test");
}

TEST_CASE("HttpClient POST sends body content type and headers") {
    LocalHttpServer server;

    const auto client = nexus::net::HttpClient::create(server.base_url());
    REQUIRE(client.ok());

    const auto response = client.value().post(
        "/echo",
        R"({"name":"camera"})",
        "application/json",
        {{"X-Trace", "phase-4b"}});

    REQUIRE(response.ok());
    CHECK(response.value().status_code == 201);
    CHECK(response.value().body == R"({"name":"camera"})");
}

TEST_CASE("HttpClient PUT sends body and content type") {
    LocalHttpServer server;

    const auto client = nexus::net::HttpClient::create(server.base_url());
    REQUIRE(client.ok());

    const auto response = client.value().put("/resource", "updated", "text/plain");
    REQUIRE(response.ok());
    CHECK(response.value().status_code == 200);
    CHECK(response.value().body == "put:updated");
}

TEST_CASE("HttpClient DELETE sends request headers") {
    LocalHttpServer server;

    const auto client = nexus::net::HttpClient::create(server.base_url());
    REQUIRE(client.ok());

    const auto response = client.value().del("/resource", {{"X-Reason", "cleanup"}});
    REQUIRE(response.ok());
    CHECK(response.value().status_code == 202);
    CHECK(response.value().body == "delete:cleanup");
}

TEST_CASE("build_query_path percent encodes query parameters") {
    const auto path = nexus::net::build_query_path(
        "/search",
        {{"q", "usb camera"}, {"page", "1"}});

    CHECK(path == "/search?q=usb%20camera&page=1");
}

TEST_CASE("HttpClient GET uses query paths") {
    LocalHttpServer server;

    const auto client = nexus::net::HttpClient::create(server.base_url());
    REQUIRE(client.ok());

    const auto path = nexus::net::build_query_path(
        "/search",
        {{"q", "usb camera"}, {"page", "1"}});
    const auto response = client.value().get(path);

    REQUIRE(response.ok());
    CHECK(response.value().status_code == 200);
    CHECK(response.value().body == "usb camera:1");
}

TEST_CASE("HttpClient rejects unsupported base URLs") {
    const auto client = nexus::net::HttpClient::create("ftp://127.0.0.1:80");

    REQUIRE_FALSE(client.ok());
    CHECK(client.status().code() == nexus::StatusCode::kInvalidArgument);
}
