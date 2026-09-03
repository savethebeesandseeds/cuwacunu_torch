#pragma once

#include "piaabo/digest/sha256.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cuwacunu::tests::mtf_surface_sufficiency_map_reference_auditor {

inline constexpr std::string_view kAuditSchema =
    "wikimyei.mtf_jepa_mae_vicreg.rssm_reference_audit.v1";
inline constexpr std::string_view kReferenceSchema =
    "wikimyei.mtf_jepa_mae_vicreg.jmcd.v1";
inline constexpr std::string_view kCandidateSchema =
    "wikimyei.mtf_jepa_mae_vicreg.rssm.v1";
inline constexpr std::size_t kReferenceSizeBytes = 1'050'475;
inline constexpr std::string_view kReferenceSha256 =
    "269665e337730d5d3085848904d2aa6217fdbc14aa65e78393723567d818f1bd";

inline constexpr std::size_t kAcceptedKeyCount = 72;
inline constexpr std::string_view kAcceptedKeySetSha256 =
    "3d32071fc014a2df24745f9b1328e8073dd798a23811679c20e4ee4419347915";
inline constexpr std::string_view kAcceptedKeyValueSha256 =
    "97e87857111ec58e94c2b799808576ea86823a797edf9c52b176031ade791ab1";

inline constexpr std::size_t kLegacyRawKeyCount = 6;
inline constexpr std::string_view kLegacyRawKeySetSha256 =
    "5c09768b4bd6c71e38051c1163cf9bce18911cd474c30974e81923d71d005048";
inline constexpr std::string_view kLegacyRawKeyValueSha256 =
    "061252d17a9768702bb1c43dbd7ba62dcb4984be6811c3e2ca88299a7c586446";

struct BytewiseLess {
  [[nodiscard]] bool operator()(std::string_view lhs,
                                std::string_view rhs) const {
    return std::lexicographical_compare(
        lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
        [](char left, char right) {
          return static_cast<unsigned char>(left) <
                 static_cast<unsigned char>(right);
        });
  }
};

struct Record {
  std::string key{};
  std::string value{};
};

struct SelectionContract {
  std::size_t key_count{0};
  std::string key_set_sha256{};
  std::string key_value_sha256{};
};

struct AuditContract {
  std::size_t reference_size_bytes{0};
  std::string reference_sha256{};
  std::string reference_schema{};
  std::string candidate_schema{};
  SelectionContract accepted{};
  SelectionContract legacy_raw{};
};

struct SelectionSummary {
  std::vector<Record> records{};
  std::set<std::string, BytewiseLess> keys{};
  std::size_t line_count{0};
  std::size_t unique_key_count{0};
  std::size_t duplicate_key_count{0};
  std::size_t duplicate_line_count{0};
  std::string key_set_sha256{};
  std::string key_value_sha256{};
};

struct LogSummary {
  bool utf8_valid{false};
  std::size_t size_bytes{0};
  std::string file_sha256{};
  std::vector<std::string> schema_values{};
  SelectionSummary accepted{};
  SelectionSummary legacy_raw{};
};

struct SelectionAudit {
  bool reference_contract_pass{false};
  bool candidate_contract_pass{false};
  bool candidate_key_set_exact{false};
  bool candidate_value_set_exact{false};
  std::size_t candidate_missing_key_count{0};
  std::size_t candidate_extra_key_count{0};
  bool passed{false};
};

struct AuditResult {
  LogSummary reference{};
  LogSummary candidate{};
  bool reference_file_identity_pass{false};
  bool reference_schema_pass{false};
  bool candidate_schema_pass{false};
  SelectionAudit accepted{};
  SelectionAudit legacy_raw{};
  bool reference_contract_pass{false};
  bool candidate_contract_pass{false};
  bool passed{false};
};

