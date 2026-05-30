#pragma once

#include <cstdint>
#include <string>

namespace cuwacunu {
namespace piaabo {
namespace curl {

struct openai_responses_request_t {
  std::string endpoint{};
  std::string bearer_token{};
  std::string model{};
  std::string reasoning_effort{"medium"};
  std::string system_prompt{};
  std::string user_prompt{};
  double temperature{0.0};
  double top_p{1.0};
  int64_t max_output_tokens{1024};
  std::string user_agent{"cuwacunu-openai-responses/0.1"};
  int64_t timeout_ms{30000};
  int64_t connect_timeout_ms{10000};
  int64_t retry_max_attempts{0};
  int64_t retry_backoff_ms{0};
  bool verify_tls{true};
};

struct openai_responses_result_t {
  bool ok{false};
  long http_code{0};
  std::string body{};
  std::string error{};
};

[[nodiscard]] bool openai_curl_global_init(std::string* error = nullptr);
void openai_curl_global_cleanup();

[[nodiscard]] bool post_openai_responses(const openai_responses_request_t& request,
                                         openai_responses_result_t* result);

}  // namespace curl
}  // namespace piaabo
}  // namespace cuwacunu
