#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "camahjucunu/db/idydb.h"
#include "hero/hashimyei_hero/hashimyei_catalog.h"

namespace cuwacunu {
namespace hero {
namespace wave {

struct wave_cell_coord_t {
  std::string contract_hash{};
  std::string wave_hash{};
};

struct wave_execution_profile_t {
  std::string binding_id{};
  std::string wave_id{};
  std::string device{"cpu"};
  std::string sampler{};
  std::string record_type{};
  std::string dtype{};
  std::string seed{};
  std::string determinism_policy{"non_deterministic"};
};

struct lattice_cell_report_t {
  std::string report_schema{"wave.cell.report.v1"};
  std::vector<std::string> source_report_fragment_ids{};
  std::vector<std::pair<std::string, double>> summary_num{};
  std::vector<std::pair<std::string, std::string>> summary_txt{};
  std::string report_lls{};
  std::string report_sha256{};
};

struct wave_trial_t {
  std::string trial_id{};
  std::string cell_id{};
  std::uint64_t started_at_ms{0};
  std::uint64_t finished_at_ms{0};
  bool ok{false};
  std::string error{};
  std::uint64_t total_steps{0};
  std::string campaign_hash{};
  std::string run_id{};
  std::string state_snapshot_id{};
};

struct wave_cell_t {
  std::string cell_id{};
  wave_cell_coord_t coord{};
  wave_execution_profile_t execution_profile{};
  std::string state{"pending"};
  std::size_t trial_count{0};
  std::string last_trial_id{};
  lattice_cell_report_t report{};
  std::uint32_t projection_version{2};
  std::uint64_t created_at_ms{0};
  std::uint64_t updated_at_ms{0};
};

struct wave_projection_t {
  std::uint32_t projection_version{2};
  std::string projector_build_id{"wave.projector.v2"};
  std::string projection_lls{};
  std::vector<std::pair<std::string, double>> projection_num{};
  std::vector<std::pair<std::string, std::string>> projection_txt{};
};

struct matrix_query_t {
  std::string contract_hash{};
  std::string wave_hash{};
  std::string state_snapshot_id{};
  std::vector<std::pair<std::string, double>> projection_num_eq{};
  std::vector<std::pair<std::string, std::string>> projection_txt_eq{};
  std::size_t limit{64};
  std::size_t offset{0};
  bool newest_first{true};
  bool latest_success_only{true};
};

struct runtime_report_fragment_t {
  std::string report_fragment_id{};
  std::string run_id{};
  std::string canonical_path{};
  std::string family{};
  std::string hashimyei{};
  std::string contract_hash{};
  std::string schema{};
  std::string report_fragment_sha256{};
  std::string path{};
  std::uint64_t ts_ms{0};
  std::uint64_t wave_cursor{0};
  std::optional<std::uint64_t> family_rank{};
  std::string intersection_cursor{};
  std::string payload_json{};
};

struct runtime_view_report_t {
  std::string view_kind{};
  std::string canonical_path{};
  std::string selector_hashimyei_cursor{};
  std::string selector_intersection_cursor{};
  std::string contract_hash{};
  std::uint64_t wave_cursor{0};
  bool has_wave_cursor{false};
  std::size_t match_count{0};
  std::size_t ambiguity_count{0};
  std::string view_lls{};
};

struct runtime_intersection_cursor_t {
  std::string hashimyei_cursor{};
  std::uint64_t wave_cursor{0};
};

[[nodiscard]] std::string compute_coord_hash(std::string_view contract_hash,
                                             std::string_view wave_hash);
[[nodiscard]] std::string canonical_execution_profile_json(
    const wave_execution_profile_t& profile);
[[nodiscard]] std::string compute_profile_id(
    const wave_execution_profile_t& profile);
[[nodiscard]] std::string compute_cell_id(std::string_view contract_hash,
                                          std::string_view wave_hash,
                                          const wave_execution_profile_t& profile);

[[nodiscard]] bool encode_cell_report_payload(
    const lattice_cell_report_t& report,
    std::string* out_payload,
    std::string* error = nullptr);
[[nodiscard]] bool decode_cell_report_payload(std::string_view payload,
                                              lattice_cell_report_t* out,
                                              std::string* error = nullptr);

class lattice_catalog_store_t {
 public:
  [[nodiscard]] static std::string trim_runtime_wave_cursor_token(
      std::string_view text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end &&
           std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
      ++begin;
    }
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
      --end;
    }
    return std::string(text.substr(begin, end - begin));
  }

