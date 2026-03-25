// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace cuwacunu {
namespace hero {
namespace human {

inline constexpr std::string_view kHumanResponseSchemaV1 =
    "hero.human.response.v1";

struct human_response_record_t {
  std::string schema{std::string(kHumanResponseSchemaV1)};
  std::string loop_id{};
  std::uint64_t review_index{0};
  std::string request_sha256_hex{};
  std::string operator_id{};
  std::uint64_t responded_at_ms{0};
  std::string control_kind{};
  std::string next_action_kind{};
  std::string target_binding_id{};
  bool reset_runtime_state{false};
  std::string reason{};
  std::string memory_note{};
  std::string signer_public_key_fingerprint_sha256_hex{};
};

[[nodiscard]] bool validate_human_response_record(
    const human_response_record_t& record, std::string* error);
[[nodiscard]] std::string human_response_to_json(
    const human_response_record_t& record);
[[nodiscard]] bool parse_human_response_json(const std::string& json,
                                             human_response_record_t* out,
                                             std::string* error);

[[nodiscard]] bool sha256_hex_string(std::string_view payload,
                                     std::string* out_hex,
                                     std::string* error);
[[nodiscard]] bool sha256_hex_file(const std::filesystem::path& path,
                                   std::string* out_hex,
                                   std::string* error);

[[nodiscard]] bool sign_human_response_json(
    const std::filesystem::path& ssh_identity_path,
    std::string_view response_json, std::string* out_signature_hex,
    std::string* out_public_key_fingerprint_sha256_hex, std::string* error);
[[nodiscard]] bool verify_human_response_json_signature(
    const std::filesystem::path& operator_identities_path,
    std::string_view operator_id, std::string_view response_json,
    std::string_view signature_hex,
    std::string* out_public_key_fingerprint_sha256_hex, std::string* error);

}  // namespace human
}  // namespace hero
}  // namespace cuwacunu
