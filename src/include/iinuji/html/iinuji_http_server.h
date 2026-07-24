// SPDX-License-Identifier: MIT
#pragma once

#include "iinuji/html/synthetic_chart_api.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

namespace cuwacunu::iinuji::html {

inline constexpr std::size_t kMaximumHttpRequestHeaderBytes = 8U * 1024U;

enum class http_method_e { get, head };

struct http_request_t {
  http_method_e method{http_method_e::get};
  std::string path;
  std::string version;
  std::map<std::string, std::string> headers;
  std::map<std::string, std::string> query;
};

struct http_response_t {
  int status{200};
  std::string reason{"OK"};
  std::string content_type{"text/plain; charset=utf-8"};
  std::string body;
  std::vector<std::pair<std::string, std::string>> extra_headers;
};

class http_request_error : public std::runtime_error {
public:
  http_request_error(int status, std::string reason, std::string message,
                     bool include_allow = false)
      : std::runtime_error(std::move(message)), status_(status),
        reason_(std::move(reason)), include_allow_(include_allow) {}

  [[nodiscard]] int status() const noexcept { return status_; }
  [[nodiscard]] const std::string &reason() const noexcept { return reason_; }
  [[nodiscard]] bool include_allow() const noexcept { return include_allow_; }

private:
  int status_;
  std::string reason_;
  bool include_allow_;
};

namespace iinuji_http_detail {

inline constexpr std::string_view kAssetRelativeRoot =
    "src/resources/iinuji/html/synthetic_charts";

[[nodiscard]] inline bool is_http_token_char(unsigned char c) {
  if (std::isalnum(c) != 0) {
    return true;
  }
  constexpr std::string_view punctuation = "!#$%&'*+-.^_`|~";
  return punctuation.find(static_cast<char>(c)) != std::string_view::npos;
}

[[nodiscard]] inline std::string lowercase_ascii(std::string_view value) {
  std::string result;
  result.reserve(value.size());
  for (const unsigned char c : value) {
    result.push_back(static_cast<char>(std::tolower(c)));
  }
  return result;
}

[[nodiscard]] inline std::string trim_http_value(std::string_view value) {
  const auto first = value.find_first_not_of(" \t");
  if (first == std::string_view::npos) {
    return {};
  }
  const auto last = value.find_last_not_of(" \t");
  return std::string(value.substr(first, last - first + 1U));
}

[[nodiscard]] inline bool unsafe_request_target(std::string_view target) {
  if (target.empty() || target.front() != '/' || target.size() > 2048U) {
    return true;
  }
  if (target.find('%') != std::string_view::npos ||
      target.find('\\') != std::string_view::npos ||
      target.find('#') != std::string_view::npos ||
      target.find("..") != std::string_view::npos ||
      target.find("//") != std::string_view::npos) {
    return true;
  }
  for (const unsigned char c : target) {
    if (c <= 0x20U || c == 0x7fU) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline bool safe_benchmark_id(std::string_view value) {
  return !value.empty() && value.size() <= 96U &&
         std::all_of(value.begin(), value.end(), [](unsigned char c) {
           return std::isalnum(c) != 0 || c == '_' || c == '-' || c == '.';
         });
}

[[nodiscard]] inline std::string error_json(int status,
                                            std::string_view reason) {
  return "{\"error\":{\"status\":" + std::to_string(status) + ",\"reason\":\"" +
         synthetic_chart_detail::json_escape(reason) + "\"}}\n";
}

[[nodiscard]] inline http_response_t
make_error_response(int status, std::string reason,
                    bool include_allow = false) {
  http_response_t response;
  response.status = status;
  response.reason = std::move(reason);
  response.content_type = "application/json; charset=utf-8";
  response.body = error_json(response.status, response.reason);
  if (include_allow) {
    response.extra_headers.emplace_back("Allow", "GET, HEAD");
  }
  return response;
}

class socket_t {
public:
  explicit socket_t(int descriptor = -1) noexcept : descriptor_(descriptor) {}
  ~socket_t() {
    if (descriptor_ >= 0) {
      ::close(descriptor_);
    }
  }
  socket_t(const socket_t &) = delete;
  socket_t &operator=(const socket_t &) = delete;
  socket_t(socket_t &&other) noexcept
      : descriptor_(std::exchange(other.descriptor_, -1)) {}
  socket_t &operator=(socket_t &&other) noexcept {
    if (this != &other) {
      if (descriptor_ >= 0) {
        ::close(descriptor_);
      }
      descriptor_ = std::exchange(other.descriptor_, -1);
    }
    return *this;
  }
  [[nodiscard]] int get() const noexcept { return descriptor_; }
  [[nodiscard]] explicit operator bool() const noexcept {
    return descriptor_ >= 0;
  }

private:
  int descriptor_;
};

[[nodiscard]] inline std::string receive_request_header(int client) {
  std::string request;
  request.reserve(2048U);
  std::array<char, 2048> buffer{};
  while (request.find("\r\n\r\n") == std::string::npos) {
    const auto received = ::recv(client, buffer.data(), buffer.size(), 0);
    if (received == 0) {
      break;
    }
    if (received < 0) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        throw http_request_error(408, "Request Timeout",
                                 "request header timed out");
      }
      throw http_request_error(400, "Bad Request", "request receive failed");
    }
    const auto count = static_cast<std::size_t>(received);
    if (request.size() + count > kMaximumHttpRequestHeaderBytes) {
      throw http_request_error(431, "Request Header Fields Too Large",
                               "request header exceeds 8 KiB");
    }
    request.append(buffer.data(), count);
  }
  if (request.find("\r\n\r\n") == std::string::npos) {
    throw http_request_error(400, "Bad Request", "incomplete request header");
  }
  return request;
}

inline void send_all(int client, std::string_view response) {
  std::size_t offset = 0U;
  while (offset < response.size()) {
    const auto sent = ::send(client, response.data() + offset,
                             response.size() - offset, MSG_NOSIGNAL);
    if (sent < 0) {
      if (errno == EINTR) {
        continue;
      }
      return;
    }
    if (sent == 0) {
      return;
    }
    offset += static_cast<std::size_t>(sent);
  }
}

} // namespace iinuji_http_detail

[[nodiscard]] inline http_request_t parse_http_request(std::string_view raw) {
  if (raw.size() > kMaximumHttpRequestHeaderBytes) {
    throw http_request_error(431, "Request Header Fields Too Large",
                             "request header exceeds 8 KiB");
  }
  const auto header_end = raw.find("\r\n\r\n");
  if (header_end == std::string_view::npos) {
    throw http_request_error(400, "Bad Request", "incomplete request header");
  }
  const auto first_line_end = raw.find("\r\n");
  if (first_line_end == std::string_view::npos || first_line_end == 0U) {
    throw http_request_error(400, "Bad Request", "missing request line");
  }
  for (std::size_t index = 0U; index < header_end + 4U; ++index) {
    const unsigned char c = static_cast<unsigned char>(raw[index]);
    if (c == 0U || (c < 0x20U && c != '\r' && c != '\n' && c != '\t') ||
        c == 0x7fU) {
      throw http_request_error(400, "Bad Request",
                               "request contains a forbidden control byte");
    }
  }

  const auto request_line = raw.substr(0U, first_line_end);
  const auto first_space = request_line.find(' ');
  const auto second_space = first_space == std::string_view::npos
                                ? std::string_view::npos
                                : request_line.find(' ', first_space + 1U);
  if (first_space == std::string_view::npos ||
      second_space == std::string_view::npos || first_space == 0U ||
      second_space == first_space + 1U ||
      request_line.find(' ', second_space + 1U) != std::string_view::npos) {
    throw http_request_error(400, "Bad Request", "malformed request line");
  }

  http_request_t request;
  const auto method = request_line.substr(0U, first_space);
  if (method == "GET") {
    request.method = http_method_e::get;
  } else if (method == "HEAD") {
    request.method = http_method_e::head;
  } else {
    throw http_request_error(405, "Method Not Allowed",
                             "only GET and HEAD are supported", true);
  }
  const auto request_target = request_line.substr(
      first_space + 1U, second_space - first_space - 1U);
  const auto query_at = request_target.find('?');
  const auto path_view = request_target.substr(0U, query_at);
  request.path = std::string(path_view);
  request.version = std::string(request_line.substr(second_space + 1U));
  if (request.version != "HTTP/1.1" && request.version != "HTTP/1.0") {
    throw http_request_error(505, "HTTP Version Not Supported",
                             "only HTTP/1.0 and HTTP/1.1 are supported");
  }
  if (iinuji_http_detail::unsafe_request_target(request.path)) {
    throw http_request_error(400, "Bad Request", "unsafe request target");
  }
  if (query_at != std::string_view::npos) {
    const auto query = request_target.substr(query_at + 1U);
    if (query.empty() || query.find('?') != std::string_view::npos ||
        query.find('&') != std::string_view::npos ||
        query.find(';') != std::string_view::npos ||
        query.find('%') != std::string_view::npos ||
        query.find('#') != std::string_view::npos) {
      throw http_request_error(400, "Bad Request", "unsafe query string");
    }
    const auto equals = query.find('=');
    if (equals == std::string_view::npos || equals == 0U ||
        equals + 1U >= query.size() ||
        query.find('=', equals + 1U) != std::string_view::npos) {
      throw http_request_error(400, "Bad Request", "malformed query string");
    }
    const auto name = query.substr(0U, equals);
    const auto value = query.substr(equals + 1U);
    if (name != "benchmark" ||
        !iinuji_http_detail::safe_benchmark_id(value)) {
      throw http_request_error(400, "Bad Request",
                               "unsupported query parameter");
    }
    request.query.emplace("benchmark", std::string(value));
  }

  std::size_t cursor = first_line_end + 2U;
  std::size_t header_count = 0U;
  while (cursor < header_end) {
    const auto line_end = raw.find("\r\n", cursor);
    if (line_end == std::string_view::npos || line_end > header_end ||
        line_end == cursor) {
      throw http_request_error(400, "Bad Request", "malformed header line");
    }
    const auto line = raw.substr(cursor, line_end - cursor);
    if (line.front() == ' ' || line.front() == '\t') {
      throw http_request_error(400, "Bad Request",
                               "obsolete folded headers are rejected");
    }
    const auto colon = line.find(':');
    if (colon == std::string_view::npos || colon == 0U) {
      throw http_request_error(400, "Bad Request", "malformed header field");
    }
    const auto name_view = line.substr(0U, colon);
    if (!std::all_of(name_view.begin(), name_view.end(), [](unsigned char c) {
          return iinuji_http_detail::is_http_token_char(c);
        })) {
      throw http_request_error(400, "Bad Request", "invalid header name");
    }
    const auto name = iinuji_http_detail::lowercase_ascii(name_view);
    const auto value =
        iinuji_http_detail::trim_http_value(line.substr(colon + 1U));
    for (const unsigned char c : value) {
      if ((c < 0x20U && c != '\t') || c == 0x7fU) {
        throw http_request_error(400, "Bad Request", "invalid header value");
      }
    }
    if (!request.headers.emplace(name, value).second) {
      throw http_request_error(400, "Bad Request", "duplicate header field");
    }
    ++header_count;
    if (header_count > 64U) {
      throw http_request_error(431, "Request Header Fields Too Large",
                               "too many header fields");
    }
    cursor = line_end + 2U;
  }

  if (request.version == "HTTP/1.1") {
    const auto host = request.headers.find("host");
    if (host == request.headers.end() || host->second.empty()) {
      throw http_request_error(400, "Bad Request",
                               "HTTP/1.1 requires a Host header");
    }
  }
  if (request.headers.contains("transfer-encoding")) {
    throw http_request_error(400, "Bad Request",
                             "request transfer encodings are unsupported");
  }
  if (const auto content_length = request.headers.find("content-length");
      content_length != request.headers.end()) {
    std::uint64_t parsed = 0U;
    try {
      parsed = synthetic_chart_detail::parse_u64(content_length->second,
                                                 "HTTP Content-Length");
    } catch (const synthetic_chart_error &) {
      throw http_request_error(400, "Bad Request",
                               "invalid HTTP Content-Length");
    }
    if (parsed != 0U) {
      throw http_request_error(400, "Bad Request",
                               "GET/HEAD request bodies are unsupported");
    }
  }
  return request;
}

[[nodiscard]] inline std::string
serialize_http_response(const http_response_t &response, bool head_only) {
  if (response.status < 100 || response.status > 599 ||
      response.reason.empty()) {
    throw std::invalid_argument("invalid HTTP response status");
  }
  std::ostringstream output;
  output << "HTTP/1.1 " << response.status << ' ' << response.reason << "\r\n"
         << "Content-Type: " << response.content_type << "\r\n"
         << "Content-Length: " << response.body.size() << "\r\n"
         << "Connection: close\r\n"
         << "Cache-Control: no-store\r\n"
         << "Content-Security-Policy: default-src 'none'; script-src 'self'; "
            "style-src 'self'; img-src 'self'; connect-src 'self'; "
            "base-uri 'none'; frame-ancestors 'none'; form-action 'none'; "
            "object-src 'none'\r\n"
         << "X-Content-Type-Options: nosniff\r\n"
         << "X-Frame-Options: DENY\r\n"
         << "Referrer-Policy: no-referrer\r\n"
         << "Cross-Origin-Resource-Policy: same-origin\r\n"
         << "Permissions-Policy: camera=(), microphone=(), geolocation=()\r\n";
  for (const auto &[name, value] : response.extra_headers) {
    output << name << ": " << value << "\r\n";
  }
  output << "\r\n";
  if (!head_only) {
    output << response.body;
  }
  return output.str();
}

class iinuji_http_application_t {
public:
  explicit iinuji_http_application_t(std::filesystem::path repo_root)
      : repository_(std::move(repo_root)),
        v2_repository_(repository_.repo_root()) {
    using synthetic_chart_detail::read_file;
    const auto base =
        std::filesystem::path(iinuji_http_detail::kAssetRelativeRoot);
    assets_.emplace(
        "/index.html",
        static_asset_t{"text/html; charset=utf-8",
                       read_file(repository_.repo_root(), base / "index.html",
                                 1024U * 1024U)});
    assets_.emplace(
        "/app.css",
        static_asset_t{"text/css; charset=utf-8",
                       read_file(repository_.repo_root(), base / "app.css",
                                 2U * 1024U * 1024U)});
    assets_.emplace("/app.js", static_asset_t{"text/javascript; charset=utf-8",
                                              read_file(repository_.repo_root(),
                                                        base / "app.js",
                                                        2U * 1024U * 1024U)});
  }