  [[nodiscard]] static bool parse_runtime_wave_cursor_scalar(
      std::string_view text, std::uint64_t* out) noexcept {
    if (!out) return false;
    const std::string token = trim_runtime_wave_cursor_token(text);
    if (token.empty()) return false;

    int base = 10;
    const char* begin = token.data();
    const char* end = token.data() + token.size();
    if (token.size() > 2 && token[0] == '0' &&
        (token[1] == 'x' || token[1] == 'X')) {
      base = 16;
      begin += 2;
      if (begin == end) return false;
    }

    std::uint64_t parsed = 0;
    const auto [ptr, ec] = std::from_chars(begin, end, parsed, base);
    if (ec != std::errc{} || ptr != end) return false;
    *out = parsed;
    return true;
  }

  [[nodiscard]] static constexpr db::wave_cursor::layout_t
  runtime_wave_cursor_layout() noexcept {
    return db::wave_cursor::layout_t{
        .run_bits = 41,
        .episode_bits = 11,
        .batch_bits = 11,
    };
  }

  [[nodiscard]] static constexpr std::uint64_t
  runtime_wave_cursor_full_mask() noexcept {
    db::wave_cursor::masked_query_t q{};
    const db::wave_cursor::parts_t zero{};
    const auto layout = runtime_wave_cursor_layout();
    (void)db::wave_cursor::build_masked_query(
        layout,
        zero,
        static_cast<std::uint8_t>(
            db::wave_cursor::field_run | db::wave_cursor::field_episode |
            db::wave_cursor::field_batch),
        &q);
    return q.mask;
  }

  [[nodiscard]] static bool unpack_runtime_wave_cursor(
      std::uint64_t packed, db::wave_cursor::parts_t* out_parts) noexcept {
    return db::wave_cursor::unpack(runtime_wave_cursor_layout(), packed, out_parts);
  }

  [[nodiscard]] static bool parse_runtime_wave_cursor_token(
      std::string_view text, std::uint64_t* out) noexcept {
    if (!out) return false;
    const std::string token = trim_runtime_wave_cursor_token(text);
    if (token.empty()) return false;
    if (token.find('.') == std::string::npos &&
        token.find(',') == std::string::npos) {
      return parse_runtime_wave_cursor_scalar(token, out);
    }

    const std::size_t first_dot = token.find('.');
    if (first_dot == std::string::npos) return false;

    std::size_t second_sep = std::string::npos;
    if (const std::size_t second_dot = token.find('.', first_dot + 1);
        second_dot != std::string::npos) {
      second_sep = second_dot;
    } else if (const std::size_t comma = token.find(',', first_dot + 1);
               comma != std::string::npos) {
      second_sep = comma;
    } else {
      return false;
    }
    if (token.find('.', second_sep + 1) != std::string::npos) return false;
    if (token.find(',', second_sep + 1) != std::string::npos) return false;

    std::uint64_t run_id = 0;
    std::uint64_t episode_k = 0;
    std::uint64_t batch_j = 0;
    if (!parse_runtime_wave_cursor_scalar(token.substr(0, first_dot), &run_id)) {
      return false;
    }
    if (!parse_runtime_wave_cursor_scalar(
            token.substr(first_dot + 1, second_sep - first_dot - 1),
            &episode_k)) {
      return false;
    }
    if (!parse_runtime_wave_cursor_scalar(token.substr(second_sep + 1),
                                          &batch_j)) {
      return false;
    }

    const db::wave_cursor::parts_t parts{
        .run_id = run_id,
        .episode_k = episode_k,
        .batch_j = batch_j,
    };
    return db::wave_cursor::pack(runtime_wave_cursor_layout(), parts, out);
  }

  [[nodiscard]] static std::string format_runtime_wave_cursor(
      std::uint64_t packed) {
    db::wave_cursor::parts_t parts{};
    if (!unpack_runtime_wave_cursor(packed, &parts)) {
      return std::to_string(packed);
    }
    return std::to_string(parts.run_id) + "." +
           std::to_string(parts.episode_k) + "." +
           std::to_string(parts.batch_j);
  }

