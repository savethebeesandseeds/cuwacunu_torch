// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace cuwacunu {
namespace hero {
namespace human {

inline constexpr std::string_view kHumanGovernanceResolutionSchemaV3 =
    "hero.human.governance_resolution.v3";

struct human_resolution_record_t {
  std::string schema{std::string(kHumanGovernanceResolutionSchemaV3)};
  std::string marshal_session_id{};
  std::uint64_t checkpoint_index{0};
  std::string request_sha256_hex{};
  std::string operator_id{};
  std::uint64_t resolved_at_ms{0};
  std::string resolution_kind{};
  std::string governance_kind{};
  bool grant_allow_default_write{false};
  std::uint64_t grant_additional_campaign_launches{0};
  std::string reason{};
  std::string signer_public_key_fingerprint_sha256_hex{};
};

[[nodiscard]] bool validate_human_resolution_record(
    const human_resolution_record_t& record, std::string* error);
[[nodiscard]] std::string human_resolution_to_json(
    const human_resolution_record_t& record);
[[nodiscard]] bool parse_human_resolution_json(const std::string& json,
                                               human_resolution_record_t* out,
                                               std::string* error);

[[nodiscard]] bool sha256_hex_string(std::string_view payload,
                                     std::string* out_hex,
                                     std::string* error);
[[nodiscard]] bool sha256_hex_file(const std::filesystem::path& path,
                                   std::string* out_hex,
                                   std::string* error);

[[nodiscard]] bool sign_human_attested_json(
    const std::filesystem::path& ssh_identity_path,
    std::string_view payload_json, std::string* out_signature_hex,
    std::string* out_public_key_fingerprint_sha256_hex, std::string* error);
[[nodiscard]] bool verify_human_attested_json_signature(
    const std::filesystem::path& operator_identities_path,
    std::string_view operator_id, std::string_view payload_json,
    std::string_view signature_hex,
    std::string* out_public_key_fingerprint_sha256_hex, std::string* error);

}  // namespace human
}  // namespace hero
}  // namespace cuwacunu
