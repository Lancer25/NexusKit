#include <nexus/net/http.h>

#include <cctype>
#include <exception>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <httplib.h>
#include <nexus/log/logger.h>

namespace nexus::net {

namespace detail {

class HttpClientStorage {
public:
    HttpClientStorage(std::string url, HttpClientOptions options)
        : base_url(std::move(url)), client(base_url), options(options) {
        client.set_connection_timeout(options.connect_timeout);
        client.set_read_timeout(options.read_timeout);
        client.set_write_timeout(options.write_timeout);
    }

    std::string base_url;
    httplib::Client client;
    HttpClientOptions options;
};

} // namespace detail

namespace {

bool starts_with(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

bool contains_space(std::string_view value) {
    for (const auto character : value) {
        if (std::isspace(static_cast<unsigned char>(character)) != 0) {
            return true;
        }
    }
    return false;
}

bool equals_ignore_case(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        const auto left = static_cast<unsigned char>(lhs[index]);
        const auto right = static_cast<unsigned char>(rhs[index]);
        if (std::tolower(left) != std::tolower(right)) {
            return false;
        }
    }

    return true;
}

Status validate_base_url(std::string_view base_url) {
    if (!starts_with(base_url, "http://")) {
        return Status::invalid_argument("Only http:// base URLs are supported");
    }

    const auto authority = base_url.substr(7);
    if (authority.empty()) {
        return Status::invalid_argument("HTTP base URL must contain a host");
    }

    if (authority.find('/') != std::string_view::npos) {
        return Status::invalid_argument("HTTP base URL must not contain a path");
    }

    if (contains_space(authority)) {
        return Status::invalid_argument("HTTP base URL must not contain whitespace");
    }

    return Status::ok_status();
}

std::string normalize_path(std::string_view path) {
    if (path.empty()) {
        return "/";
    }

    if (path.front() == '/') {
        return std::string(path);
    }

    return "/" + std::string(path);
}

bool is_unreserved_uri_character(unsigned char character) {
    return (character >= 'A' && character <= 'Z') ||
        (character >= 'a' && character <= 'z') ||
        (character >= '0' && character <= '9') ||
        character == '-' ||
        character == '.' ||
        character == '_' ||
        character == '~';
}

std::string percent_encode(std::string_view value) {
    std::ostringstream stream;
    stream << std::uppercase << std::hex;

    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (is_unreserved_uri_character(byte)) {
            stream << character;
            continue;
        }

        stream << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }

    return stream.str();
}

Status error_to_status(httplib::Error error) {
    switch (error) {
    case httplib::Error::Connection:
        return Status(StatusCode::kUnavailable, "HTTP connection failed");
    case httplib::Error::Read:
        return Status(StatusCode::kUnavailable, "HTTP read failed");
    case httplib::Error::Write:
        return Status(StatusCode::kUnavailable, "HTTP write failed");
    case httplib::Error::ExceedRedirectCount:
        return Status(StatusCode::kFailedPrecondition, "HTTP redirect limit exceeded");
    case httplib::Error::Canceled:
        return Status(StatusCode::kCancelled, "HTTP request was cancelled");
    case httplib::Error::UnsupportedMultipartBoundaryChars:
    case httplib::Error::Compression:
    case httplib::Error::ConnectionTimeout:
        return Status(StatusCode::kUnavailable, "HTTP request failed");
    case httplib::Error::Success:
        return Status::internal("HTTP backend reported success without a response");
    case httplib::Error::Unknown:
    default:
        return Status(StatusCode::kUnknown, "HTTP request failed");
    }
}

void diagnostic_log(log::Level level, const std::string& message) {
    log::write(level, message);
}

std::string request_prefix(std::string_view method, std::string_view path) {
    return "HTTP " + std::string(method) + " " + normalize_path(path);
}

Status log_and_return_failure(std::string_view operation, httplib::Error error) {
    const auto status = error_to_status(error);
    diagnostic_log(log::Level::warn, std::string(operation) + " failed: " + status.message());
    return status;
}

httplib::Headers to_backend_headers(const std::vector<HttpHeader>& headers) {
    httplib::Headers backend_headers;
    for (const auto& header : headers) {
        backend_headers.emplace(header.name, header.value);
    }
    return backend_headers;
}

HttpResponse to_response(const httplib::Response& backend_response) {
    HttpResponse response;
    response.status_code = backend_response.status;
    response.body = backend_response.body;
    response.headers.reserve(backend_response.headers.size());

    for (const auto& header : backend_response.headers) {
        response.headers.push_back(HttpHeader{header.first, header.second});
    }

    return response;
}

HttpResponse log_and_return_response(std::string_view operation, const httplib::Response& backend_response) {
    auto response = to_response(backend_response);
    diagnostic_log(
        log::Level::info,
        std::string(operation) +
            " status=" + std::to_string(response.status_code) +
            " bytes=" + std::to_string(response.body.size()));
    return response;
}

std::shared_ptr<detail::HttpClientStorage> make_storage(
    std::string base_url,
    HttpClientOptions options) {
    return std::make_shared<detail::HttpClientStorage>(std::move(base_url), options);
}

} // namespace

