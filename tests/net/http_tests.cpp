#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include <httplib.h>

#include <nexus/core/status.h>
#include <nexus/log/logger.h>
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

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

std::filesystem::path test_log_path(const std::string& name) {
    auto path = std::filesystem::temp_directory_path() / "nexuskit-tests" / name;
    std::filesystem::create_directories(path.parent_path());
    std::filesystem::remove(path);
    return path;
}

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
    CHECK(response.value().ok());

    const auto header = response.value().header("x-nexus");
    REQUIRE(header.ok());
    CHECK(header.value() == "ready");
}

TEST_CASE("HttpClient GET returns HTTP error responses") {
    LocalHttpServer server;

    const auto client = nexus::net::HttpClient::create(server.base_url());
    REQUIRE(client.ok());

    const auto response = client.value().get("/missing");
    REQUIRE(response.ok());
    CHECK(response.value().status_code == 404);
    CHECK(response.value().body == "missing");
    CHECK_FALSE(response.value().ok());
}

TEST_CASE("HttpResponse reports missing headers") {
    LocalHttpServer server;

    const auto client = nexus::net::HttpClient::create(server.base_url());
    REQUIRE(client.ok());

    const auto response = client.value().get("/health");
    REQUIRE(response.ok());

    const auto header = response.value().header("X-Missing");
    REQUIRE_FALSE(header.ok());
    CHECK(header.status().code() == nexus::StatusCode::kNotFound);
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

TEST_CASE("HttpClient writes diagnostic logs when a default logger is installed") {
    LocalHttpServer server;
    const auto path = test_log_path("http_client_diagnostics.log");
    auto logger = nexus::log::create_file_logger("http_client_diagnostics", path);
    REQUIRE(logger.ok());

    nexus::log::clear_default_logger();
    nexus::log::set_default_logger(logger.value());

    const auto client = nexus::net::HttpClient::create(server.base_url());
    REQUIRE(client.ok());

    const auto response = client.value().get("/health");
    REQUIRE(response.ok());

    nexus::log::default_logger().flush();
    nexus::log::clear_default_logger();

    const auto contents = read_file(path);
    REQUIRE(contents.find("HTTP GET /health") != std::string::npos);
    REQUIRE(contents.find("status=200") != std::string::npos);
}