  [[nodiscard]] static std::string normalize_runtime_hashimyei_cursor(
      std::string_view canonical_path) {
    const auto trim_ascii_local = [](std::string_view in) {
      std::size_t begin = 0;
      std::size_t end = in.size();
      while (begin < end &&
             std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
        ++begin;
      }
      while (end > begin &&
             std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
        --end;
      }
      return std::string(in.substr(begin, end - begin));
    };
    return trim_ascii_local(canonical_path);
  }
  [[nodiscard]] static bool parse_runtime_intersection_cursor(
      std::string_view intersection_cursor,
      runtime_intersection_cursor_t* out,
      std::string* error = nullptr) {
    if (error) error->clear();
    if (!out) {
      if (error) *error = "intersection cursor output pointer is null";
      return false;
    }
    *out = runtime_intersection_cursor_t{};
    const std::string token = trim_runtime_wave_cursor_token(intersection_cursor);
    if (token.empty()) {
      if (error) *error = "intersection_cursor is empty";
      return false;
    }
    const std::size_t sep = token.rfind('|');
    if (sep == std::string::npos || sep == 0 || sep + 1 >= token.size()) {
      if (error) {
        *error =
            "intersection_cursor must be formatted as <canonical_path>|<wave_cursor>";
      }
      return false;
    }
    if (token.find('|', sep + 1) != std::string::npos) {
      if (error) *error = "intersection_cursor has too many separators";
      return false;
    }
    const std::string hashimyei_cursor =
        normalize_runtime_hashimyei_cursor(token.substr(0, sep));
    if (hashimyei_cursor.empty()) {
      if (error) *error = "intersection_cursor is missing canonical_path";
      return false;
    }
    std::uint64_t wave_cursor = 0;
    if (!parse_runtime_wave_cursor_token(token.substr(sep + 1), &wave_cursor)) {
      if (error) {
        *error =
            "intersection_cursor wave_cursor is not valid; expected integer or <run>.<epoch>.<batch>";
      }
      return false;
    }
    out->hashimyei_cursor = hashimyei_cursor;
    out->wave_cursor = wave_cursor;
    return true;
  }
  [[nodiscard]] static bool runtime_hashimyei_cursor_matches(
      std::string_view query_canonical_path,
      std::string_view fragment_canonical_path) {
    const auto is_hashimyei_hex_token_local = [](std::string_view token) {
      if (token.size() < 3) return false;
      if (token[0] != '0' || token[1] != 'x') return false;
      for (std::size_t i = 2; i < token.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(token[i]);
        const bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        if (!hex) return false;
      }
      return true;
    };
    const auto dot_count_local = [](std::string_view value) {
      return static_cast<std::size_t>(
          std::count(value.begin(), value.end(), '.'));
    };
    const std::string query = normalize_runtime_hashimyei_cursor(query_canonical_path);
    if (query.empty()) return true;
    const std::string fragment =
        normalize_runtime_hashimyei_cursor(fragment_canonical_path);
    if (fragment == query) return true;
    if (fragment.size() <= query.size() ||
        fragment.compare(0, query.size(), query) != 0 ||
        fragment[query.size()] != '.') {
      return false;
    }
    const std::string_view suffix(fragment.data() + query.size() + 1,
                                  fragment.size() - query.size() - 1);
    if (suffix.empty() || suffix.find('.') != std::string_view::npos) {
      return false;
    }
    if (is_hashimyei_hex_token_local(suffix)) return true;
    return query.rfind("tsi.source.", 0) == 0 && dot_count_local(query) == 2;
  }

  struct options_t {
    std::filesystem::path catalog_path{};
    std::string passphrase{};
    bool encrypted{true};
    std::uint32_t projection_version{2};
  };

  lattice_catalog_store_t() = default;
  ~lattice_catalog_store_t();

  lattice_catalog_store_t(const lattice_catalog_store_t&) = delete;
  lattice_catalog_store_t& operator=(const lattice_catalog_store_t&) = delete;

  [[nodiscard]] bool open(const options_t& options, std::string* error = nullptr);
  [[nodiscard]] bool close(std::string* error = nullptr);
  [[nodiscard]] bool opened() const noexcept { return db_ != nullptr; }
  [[nodiscard]] const options_t& options() const noexcept { return options_; }

  [[nodiscard]] bool rebuild_indexes(std::string* error = nullptr);
  [[nodiscard]] bool ingest_runtime_report_fragments(
      const std::filesystem::path& store_root, std::string* error = nullptr);

