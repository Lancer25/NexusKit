#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/net/export.h>

namespace nexus::net {

namespace detail {
class HttpClientStorage;
}

struct HttpHeader {
    std::string name;
    std::string value;
};

struct HttpQueryParameter {
    std::string name;
    std::string value;
};

struct HttpResponse {
    int status_code = 0;
    std::string body;
    std::vector<HttpHeader> headers;
};

struct HttpClientOptions {
    std::chrono::milliseconds connect_timeout{5000};
    std::chrono::milliseconds read_timeout{5000};
    std::chrono::milliseconds write_timeout{5000};
};

class NEXUS_NET_API HttpClient {
public:
    HttpClient(const HttpClient& other);
    HttpClient(HttpClient&& other) noexcept;
    HttpClient& operator=(const HttpClient& other);
    HttpClient& operator=(HttpClient&& other) noexcept;
    ~HttpClient();

    static Result<HttpClient> create(std::string base_url, HttpClientOptions options = {});

    Result<HttpResponse> get(
        std::string_view path,
        const std::vector<HttpHeader>& headers = {}) const;

    Result<HttpResponse> post(
        std::string_view path,
        std::string_view body,
        std::string_view content_type,
        const std::vector<HttpHeader>& headers = {}) const;

    Result<HttpResponse> put(
        std::string_view path,
        std::string_view body,
        std::string_view content_type,
        const std::vector<HttpHeader>& headers = {}) const;

    Result<HttpResponse> del(
        std::string_view path,
        const std::vector<HttpHeader>& headers = {}) const;

private:
    explicit HttpClient(std::shared_ptr<detail::HttpClientStorage> storage);

    std::shared_ptr<detail::HttpClientStorage> storage_;
};

NEXUS_NET_API std::string build_query_path(
    std::string_view path,
    const std::vector<HttpQueryParameter>& parameters);

} // namespace nexus::net
