#include "piaabo/https_compat/curl_toolkit/openai_responses_api.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

#include <curl/curl.h>

namespace {

std::mutex g_openai_curl_global_mutex;
bool g_openai_curl_initialized = false;

[[nodiscard]] std::string json_escape(std::string_view in) {
  std::ostringstream out;
  for (const unsigned char c : in) {
    switch (c) {
      case '\"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (c < 0x20) {
          out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<int>(c) << std::dec << std::setfill(' ');
        } else {
          out << static_cast<char>(c);
        }
        break;
    }
  }
  return out.str();
}

[[nodiscard]] std::string build_openai_request_payload(
    const cuwacunu::piaabo::curl::openai_responses_request_t& request) {
  std::ostringstream json;
  json << "{";
  json << "\"model\":\"" << json_escape(request.model) << "\",";
  json << "\"input\":[";
  json << "{\"role\":\"system\",\"content\":[{\"type\":\"input_text\",\"text\":\""
       << json_escape(request.system_prompt) << "\"}]},";
  json << "{\"role\":\"user\",\"content\":[{\"type\":\"input_text\",\"text\":\""
       << json_escape(request.user_prompt) << "\"}]}";
  json << "],";
  json << "\"reasoning\":{\"effort\":\"" << json_escape(request.reasoning_effort)
       << "\"},";
  json << "\"temperature\":" << std::setprecision(12) << request.temperature
       << ",";
  json << "\"top_p\":" << std::setprecision(12) << request.top_p << ",";
  json << "\"max_output_tokens\":" << request.max_output_tokens;
  json << "}";
  return json.str();
}

size_t curl_write_callback(char* ptr, size_t size, size_t nmemb,
                           void* userdata) {
  const size_t bytes = size * nmemb;
  if (userdata == nullptr || ptr == nullptr) return 0;
  auto* out = static_cast<std::string*>(userdata);
  out->append(ptr, bytes);
  return bytes;
}

struct http_attempt_t {
  bool ok{false};
  bool retryable{false};
  long http_code{0};
  std::string body{};
  std::string error{};
};

[[nodiscard]] http_attempt_t post_once(
    const cuwacunu::piaabo::curl::openai_responses_request_t& request,
    const std::string& payload) {
  http_attempt_t out;

  CURL* curl = curl_easy_init();
  if (curl == nullptr) {
    out.error = "curl_easy_init() failed";
    out.retryable = true;
    return out;
  }

  std::string response_body;
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  const std::string auth_header = "Authorization: Bearer " + request.bearer_token;
  headers = curl_slist_append(headers, auth_header.c_str());
  if (headers == nullptr) {
    curl_easy_cleanup(curl);
    out.error = "failed to build HTTP headers";
    out.retryable = true;
    return out;
  }

  curl_easy_setopt(curl, CURLOPT_URL, request.endpoint.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
  curl_easy_setopt(curl, CURLOPT_USERAGENT, request.user_agent.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, request.timeout_ms);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, request.connect_timeout_ms);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, request.verify_tls ? 1L : 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, request.verify_tls ? 2L : 0L);

  const CURLcode rc = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  out.http_code = http_code;
  out.body = std::move(response_body);

  if (rc != CURLE_OK) {
    out.error = curl_easy_strerror(rc);
    out.retryable = true;
    return out;
  }
  if (http_code < 200 || http_code >= 300) {
    std::ostringstream ss;
    ss << "http status " << http_code;
    out.error = ss.str();
    out.retryable = (http_code == 429 || http_code >= 500);
    return out;
  }

  out.ok = true;
  return out;
}

}  // namespace

namespace cuwacunu {
namespace piaabo {
namespace curl {

bool openai_curl_global_init(std::string* error) {
  std::lock_guard<std::mutex> lk(g_openai_curl_global_mutex);
  if (g_openai_curl_initialized) return true;
  const CURLcode rc = curl_global_init(CURL_GLOBAL_DEFAULT);
  if (rc != CURLE_OK) {
    if (error != nullptr) *error = curl_easy_strerror(rc);
    return false;
  }
  g_openai_curl_initialized = true;
  return true;
}

void openai_curl_global_cleanup() {
  std::lock_guard<std::mutex> lk(g_openai_curl_global_mutex);
  if (!g_openai_curl_initialized) return;
  curl_global_cleanup();
  g_openai_curl_initialized = false;
}

bool post_openai_responses(const openai_responses_request_t& request,
                           openai_responses_result_t* result) {
  if (result == nullptr) return false;
  result->ok = false;
  result->http_code = 0;
  result->body.clear();
  result->error.clear();

  if (request.endpoint.empty()) {
    result->error = "endpoint is empty";
    return false;
  }
  if (request.bearer_token.empty()) {
    result->error = "bearer token is empty";
    return false;
  }
  if (request.model.empty()) {
    result->error = "model is empty";
    return false;
  }

  std::string init_error;
  if (!openai_curl_global_init(&init_error)) {
    result->error = "curl global init failed: " + init_error;
    return false;
  }

  const std::string payload = build_openai_request_payload(request);
  const int max_attempts =
      static_cast<int>(std::max<int64_t>(0, request.retry_max_attempts)) + 1;

  int attempt = 0;
  http_attempt_t last;
  while (attempt < max_attempts) {
    last = post_once(request, payload);
    if (last.ok) {
      result->ok = true;
      result->http_code = last.http_code;
      result->body = std::move(last.body);
      return true;
    }
    ++attempt;
    if (attempt >= max_attempts || !last.retryable) break;
    std::this_thread::sleep_for(
        std::chrono::milliseconds(std::max<int64_t>(0, request.retry_backoff_ms)));
  }

  result->ok = false;
  result->http_code = last.http_code;
  result->body = std::move(last.body);
  result->error = std::move(last.error);
  return false;
}

}  // namespace curl
}  // namespace piaabo
}  // namespace cuwacunu