  [[nodiscard]] const synthetic_chart_repository_t &
  repository() const noexcept {
    return repository_;
  }

  [[nodiscard]] const synthetic_chart_v2_repository_t &
  v2_repository() const noexcept {
    return v2_repository_;
  }

  [[nodiscard]] const std::string &benchmark_catalog_json() const noexcept {
    return benchmark_catalog_json_;
  }

  [[nodiscard]] http_response_t route(const http_request_t &request) const {
    if (request.method != http_method_e::get &&
        request.method != http_method_e::head) {
      return iinuji_http_detail::make_error_response(405, "Method Not Allowed",
                                                     true);
    }
    if (request.path == "/healthz") {
      return {200,
              "OK",
              "application/json; charset=utf-8",
              "{\"status\":\"ok\",\"service\":\"iinuji-html\"}\n",
              {}};
    }
    if (request.path == "/api/v1/benchmarks" && request.query.empty()) {
      return {200,
              "OK",
              "application/json; charset=utf-8",
              benchmark_catalog_json_,
              {}};
    }
    if (request.path == "/api/v1/catalog") {
      const auto benchmark = selected_benchmark(request);
      if (benchmark == kSyntheticBenchmarkV2Id) {
        return {200,
                "OK",
                "application/json; charset=utf-8",
                v2_repository_.catalog_json(),
                {}};
      }
      if (benchmark != kSyntheticBenchmarkV1Id) {
        return iinuji_http_detail::make_error_response(404, "Not Found");
      }
      return {200,
              "OK",
              "application/json; charset=utf-8",
              repository_.catalog_json(),
              {}};
    }
    constexpr std::string_view chart_prefix = "/api/v1/chart/";
    if (request.path.starts_with(chart_prefix)) {
      const std::string_view suffix(request.path.data() + chart_prefix.size(),
                                    request.path.size() - chart_prefix.size());
      const auto slash = suffix.find('/');
      if (slash == std::string_view::npos || slash == 0U ||
          slash + 1U >= suffix.size() ||
          suffix.find('/', slash + 1U) != std::string_view::npos) {
        return iinuji_http_detail::make_error_response(404, "Not Found");
      }
      const auto instrument = suffix.substr(0U, slash);
      const auto interval = suffix.substr(slash + 1U);
      const auto benchmark = selected_benchmark(request);
      if (benchmark == kSyntheticBenchmarkV2Id) {
        if (!synthetic_chart_v2_repository_t::instrument_allowed(instrument) ||
            !synthetic_chart_v2_repository_t::interval_allowed(interval)) {
          return iinuji_http_detail::make_error_response(404, "Not Found");
        }
        return {200,
                "OK",
                "application/json; charset=utf-8",
                v2_repository_.chart_json(instrument, interval),
                {}};
      }
      if (benchmark != kSyntheticBenchmarkV1Id ||
          !synthetic_chart_repository_t::instrument_allowed(instrument) ||
          !synthetic_chart_repository_t::interval_allowed(interval)) {
        return iinuji_http_detail::make_error_response(404, "Not Found");
      }
      return {200,
              "OK",
              "application/json; charset=utf-8",
              repository_.chart_json(instrument, interval),
              {}};
    }
    const std::string asset_path =
        request.path == "/" ? "/index.html" : request.path;
    if (const auto asset = assets_.find(asset_path); asset != assets_.end()) {
      return {200, "OK", asset->second.content_type, asset->second.body, {}};
    }
    return iinuji_http_detail::make_error_response(404, "Not Found");
  }

private:
  struct static_asset_t {
    std::string content_type;
    std::string body;
  };