bool HttpResponse::ok() const {
    return status_code >= 200 && status_code < 300;
}

Result<std::string> HttpResponse::header(std::string_view name) const {
    for (const auto& item : headers) {
        if (equals_ignore_case(item.name, name)) {
            return item.value;
        }
    }

    return Status::not_found("HTTP response header not found: " + std::string(name));
}

HttpClient::HttpClient(const HttpClient& other)
    : storage_(make_storage(other.storage_->base_url, other.storage_->options)) {}

HttpClient::HttpClient(HttpClient&& other) noexcept = default;

HttpClient& HttpClient::operator=(const HttpClient& other) {
    if (this != &other) {
        storage_ = make_storage(other.storage_->base_url, other.storage_->options);
    }
    return *this;
}

HttpClient& HttpClient::operator=(HttpClient&& other) noexcept = default;

HttpClient::~HttpClient() = default;

Result<HttpClient> HttpClient::create(std::string base_url, HttpClientOptions options) {
    const auto status = validate_base_url(base_url);
    if (!status.ok()) {
        return status;
    }

    try {
        diagnostic_log(log::Level::debug, "HTTP client create base_url=" + base_url);
        return HttpClient(make_storage(std::move(base_url), options));
    } catch (const std::exception& error) {
        diagnostic_log(log::Level::warn, std::string("HTTP client create failed: ") + error.what());
        return Status::internal(error.what());
    }
}

Result<HttpResponse> HttpClient::get(
    std::string_view path,
    const std::vector<HttpHeader>& headers) const {
    const auto operation = request_prefix("GET", path);
    diagnostic_log(log::Level::debug, operation);

    const auto result = storage_->client.Get(normalize_path(path), to_backend_headers(headers));
    if (!result) {
        return log_and_return_failure(operation, result.error());
    }

    return log_and_return_response(operation, *result);
}

Result<HttpResponse> HttpClient::post(
    std::string_view path,
    std::string_view body,
    std::string_view content_type,
    const std::vector<HttpHeader>& headers) const {
    const auto operation = request_prefix("POST", path);
    diagnostic_log(log::Level::debug, operation + " bytes=" + std::to_string(body.size()));

    const auto result = storage_->client.Post(
        normalize_path(path),
        to_backend_headers(headers),
        std::string(body),
        std::string(content_type));

    if (!result) {
        return log_and_return_failure(operation, result.error());
    }

    return log_and_return_response(operation, *result);
}

Result<HttpResponse> HttpClient::put(
    std::string_view path,
    std::string_view body,
    std::string_view content_type,
    const std::vector<HttpHeader>& headers) const {
    const auto operation = request_prefix("PUT", path);
    diagnostic_log(log::Level::debug, operation + " bytes=" + std::to_string(body.size()));

    const auto result = storage_->client.Put(
        normalize_path(path),
        to_backend_headers(headers),
        std::string(body),
        std::string(content_type));

    if (!result) {
        return log_and_return_failure(operation, result.error());
    }

    return log_and_return_response(operation, *result);
}

Result<HttpResponse> HttpClient::del(
    std::string_view path,
    const std::vector<HttpHeader>& headers) const {
    const auto operation = request_prefix("DELETE", path);
    diagnostic_log(log::Level::debug, operation);

    const auto result = storage_->client.Delete(normalize_path(path), to_backend_headers(headers));
    if (!result) {
        return log_and_return_failure(operation, result.error());
    }

    return log_and_return_response(operation, *result);
}

HttpClient::HttpClient(std::shared_ptr<detail::HttpClientStorage> storage)
    : storage_(std::move(storage)) {}

std::string build_query_path(
    std::string_view path,
    const std::vector<HttpQueryParameter>& parameters) {
    auto result = normalize_path(path);
    if (parameters.empty()) {
        return result;
    }

    result += result.find('?') == std::string::npos ? '?' : '&';

    bool first = true;
    for (const auto& parameter : parameters) {
        if (!first) {
            result += '&';
        }
        first = false;
        result += percent_encode(parameter.name);
        result += '=';
        result += percent_encode(parameter.value);
    }

    return result;
}

} // namespace nexus::net
