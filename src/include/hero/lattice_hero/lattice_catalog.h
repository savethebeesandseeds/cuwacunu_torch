#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
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

struct wave_artifact_link_t {
  std::string aggregate_schema{"wave.sink.artifact_link.v1"};
  std::vector<std::string> artifact_ids{};
  std::vector<std::pair<std::string, double>> numeric_summary{};
  std::vector<std::pair<std::string, std::string>> text_summary{};
  std::string joined_kv_report{};
  std::string aggregate_sha256{};
};

struct wave_trial_t {
  std::string trial_id{};
  std::string cell_id{};
  std::uint64_t started_at_ms{0};
  std::uint64_t finished_at_ms{0};
  bool ok{false};
  std::string error{};
  std::uint64_t total_steps{0};
  std::string board_hash{};
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
  wave_artifact_link_t artifact_link{};
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

[[nodiscard]] std::string compute_coord_hash(std::string_view contract_hash,
                                             std::string_view wave_hash);
[[nodiscard]] std::string canonical_execution_profile_json(
    const wave_execution_profile_t& profile);
[[nodiscard]] std::string compute_profile_id(
    const wave_execution_profile_t& profile);
[[nodiscard]] std::string compute_cell_id(std::string_view contract_hash,
                                          std::string_view wave_hash,
                                          const wave_execution_profile_t& profile);

[[nodiscard]] bool encode_artifact_link_payload(
    const wave_artifact_link_t& artifact_link,
    std::string* out_payload,
    std::string* error = nullptr);
[[nodiscard]] bool decode_artifact_link_payload(std::string_view payload,
                                                wave_artifact_link_t* out,
                                                std::string* error = nullptr);

class lattice_catalog_store_t {
 public:
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
  [[nodiscard]] bool ingest_runtime_reports(const std::filesystem::path& store_root,
                                            std::string* error = nullptr);

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
  [[nodiscard]] bool provenance(std::string_view cell_id,
                                wave_artifact_link_t* out,
                                std::string* error = nullptr) const;
  [[nodiscard]] bool list_runtime_runs_by_binding(
      std::string_view contract_hashimyei, std::string_view wave_hashimyei,
      std::string_view binding_hashimyei,
      std::vector<cuwacunu::hero::hashimyei::run_manifest_t>* out,
      std::string* error = nullptr) const;
  [[nodiscard]] bool list_runtime_artifacts(
      std::string_view canonical_path, std::string_view schema, std::size_t limit,
      std::size_t offset, bool newest_first,
      std::vector<cuwacunu::hero::hashimyei::artifact_entry_t>* out,
      std::string* error = nullptr) const;

  [[nodiscard]] bool record_trial(const wave_cell_coord_t& coord,
                                  const wave_execution_profile_t& profile,
                                  const wave_trial_t& trial,
                                  const wave_artifact_link_t& artifact_link,
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
                                 std::string_view board_hash,
                                 std::string_view run_id,
                                 std::string* error);

  [[nodiscard]] bool record_cell_projection_(std::string_view cell_id,
                                             const wave_projection_t& projection,
                                             std::uint64_t ts_ms,
                                             std::string* error);
  [[nodiscard]] bool ingest_runtime_run_manifest_file_(
      const std::filesystem::path& path, std::string* error);
  [[nodiscard]] bool ingest_runtime_artifact_file_(const std::filesystem::path& path,
                                                   std::string* error);
  [[nodiscard]] bool runtime_ledger_contains_(std::string_view artifact_id,
                                              bool* out_exists,
                                              std::string* error);
  [[nodiscard]] bool append_runtime_ledger_(std::string_view artifact_id,
                                            std::string_view path,
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
  static constexpr idydb_column_row_sizing kColBoardHash = 24;
  static constexpr idydb_column_row_sizing kColRunId = 25;

  options_t options_{};
  idydb* db_{nullptr};

  std::unordered_map<std::string, wave_cell_t> cells_by_id_{};
  std::unordered_map<std::string, std::string> cell_id_by_coord_profile_{};
  std::unordered_map<std::string, std::vector<wave_trial_t>> trials_by_cell_{};
  std::unordered_map<std::string, wave_artifact_link_t> artifact_by_trial_id_{};
  std::unordered_map<std::string, std::unordered_map<std::string, double>>
      projection_num_by_cell_{};
  std::unordered_map<std::string,
                     std::unordered_map<std::string, std::string>>
      projection_txt_by_cell_{};
  std::unordered_map<std::string, cuwacunu::hero::hashimyei::run_manifest_t>
      runtime_runs_by_id_{};
  std::unordered_map<std::string, cuwacunu::hero::hashimyei::artifact_entry_t>
      runtime_artifacts_by_id_{};
  std::unordered_map<std::string, std::string> runtime_latest_artifact_by_key_{};
  std::unordered_map<std::string, std::vector<cuwacunu::hero::hashimyei::dependency_file_t>>
      runtime_provenance_by_run_id_{};
  std::unordered_set<std::string> runtime_ledger_{};
};

}  // namespace wave
}  // namespace hero
}  // namespace cuwacunu