  [[nodiscard]] bool resolve_cell(const wave_cell_coord_t& coord,
                                  const wave_execution_profile_t& profile,
                                  wave_cell_t* out,
                                  std::string* error = nullptr) const;
  [[nodiscard]] bool get_cell(std::string_view cell_id,
                              wave_cell_t* out,
                              std::string* error = nullptr) const;
  [[nodiscard]] bool list_trials(std::string_view cell_id,
                                 std::size_t limit,
                                 std::size_t offset,
                                 bool newest_first,
                                 std::vector<wave_trial_t>* out,
                                 std::string* error = nullptr) const;
  [[nodiscard]] bool query_matrix(const matrix_query_t& query,
                                  std::vector<wave_cell_t>* out,
                                  std::string* error = nullptr) const;
  [[nodiscard]] bool get_cell_report(
      std::string_view cell_id, lattice_cell_report_t* out,
      std::string* error = nullptr) const;
  [[nodiscard]] bool list_runtime_runs_by_binding(
      std::string_view contract_hashimyei, std::string_view wave_hashimyei,
      std::string_view binding_hashimyei,
      std::vector<cuwacunu::hero::hashimyei::run_manifest_t>* out,
      std::string* error = nullptr) const;
  [[nodiscard]] bool list_runtime_report_fragments(
      std::string_view canonical_path, std::string_view schema, std::size_t limit,
      std::size_t offset, bool newest_first,
      std::vector<runtime_report_fragment_t>* out,
      std::string* error = nullptr) const;
  [[nodiscard]] bool latest_runtime_report_fragment(
      std::string_view canonical_path, std::string_view schema,
      runtime_report_fragment_t* out, std::string* error = nullptr) const;
  [[nodiscard]] bool get_runtime_report_fragment(
      std::string_view report_fragment_id, runtime_report_fragment_t* out,
      std::string* error = nullptr) const;
  [[nodiscard]] bool list_runtime_report_schemas(
      std::string_view canonical_path, std::vector<std::string>* out,
      std::string* error = nullptr) const;
  [[nodiscard]] bool get_runtime_view_lls(
      std::string_view view_kind, std::string_view intersection_cursor,
      std::uint64_t wave_cursor, bool use_wave_cursor,
      std::string_view contract_hash, runtime_view_report_t* out,
      std::string* error = nullptr) const;
  [[nodiscard]] bool get_explicit_family_rank(
      std::string_view family, std::string_view contract_hash,
      cuwacunu::hero::family_rank::state_t* out,
      std::string* error = nullptr) const;
  [[nodiscard]] bool get_family_rank(
      std::string_view family, std::string_view contract_hash,
      cuwacunu::hero::family_rank::state_t* out,
      std::string* error = nullptr) const;

  [[nodiscard]] bool record_trial(const wave_cell_coord_t& coord,
                                  const wave_execution_profile_t& profile,
                                  const wave_trial_t& trial,
                                  const lattice_cell_report_t& report,
                                  const wave_projection_t& projection,
                                  wave_cell_t* out_cell,
                                  std::string* error = nullptr);

 private:
  [[nodiscard]] bool ensure_catalog_header_(std::string* error);
  [[nodiscard]] bool append_row_(std::string_view record_kind,
                                 std::string_view record_id,
                                 std::string_view cell_id,
                                 std::string_view contract_hash,
                                 std::string_view wave_hash,
                                 std::string_view profile_id,
                                 std::string_view execution_profile_json,
                                 std::string_view state_txt,
                                 double metric_num,
                                 std::string_view text_a,
                                 std::string_view text_b,
                                 std::string_view projection_version,
                                 std::string_view ts_ms,
                                 std::string_view payload_json,
                                 std::string_view projection_key,
                                 double projection_num,
                                 std::string_view projection_txt,
                                 std::string_view projection_key_aux,
                                 std::string_view projection_txt_aux,
                                 std::string_view started_at_ms,
                                 std::string_view finished_at_ms,
                                 std::string_view ok_txt,
                                 std::string_view total_steps,
                                 std::string_view campaign_hash,
                                 std::string_view run_id,
                                 std::string* error);