  [[nodiscard]] static std::string_view
  selected_benchmark(const http_request_t &request) {
    const auto found = request.query.find("benchmark");
    return found == request.query.end() ? kSyntheticBenchmarkV1Id
                                        : std::string_view(found->second);
  }

  synthetic_chart_repository_t repository_;
  synthetic_chart_v2_repository_t v2_repository_;
  const std::string benchmark_catalog_json_{
      "{\"schema_id\":\"iinuji.synthetic_benchmark_catalog.v1\","
      "\"default_benchmark_id\":\"synthetic_continuous_graph_v1\","
      "\"benchmarks\":["
      "{\"id\":\"synthetic_continuous_graph_v1\","
      "\"label\":\"V1 periodic diagnostic\","
      "\"served_anchor_end_exclusive\":1088,"
      "\"test_holdout_served\":false},"
      "{\"id\":\"synthetic_continuous_graph_v2\","
      "\"label\":\"V2 causal aggregate process\","
      "\"data_scope\":\"development_prefix_only\","
      "\"served_anchor_end_exclusive\":3264,"
      "\"test_holdout_served\":false,\"raw_source_served\":false}]}"};
  std::map<std::string, static_asset_t> assets_;
};

struct iinuji_http_server_config_t {
  std::string bind_address{"127.0.0.1"};
  std::uint16_t port{8765U};
  std::size_t max_requests{0U};
};

class iinuji_http_server_t {
public:
  iinuji_http_server_t(
      iinuji_http_server_config_t config,
      std::shared_ptr<const iinuji_http_application_t> application)
      : config_(std::move(config)), application_(std::move(application)) {
    if (!application_) {
      throw std::invalid_argument("iinuji HTTP application is null");
    }
    if (config_.bind_address.empty() || config_.port == 0U) {
      throw std::invalid_argument("iinuji bind address/port is invalid");
    }
  }

