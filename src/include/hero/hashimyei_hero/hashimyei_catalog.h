#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "camahjucunu/db/idydb.h"
#include "hero/hashimyei_hero/family_rank.h"
#include "hero/hashimyei_hero/hashimyei_identity.h"
#include "hero/hashimyei_hero/hashimyei_schema.h"

namespace cuwacunu {
namespace hero {
namespace hashimyei {

struct dependency_file_t {
  std::string canonical_path{};
  std::string sha256_hex{};
};

struct component_instance_t {
  std::string canonical_path{};
  std::string tsi_type{};
  std::string hashimyei{};
};

struct wave_contract_binding_t {
  cuwacunu::hashimyei::hashimyei_t identity{};
  cuwacunu::hashimyei::hashimyei_t contract{};
  cuwacunu::hashimyei::hashimyei_t wave{};
  std::string binding_alias{};
};

struct run_manifest_t {
  std::string schema{cuwacunu::hashimyei::kRunManifestSchemaV2};
  std::string run_id{};
  std::uint64_t started_at_ms{0};
  cuwacunu::hashimyei::hashimyei_t campaign_identity{};
  wave_contract_binding_t wave_contract_binding{};
  std::string sampler{};
  std::string record_type{};
  std::string seed{};
  std::string device{};
  std::string dtype{};
  std::vector<dependency_file_t> dependency_files{};
  std::vector<component_instance_t> components{};
};

struct component_manifest_t {
  std::string schema{cuwacunu::hashimyei::kComponentManifestSchemaV2};
  std::string canonical_path{};
  std::string family{};
  std::string tsi_type{};
  cuwacunu::hashimyei::hashimyei_t component_identity{};
  std::optional<cuwacunu::hashimyei::hashimyei_t> parent_identity{};
  std::string revision_reason{"initial"};
  std::string config_revision_id{};
  wave_contract_binding_t wave_contract_binding{};
  std::string dsl_canonical_path{};
  std::string dsl_sha256_hex{};
  std::string status{"active"};
  std::string replaced_by{};
  std::uint64_t created_at_ms{0};
  std::uint64_t updated_at_ms{0};
};

struct component_state_t {
  std::string component_id{};
  std::uint64_t ts_ms{0};
  std::string manifest_path{};
  std::string report_fragment_sha256{};
  std::optional<std::uint64_t> family_rank{};
  component_manifest_t manifest{};
};

struct report_fragment_entry_t {
  std::string report_fragment_id{};
  std::string run_id{};
  std::string canonical_path{};
  std::string hashimyei{};
  std::string schema{};
  std::string report_fragment_sha256{};
  std::string path{};
  std::uint64_t ts_ms{0};
  std::string payload_json{};
};

struct report_fragment_snapshot_t {
  report_fragment_entry_t report_fragment{};
  std::vector<std::pair<std::string, double>> numeric_metrics{};
  std::vector<std::pair<std::string, std::string>> text_metrics{};
};

[[nodiscard]] std::string compute_run_id(
    const cuwacunu::hashimyei::hashimyei_t& campaign_identity,
    const wave_contract_binding_t& wave_contract_binding,
                                         std::uint64_t started_at_ms);

[[nodiscard]] bool load_run_manifest(const std::filesystem::path& path,
                                     run_manifest_t* out,
                                     std::string* error = nullptr);

[[nodiscard]] bool parse_component_manifest_kv(
    const std::unordered_map<std::string, std::string>& kv,
    component_manifest_t* out,
    std::string* error = nullptr);
[[nodiscard]] bool validate_component_manifest(const component_manifest_t& manifest,
                                               std::string* error = nullptr);
[[nodiscard]] bool load_component_manifest(const std::filesystem::path& path,
                                           component_manifest_t* out,
                                           std::string* error = nullptr);
[[nodiscard]] std::string compute_component_manifest_id(
    const component_manifest_t& manifest);

[[nodiscard]] bool save_run_manifest(const std::filesystem::path& store_root,
                                     const run_manifest_t& manifest,
                                     std::filesystem::path* out_path = nullptr,
                                     std::string* error = nullptr);

[[nodiscard]] bool parse_latent_lineage_state_payload(
    std::string_view payload, std::unordered_map<std::string, std::string>* out);

class hashimyei_catalog_store_t {
 public:
  struct options_t {
    std::filesystem::path catalog_path{};
    std::string passphrase{};
    bool encrypted{true};
    std::uint32_t ingest_version{1};
  };

  hashimyei_catalog_store_t() = default;
  ~hashimyei_catalog_store_t();