  [[nodiscard]] bool record_cell_projection_(std::string_view cell_id,
                                             const wave_projection_t& projection,
                                             std::uint64_t ts_ms,
                                             std::string* error);
  [[nodiscard]] bool ingest_runtime_run_manifest_file_(
      const std::filesystem::path& path, std::string* error);
  [[nodiscard]] bool ingest_runtime_component_manifest_file_(
      const std::filesystem::path& path, std::string* error);
  [[nodiscard]] bool ingest_runtime_report_fragment_file_(
      const std::filesystem::path& path, std::string* error);
  [[nodiscard]] bool runtime_ledger_contains_(
      std::string_view report_fragment_id, bool* out_exists,
      std::string* error);
  [[nodiscard]] bool append_runtime_ledger_(
      std::string_view report_fragment_id, std::string_view path,
      std::string* error);

  [[nodiscard]] static std::string coord_profile_key_(
      std::string_view contract_hash,
      std::string_view wave_hash,
      std::string_view profile_id);

  static constexpr idydb_column_row_sizing kColRecordKind = 1;
  static constexpr idydb_column_row_sizing kColRecordId = 2;
  static constexpr idydb_column_row_sizing kColCellId = 3;
  static constexpr idydb_column_row_sizing kColContractHash = 4;
  static constexpr idydb_column_row_sizing kColWaveHash = 5;
  static constexpr idydb_column_row_sizing kColProfileId = 6;
  static constexpr idydb_column_row_sizing kColExecutionProfileJson = 7;
  static constexpr idydb_column_row_sizing kColStateTxt = 8;
  static constexpr idydb_column_row_sizing kColMetricNum = 9;
  static constexpr idydb_column_row_sizing kColTextA = 10;
  static constexpr idydb_column_row_sizing kColTextB = 11;
  static constexpr idydb_column_row_sizing kColProjectionVersion = 12;
  static constexpr idydb_column_row_sizing kColTsMs = 13;
  static constexpr idydb_column_row_sizing kColPayload = 14;
  static constexpr idydb_column_row_sizing kColProjectionKey = 15;
  static constexpr idydb_column_row_sizing kColProjectionNum = 16;
  static constexpr idydb_column_row_sizing kColProjectionTxt = 17;
  static constexpr idydb_column_row_sizing kColProjectionKeyAux = 18;
  static constexpr idydb_column_row_sizing kColProjectionTxtAux = 19;
  static constexpr idydb_column_row_sizing kColStartedAtMs = 20;
  static constexpr idydb_column_row_sizing kColFinishedAtMs = 21;
  static constexpr idydb_column_row_sizing kColOkTxt = 22;
  static constexpr idydb_column_row_sizing kColTotalSteps = 23;
  static constexpr idydb_column_row_sizing kColCampaignHash = 24;
  static constexpr idydb_column_row_sizing kColRunId = 25;

  options_t options_{};
  idydb* db_{nullptr};

  std::unordered_map<std::string, wave_cell_t> cells_by_id_{};
  std::unordered_map<std::string, std::string> cell_id_by_coord_profile_{};
  std::unordered_map<std::string, std::vector<wave_trial_t>> trials_by_cell_{};
  std::unordered_map<std::string, lattice_cell_report_t> report_by_trial_id_{};
  std::unordered_map<std::string, std::unordered_map<std::string, double>>
      projection_num_by_cell_{};
  std::unordered_map<std::string,
                     std::unordered_map<std::string, std::string>>
      projection_txt_by_cell_{};
  std::unordered_map<std::string, cuwacunu::hero::hashimyei::run_manifest_t>
      runtime_runs_by_id_{};
  std::unordered_map<std::string, cuwacunu::hero::hashimyei::component_state_t>
      runtime_components_by_id_{};
  std::unordered_map<std::string, runtime_report_fragment_t>
      runtime_report_fragments_by_id_{};
  std::unordered_map<std::string, std::string>
      runtime_latest_report_fragment_by_key_{};
  std::unordered_map<std::string,
                     std::vector<cuwacunu::hero::hashimyei::dependency_file_t>>
      runtime_dependency_files_by_run_id_{};
  std::unordered_map<std::uint64_t, std::vector<std::string>>
      runtime_report_fragment_ids_by_wave_cursor_{};
  std::unordered_set<std::string> runtime_ledger_{};
  std::unordered_map<std::string, cuwacunu::hero::family_rank::state_t>
      explicit_family_rank_by_scope_{};
};

}  // namespace wave
}  // namespace hero
}  // namespace cuwacunu
