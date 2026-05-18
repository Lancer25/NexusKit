#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/net/export.h>

/// HTTP client built on cpp-httplib.
///
/// Backend headers are private; users do not need to link against cpp-httplib.
namespace nexus::net {

namespace detail {
class HttpClientStorage;
}

/// An HTTP request or response header.
struct HttpHeader {
    /// Header name (case-insensitive by convention).
    std::string name;
    /// Header value.
    std::string value;
};

/// A URL query parameter.
struct HttpQueryParameter {
    /// Parameter name (percent-encoded in the URL).
    std::string name;
    /// Parameter value (percent-encoded in the URL).
    std::string value;
};

/// An HTTP response.
struct HttpResponse {
    /// HTTP status code (e.g. 200, 404).
    int status_code = 0;
    /// Response body bytes.
    std::string body;
    /// Response headers.
    std::vector<HttpHeader> headers;

    /// True when the status code is in the 2xx range.
    bool ok() const;

    /// Looks up a response header by name (case-insensitive).
    ///
    /// @return The header value, or `kNotFound` when absent.
    Result<std::string> header(std::string_view name) const;
};

/// Connection and I/O timeouts for `HttpClient`.
struct HttpClientOptions {
    /// TCP connect timeout.
    std::chrono::milliseconds connect_timeout{5000};
    /// Socket read timeout.
    std::chrono::milliseconds read_timeout{5000};
    /// Socket write timeout.
    std::chrono::milliseconds write_timeout{5000};
    /// When true, automatically follow HTTP redirects (3xx responses).
    bool follow_redirects{true};
};

/// Copyable HTTP client backed by a shared connection pool.
///
/// Created via `create(base_url, options)`.  Copy is shallow (shared backend);
/// move transfers ownership.
///
/// Synchronous methods (`get`, `post`, `put`, `del`) may block on
/// network I/O.  Async overloads execute on a background worker thread
/// and deliver results via callbacks on that thread.
class NEXUS_NET_API HttpClient {
public:
    using RequestHandler = std::function<void(Result<HttpResponse>)>;
    using CloseHandler   = std::function<void(Status)>;

    /// Copies share the underlying state and start a shared worker thread.
    HttpClient(const HttpClient& other);
    HttpClient(HttpClient&& other) noexcept;
    HttpClient& operator=(const HttpClient& other);
    HttpClient& operator=(HttpClient&& other) noexcept;
    ~HttpClient();

    /// Creates an HTTP client for the given base URL.
    ///
    /// @param base_url Root URL for all requests (e.g. "http://localhost:8080").
    /// @return An HTTP client on success.
    /// @retval kInvalidArgument when `base_url` is empty or malformed.
    static Result<HttpClient> create(std::string base_url, HttpClientOptions options = {});

    /// True while the client hasn't been closed.
    bool is_open() const;

    /// Performs a synchronous HTTP GET request.  May block.
    Result<HttpResponse> get(
        std::string_view path,
        const std::vector<HttpHeader>& headers = {}) const;

    /// Performs a synchronous HTTP POST request.  May block.
    Result<HttpResponse> post(
        std::string_view path,
        std::string_view body,
        std::string_view content_type,
        const std::vector<HttpHeader>& headers = {}) const;

    /// Performs a synchronous HTTP PUT request.  May block.
    Result<HttpResponse> put(
        std::string_view path,
        std::string_view body,
        std::string_view content_type,
        const std::vector<HttpHeader>& headers = {}) const;

    /// Performs a synchronous HTTP DELETE request.  May block.
    Result<HttpResponse> del(
        std::string_view path,
        const std::vector<HttpHeader>& headers = {}) const;

    /// Performs an async HTTP GET.  Handler fires on the worker thread.
    void async_get(std::string_view path,
                   RequestHandler handler,
                   const std::vector<HttpHeader>& headers = {});

    /// Performs an async HTTP POST.  Handler fires on the worker thread.
    void async_post(std::string_view path,
                    std::string_view body,
                    std::string_view content_type,
                    RequestHandler handler,
                    const std::vector<HttpHeader>& headers = {});

    /// Performs an async HTTP PUT.  Handler fires on the worker thread.
    void async_put(std::string_view path,
                   std::string_view body,
                   std::string_view content_type,
                   RequestHandler handler,
                   const std::vector<HttpHeader>& headers = {});

    /// Performs an async HTTP DELETE.  Handler fires on the worker thread.
    void async_del(std::string_view path,
                   RequestHandler handler,
                   const std::vector<HttpHeader>& headers = {});

    /// Closes the client asynchronously.  Idempotent.
    void async_close(CloseHandler handler);

private:
    explicit HttpClient(std::shared_ptr<detail::HttpClientStorage> storage);

    std::shared_ptr<detail::HttpClientStorage> storage_;
};

/// Builds a URL query string from a path and parameter list.
///
/// Percent-encodes parameter names and values per RFC 3986.
NEXUS_NET_API std::string build_query_path(
    std::string_view path,
    const std::vector<HttpQueryParameter>& parameters);

} // namespace nexus::net