  [[nodiscard]] std::size_t run() const {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
    addrinfo *raw_addresses = nullptr;
    const auto port_text = std::to_string(config_.port);
    const int lookup = ::getaddrinfo(config_.bind_address.c_str(),
                                     port_text.c_str(), &hints, &raw_addresses);
    if (lookup != 0) {
      throw std::runtime_error("invalid numeric bind address: " +
                               config_.bind_address);
    }
    const std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> addresses(
        raw_addresses, &::freeaddrinfo);

    iinuji_http_detail::socket_t listener;
    for (auto *address = addresses.get(); address != nullptr;
         address = address->ai_next) {
      iinuji_http_detail::socket_t candidate(::socket(
          address->ai_family, address->ai_socktype, address->ai_protocol));
      if (!candidate) {
        continue;
      }
      int enabled = 1;
      (void)::setsockopt(candidate.get(), SOL_SOCKET, SO_REUSEADDR, &enabled,
                         sizeof(enabled));
      if (::bind(candidate.get(), address->ai_addr, address->ai_addrlen) == 0 &&
          ::listen(candidate.get(), 16) == 0) {
        listener = std::move(candidate);
        break;
      }
    }
    if (!listener) {
      throw std::runtime_error("cannot bind iinuji HTTP server to " +
                               config_.bind_address + ":" + port_text);
    }

    std::size_t handled = 0U;
    while (config_.max_requests == 0U || handled < config_.max_requests) {
      iinuji_http_detail::socket_t client;
      while (!client) {
        const int descriptor = ::accept(listener.get(), nullptr, nullptr);
        if (descriptor >= 0) {
          client = iinuji_http_detail::socket_t(descriptor);
          break;
        }
        if (errno != EINTR) {
          throw std::runtime_error("iinuji HTTP accept failed: " +
                                   std::string(std::strerror(errno)));
        }
      }
      timeval timeout{};
      timeout.tv_sec = 5;
      (void)::setsockopt(client.get(), SOL_SOCKET, SO_RCVTIMEO, &timeout,
                         sizeof(timeout));
      (void)::setsockopt(client.get(), SOL_SOCKET, SO_SNDTIMEO, &timeout,
                         sizeof(timeout));

      bool head_only = false;
      http_response_t response;
      try {
        const auto raw =
            iinuji_http_detail::receive_request_header(client.get());
        const auto request = parse_http_request(raw);
        head_only = request.method == http_method_e::head;
        response = application_->route(request);
      } catch (const http_request_error &error) {
        response = iinuji_http_detail::make_error_response(
            error.status(), error.reason(), error.include_allow());
      } catch (const std::exception &) {
        response = iinuji_http_detail::make_error_response(
            500, "Internal Server Error");
      }
      iinuji_http_detail::send_all(
          client.get(), serialize_http_response(response, head_only));
      ++handled;
    }
    return handled;
  }

private:
  iinuji_http_server_config_t config_;
  std::shared_ptr<const iinuji_http_application_t> application_;
};

} // namespace cuwacunu::iinuji::html