namespace detail {

[[nodiscard]] inline bool starts_with(std::string_view value,
                                      std::string_view prefix) {
  return value.size() >= prefix.size() &&
         value.compare(0, prefix.size(), prefix) == 0;
}

[[nodiscard]] inline bool valid_utf8(std::string_view bytes) {
  const auto byte = [&bytes](std::size_t index) {
    return static_cast<unsigned char>(bytes[index]);
  };
  const auto continuation = [&byte](std::size_t index) {
    return (byte(index) & 0xc0U) == 0x80U;
  };

  for (std::size_t index = 0; index < bytes.size();) {
    const unsigned char first = byte(index);
    if (first <= 0x7fU) {
      ++index;
      continue;
    }
    if (first >= 0xc2U && first <= 0xdfU) {
      if (index + 1 >= bytes.size() || !continuation(index + 1)) {
        return false;
      }
      index += 2;
      continue;
    }
    if (first >= 0xe0U && first <= 0xefU) {
      if (index + 2 >= bytes.size() || !continuation(index + 1) ||
          !continuation(index + 2)) {
        return false;
      }
      const unsigned char second = byte(index + 1);
      if ((first == 0xe0U && second < 0xa0U) ||
          (first == 0xedU && second >= 0xa0U)) {
        return false;
      }
      index += 3;
      continue;
    }
    if (first >= 0xf0U && first <= 0xf4U) {
      if (index + 3 >= bytes.size() || !continuation(index + 1) ||
          !continuation(index + 2) || !continuation(index + 3)) {
        return false;
      }
      const unsigned char second = byte(index + 1);
      if ((first == 0xf0U && second < 0x90U) ||
          (first == 0xf4U && second >= 0x90U)) {
        return false;
      }
      index += 4;
      continue;
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline bool accepted_seed_key(std::string_view key) {
  constexpr std::array<std::string_view, 3> prefixes{
      "seed_17.arm.jepa_mae_only.step_0.",
      "seed_31.arm.jepa_mae_only.step_0.",
      "seed_47.arm.jepa_mae_only.step_0.",
  };
  for (const std::string_view prefix : prefixes) {
    if (!starts_with(key, prefix)) {
      continue;
    }
    const std::string_view suffix = key.substr(prefix.size());
    return starts_with(suffix, "probe.") || starts_with(suffix, "geometry.");
  }
  return false;
}

[[nodiscard]] inline bool accepted_summary_key(std::string_view key) {
  constexpr std::array<std::string_view, 9> keys{
      "summary.arm.jepa_mae_only.step_0.probe_area_fixed_seed_mean",
      "summary.arm.jepa_mae_only.step_0.family_multiscale_state_r2_fixed_seed_"
      "mean",
      "summary.arm.jepa_mae_only.step_0.family_order_regime_r2_fixed_seed_mean",
      "summary.arm.jepa_mae_only.step_0.family_cross_channel_r2_fixed_seed_"
      "mean",
      "summary.arm.jepa_mae_only.step_0.family_future_r2_fixed_seed_mean",
      "summary.arm.jepa_mae_only.step_0.geometry.channel_mean_effective_rank_"
      "ratio_fixed_seed_mean",
      "summary.arm.jepa_mae_only.step_0.geometry.channel_mean_participation_"
      "rank_ratio_fixed_seed_mean",
      "summary.arm.jepa_mae_only.step_0.geometry.channel_max_top_eigenvalue_"
      "share_fixed_seed_mean",
      "summary.arm.jepa_mae_only.step_0.geometry.channel_min_active_dimension_"
      "fraction_fixed_seed_mean",
  };
  return std::find(keys.begin(), keys.end(), key) != keys.end();
}

[[nodiscard]] inline std::string
canonical_key_bytes(const std::set<std::string, BytewiseLess> &keys) {
  std::string canonical;
  for (const std::string &key : keys) {
    canonical.append(key);
    canonical.push_back('\n');
  }
  return canonical;
}

[[nodiscard]] inline std::string
canonical_record_bytes(const std::vector<Record> &records) {
  std::vector<const Record *> ordered;
  ordered.reserve(records.size());
  for (const Record &record : records) {
    ordered.push_back(&record);
  }
  std::stable_sort(ordered.begin(), ordered.end(),
                   [](const Record *left, const Record *right) {
                     return BytewiseLess{}(left->key, right->key);
                   });

  std::string canonical;
  for (const Record *record : ordered) {
    canonical.append(record->key);
    canonical.push_back('=');
    canonical.append(record->value);
    canonical.push_back('\n');
  }
  return canonical;
}

inline void finalize(SelectionSummary &summary) {
  std::map<std::string, std::size_t, BytewiseLess> counts;
  for (const Record &record : summary.records) {
    ++counts[record.key];
  }
  for (const auto &[key, count] : counts) {
    summary.keys.insert(key);
    if (count > 1) {
      ++summary.duplicate_key_count;
      summary.duplicate_line_count += count - 1;
    }
  }
  summary.line_count = summary.records.size();
  summary.unique_key_count = summary.keys.size();
  summary.key_set_sha256 =
      cuwacunu::piaabo::digest::sha256_hex(canonical_key_bytes(summary.keys));
  summary.key_value_sha256 = cuwacunu::piaabo::digest::sha256_hex(
      canonical_record_bytes(summary.records));
}

[[nodiscard]] inline bool exact_schema(const std::vector<std::string> &values,
                                       const std::string &expected) {
  return values.size() == 1 && values.front() == expected;
}

[[nodiscard]] inline bool
selection_contract_pass(const SelectionSummary &summary,
                        const SelectionContract &contract) {
  return summary.line_count == contract.key_count &&
         summary.unique_key_count == contract.key_count &&
         summary.duplicate_key_count == 0 &&
         summary.duplicate_line_count == 0 &&
         summary.key_set_sha256 == contract.key_set_sha256 &&
         summary.key_value_sha256 == contract.key_value_sha256;
}

[[nodiscard]] inline SelectionAudit
audit_selection(const SelectionSummary &reference,
                const SelectionSummary &candidate,
                const SelectionContract &contract) {
  SelectionAudit result{};
  result.reference_contract_pass = selection_contract_pass(reference, contract);

  for (const std::string &key : reference.keys) {
    result.candidate_missing_key_count +=
        candidate.keys.count(key) == 0 ? 1 : 0;
  }
  for (const std::string &key : candidate.keys) {
    result.candidate_extra_key_count += reference.keys.count(key) == 0 ? 1 : 0;
  }
  result.candidate_key_set_exact = result.candidate_missing_key_count == 0 &&
                                   result.candidate_extra_key_count == 0;
  result.candidate_value_set_exact =
      candidate.key_value_sha256 == contract.key_value_sha256;
  result.candidate_contract_pass =
      candidate.line_count == contract.key_count &&
      candidate.unique_key_count == contract.key_count &&
      candidate.duplicate_key_count == 0 &&
      candidate.duplicate_line_count == 0 && result.candidate_key_set_exact &&
      candidate.key_set_sha256 == contract.key_set_sha256 &&
      result.candidate_value_set_exact;
  result.passed =
      result.reference_contract_pass && result.candidate_contract_pass;
  return result;
}

} // namespace detail

[[nodiscard]] inline bool is_accepted_key(std::string_view key) {
  return detail::accepted_seed_key(key) || detail::accepted_summary_key(key);
}

[[nodiscard]] inline bool is_legacy_raw_key(std::string_view key) {
  return detail::starts_with(key, "control.raw_equal_width.");
}

[[nodiscard]] inline LogSummary summarize(std::string_view raw) {
  LogSummary summary{};
  summary.size_bytes = raw.size();
  summary.file_sha256 = cuwacunu::piaabo::digest::sha256_hex(raw);
  summary.utf8_valid = detail::valid_utf8(raw);
  if (!summary.utf8_valid) {
    return summary;
  }

  std::size_t begin = 0;
  while (begin < raw.size()) {
    const std::size_t newline = raw.find('\n', begin);
    const std::size_t end =
        newline == std::string_view::npos ? raw.size() : newline;
    std::string_view line = raw.substr(begin, end - begin);
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    const std::size_t separator = line.find('=');
    if (separator != std::string_view::npos) {
      const std::string key(line.substr(0, separator));
      const std::string value(line.substr(separator + 1));
      if (key == "schema") {
        summary.schema_values.push_back(value);
      }
      if (is_accepted_key(key)) {
        summary.accepted.records.push_back({key, value});
      }
      if (is_legacy_raw_key(key)) {
        summary.legacy_raw.records.push_back({key, value});
      }
    }
    if (newline == std::string_view::npos) {
      break;
    }
    begin = newline + 1;
  }
  detail::finalize(summary.accepted);
  detail::finalize(summary.legacy_raw);
  return summary;
}

[[nodiscard]] inline const AuditContract &frozen_contract() {
  static const AuditContract contract{
      .reference_size_bytes = kReferenceSizeBytes,
      .reference_sha256 = std::string(kReferenceSha256),
      .reference_schema = std::string(kReferenceSchema),
      .candidate_schema = std::string(kCandidateSchema),
      .accepted =
          {
              .key_count = kAcceptedKeyCount,
              .key_set_sha256 = std::string(kAcceptedKeySetSha256),
              .key_value_sha256 = std::string(kAcceptedKeyValueSha256),
          },
      .legacy_raw =
          {
              .key_count = kLegacyRawKeyCount,
              .key_set_sha256 = std::string(kLegacyRawKeySetSha256),
              .key_value_sha256 = std::string(kLegacyRawKeyValueSha256),
          },
  };
  return contract;
}

[[nodiscard]] inline AuditResult
audit(std::string_view reference_raw, std::string_view candidate_raw,
      const AuditContract &contract = frozen_contract()) {
  AuditResult result{};
  result.reference = summarize(reference_raw);
  result.candidate = summarize(candidate_raw);
  result.reference_file_identity_pass =
      result.reference.utf8_valid &&
      result.reference.size_bytes == contract.reference_size_bytes &&
      result.reference.file_sha256 == contract.reference_sha256;
  result.reference_schema_pass =
      result.reference.utf8_valid &&
      detail::exact_schema(result.reference.schema_values,
                           contract.reference_schema);
  result.candidate_schema_pass =
      result.candidate.utf8_valid &&
      detail::exact_schema(result.candidate.schema_values,
                           contract.candidate_schema);
  result.accepted = detail::audit_selection(
      result.reference.accepted, result.candidate.accepted, contract.accepted);
  result.legacy_raw =
      detail::audit_selection(result.reference.legacy_raw,
                              result.candidate.legacy_raw, contract.legacy_raw);
  result.reference_contract_pass = result.reference_file_identity_pass &&
                                   result.reference_schema_pass &&
                                   result.accepted.reference_contract_pass &&
                                   result.legacy_raw.reference_contract_pass;
  result.candidate_contract_pass = result.candidate_schema_pass &&
                                   result.accepted.candidate_contract_pass &&
                                   result.legacy_raw.candidate_contract_pass;
  result.passed =
      result.reference_contract_pass && result.candidate_contract_pass;
  return result;
}

[[nodiscard]] inline AuditContract
fixture_contract(std::string_view reference_raw, std::string reference_schema,
                 std::string candidate_schema) {
  const LogSummary reference = summarize(reference_raw);
  if (!reference.utf8_valid || reference.schema_values.size() != 1 ||
      reference.schema_values.front() != reference_schema ||
      reference.accepted.unique_key_count == 0 ||
      reference.legacy_raw.unique_key_count == 0 ||
      reference.accepted.duplicate_line_count != 0 ||
      reference.legacy_raw.duplicate_line_count != 0) {
    throw std::invalid_argument("invalid reference-auditor fixture");
  }
  return {
      .reference_size_bytes = reference.size_bytes,
      .reference_sha256 = reference.file_sha256,
      .reference_schema = std::move(reference_schema),
      .candidate_schema = std::move(candidate_schema),
      .accepted =
          {
              .key_count = reference.accepted.unique_key_count,
              .key_set_sha256 = reference.accepted.key_set_sha256,
              .key_value_sha256 = reference.accepted.key_value_sha256,
          },
      .legacy_raw =
          {
              .key_count = reference.legacy_raw.unique_key_count,
              .key_set_sha256 = reference.legacy_raw.key_set_sha256,
              .key_value_sha256 = reference.legacy_raw.key_value_sha256,
          },
  };
}

[[nodiscard]] inline const char *failure_name(const AuditResult &result) {
  if (!result.reference.utf8_valid || !result.candidate.utf8_valid ||
      !result.reference_file_identity_pass || !result.reference_schema_pass ||
      !result.candidate_schema_pass) {
    return "invalid_numeric_or_mechanics";
  }
  if (!result.accepted.passed) {
    return "accepted_step_zero_reference_not_reproduced";
  }
  if (!result.legacy_raw.passed) {
    return "legacy_raw_reference_not_reproduced";
  }
  return "none";
}

} // namespace cuwacunu::tests::mtf_surface_sufficiency_map_reference_auditor