  hashimyei_catalog_store_t(const hashimyei_catalog_store_t&) = delete;
  hashimyei_catalog_store_t& operator=(const hashimyei_catalog_store_t&) = delete;

  [[nodiscard]] bool open(const options_t& options, std::string* error = nullptr);
  [[nodiscard]] bool close(std::string* error = nullptr);
  [[nodiscard]] bool opened() const noexcept { return db_ != nullptr; }
  [[nodiscard]] const options_t& options() const noexcept { return options_; }

  [[nodiscard]] bool ingest_filesystem(const std::filesystem::path& store_root,
                                       std::string* error = nullptr);
  [[nodiscard]] bool rebuild_indexes(std::string* error = nullptr);

  [[nodiscard]] bool get_run(std::string_view run_id, run_manifest_t* out,
                             std::string* error = nullptr) const;
  [[nodiscard]] bool list_runs_by_binding(std::string_view contract_hashimyei,
                                          std::string_view wave_hashimyei,
                                          std::string_view binding_hashimyei,
                                          std::vector<run_manifest_t>* out,
                                          std::string* error = nullptr) const;

  [[nodiscard]] bool latest_report_fragment(std::string_view canonical_path,
                                     std::string_view schema,
                                     report_fragment_entry_t* out,
                                     std::string* error = nullptr) const;
  [[nodiscard]] bool get_report_fragment(std::string_view report_fragment_id, report_fragment_entry_t* out,
                                  std::string* error = nullptr) const;
  [[nodiscard]] bool report_fragment_metrics(
      std::string_view report_fragment_id,
      std::vector<std::pair<std::string, double>>* out_numeric,
      std::vector<std::pair<std::string, std::string>>* out_text,
      std::string* error = nullptr) const;
  [[nodiscard]] bool list_report_fragments(
      std::string_view canonical_path, std::string_view schema,
      std::size_t limit, std::size_t offset, bool newest_first,
      std::vector<report_fragment_entry_t>* out, std::string* error = nullptr) const;
  [[nodiscard]] bool resolve_component(std::string_view canonical_path,
                                       std::string_view hashimyei,
                                       component_state_t* out,
                                       std::string* error = nullptr) const;
  [[nodiscard]] bool list_components(
      std::string_view canonical_path, std::string_view hashimyei,
      std::size_t limit, std::size_t offset, bool newest_first,
      std::vector<component_state_t>* out, std::string* error = nullptr) const;
  [[nodiscard]] bool list_components_by_binding(
      std::string_view contract_hashimyei, std::string_view wave_hashimyei,
      std::string_view binding_hashimyei, std::size_t limit, std::size_t offset,
      bool newest_first, std::vector<component_state_t>* out,
      std::string* error = nullptr) const;
  [[nodiscard]] bool resolve_active_hashimyei(std::string_view canonical_path,
                                              std::string_view family,
                                              std::string* out_hashimyei,
                                              std::string* error = nullptr) const;
  [[nodiscard]] bool get_explicit_family_rank(
      std::string_view family, std::string_view contract_hash,
      cuwacunu::hero::family_rank::state_t* out,
      std::string* error = nullptr) const;
  [[nodiscard]] bool get_family_rank(
      std::string_view family, std::string_view contract_hash,
      cuwacunu::hero::family_rank::state_t* out,
      std::string* error = nullptr) const;
  [[nodiscard]] bool resolve_ranked_hashimyei(std::string_view family,
                                              std::string_view contract_hash,
                                              std::uint64_t rank,
                                              std::string* out_hashimyei,
                                              std::string* error = nullptr) const;
  [[nodiscard]] bool register_component_manifest(
      const component_manifest_t& manifest, std::string* out_component_id = nullptr,
      bool* out_inserted = nullptr, std::string* error = nullptr);
  [[nodiscard]] bool report_fragment_snapshot(std::string_view canonical_path,
                                       std::string_view run_id,
                                       report_fragment_snapshot_t* out,
                                       std::string* error = nullptr) const;
  [[nodiscard]] bool dependency_trace(std::string_view report_fragment_id,
                                      std::vector<dependency_file_t>* out,
                                      std::string* error = nullptr) const;

 private:
  struct ingest_lock_t {
    std::filesystem::path path{};
    bool held{false};
  };

  [[nodiscard]] bool ensure_catalog_header_(std::string* error);
  [[nodiscard]] bool append_row_(std::string_view record_kind,
                                 std::string_view record_id,
                                 std::string_view run_id,
                                 std::string_view canonical_path,
                                 std::string_view hashimyei,
                                 std::string_view schema,
                                 std::string_view metric_key, double metric_num,
                                 std::string_view metric_txt,
                                 std::string_view report_fragment_sha256,
                                 std::string_view path, std::string_view ts_ms,
                                 std::string_view payload_json,
                                 std::string* error);
  [[nodiscard]] bool ledger_contains_(std::string_view report_fragment_sha256,
                                      bool* out_exists, std::string* error);
  [[nodiscard]] bool append_ledger_(std::string_view report_fragment_sha256,
                                    std::string_view path, std::string* error);
  [[nodiscard]] bool append_kind_counter_(
      cuwacunu::hashimyei::hashimyei_kind_e kind, std::uint64_t next_value,
      std::string* error);
  [[nodiscard]] bool reserve_next_ordinal_(
      cuwacunu::hashimyei::hashimyei_kind_e kind, std::uint64_t* out_ordinal,
      std::string* error);
  [[nodiscard]] bool ensure_identity_allocated_(
      cuwacunu::hashimyei::hashimyei_t* identity, std::string* error);

  [[nodiscard]] bool ingest_run_manifest_file_(const std::filesystem::path& path,
                                               std::string* error);
  [[nodiscard]] bool ingest_component_manifest_file_(
      const std::filesystem::path& path,
      std::string* error);
  [[nodiscard]] bool ingest_report_fragment_file_(const std::filesystem::path& path,
                                                  std::string* error);
  [[nodiscard]] bool parse_and_append_metrics_(
      std::string_view report_fragment_id,
      const std::unordered_map<std::string, std::string>& kv,
      std::string* error);
  [[nodiscard]] bool acquire_ingest_lock_(const std::filesystem::path& store_root,
                                          ingest_lock_t* lock,
                                          std::string* error);
  void release_ingest_lock_(ingest_lock_t* lock);

  static constexpr idydb_column_row_sizing kColRecordKind = 1;
  static constexpr idydb_column_row_sizing kColRecordId = 2;
  static constexpr idydb_column_row_sizing kColRunId = 3;
  static constexpr idydb_column_row_sizing kColCanonicalPath = 4;
  static constexpr idydb_column_row_sizing kColHashimyei = 5;
  static constexpr idydb_column_row_sizing kColSchema = 6;
  static constexpr idydb_column_row_sizing kColMetricKey = 7;
  static constexpr idydb_column_row_sizing kColMetricNum = 8;
  static constexpr idydb_column_row_sizing kColMetricTxt = 9;
  static constexpr idydb_column_row_sizing kColReportFragmentSha256 = 10;
  static constexpr idydb_column_row_sizing kColPath = 11;
  static constexpr idydb_column_row_sizing kColTsMs = 12;
  static constexpr idydb_column_row_sizing kColPayload = 13;

  options_t options_{};
  idydb* db_{nullptr};

  std::unordered_map<std::string, run_manifest_t> runs_by_id_{};
  std::unordered_map<std::string, report_fragment_entry_t> report_fragments_by_id_{};
  std::unordered_map<std::string, std::string> latest_report_fragment_by_key_{};
  std::unordered_map<std::string, std::vector<std::pair<std::string, double>>>
      metrics_num_by_report_fragment_{};
  std::unordered_map<std::string,
                     std::vector<std::pair<std::string, std::string>>>
      metrics_txt_by_report_fragment_{};
  std::unordered_map<std::string, component_state_t> components_by_id_{};
  std::unordered_map<std::string, std::string> latest_component_by_canonical_{};
  std::unordered_map<std::string, std::string> latest_component_by_hashimyei_{};
  std::unordered_map<std::string, std::string> active_hashimyei_by_key_{};
  std::unordered_map<std::string, std::string> active_component_by_key_{};
  std::unordered_map<std::string, std::string> active_component_by_canonical_{};
  std::unordered_map<std::string, std::vector<dependency_file_t>>
      dependency_files_by_run_id_{};
  std::unordered_map<int, std::uint64_t> kind_counters_{};
  std::unordered_map<std::string, cuwacunu::hashimyei::hashimyei_t>
      hash_identity_by_kind_sha_{};
  std::unordered_map<std::string, std::vector<std::string>>
      component_ids_by_binding_hashimyei_{};
  std::unordered_map<std::string, cuwacunu::hero::family_rank::state_t>
      explicit_family_rank_by_scope_{};
};

}  // namespace hashimyei
}  // namespace hero
}  // namespace cuwacunu
