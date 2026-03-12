#include <hero/lattice_hero/lattice_catalog.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;
using cuwacunu::hero::wave::compute_cell_id;
using cuwacunu::hero::wave::compute_profile_id;
using cuwacunu::hero::wave::decode_artifact_link_payload;
using cuwacunu::hero::wave::encode_artifact_link_payload;
using cuwacunu::hero::wave::matrix_query_t;
using cuwacunu::hero::wave::wave_artifact_link_t;
using cuwacunu::hero::wave::lattice_catalog_store_t;
using cuwacunu::hero::wave::wave_cell_coord_t;
using cuwacunu::hero::wave::wave_cell_t;
using cuwacunu::hero::wave::wave_execution_profile_t;
using cuwacunu::hero::wave::wave_projection_t;
using cuwacunu::hero::wave::wave_trial_t;

static void require_impl(bool ok, const char* expr, const char* file, int line) {
  if (!ok) {
    std::cerr << "[TEST FAIL] " << file << ":" << line << "  (" << expr << ")\n";
    std::exit(1);
  }
}
#define REQUIRE(x) require_impl((x), #x, __FILE__, __LINE__)

[[nodiscard]] static bool find_axis_num(const wave_projection_t& projection,
                                        std::string_view key, double* out) {
  if (!out) return false;
  for (const auto& [k, v] : projection.axis_num) {
    if (k == key) {
      *out = v;
      return true;
    }
  }
  return false;
}

[[nodiscard]] static double axis_num_must(const wave_projection_t& projection,
                                          std::string_view key) {
  double out = 0.0;
  REQUIRE(find_axis_num(projection, key, &out));
  return out;
}

static std::string random_suffix() {
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dist;
  const uint64_t v = dist(gen);
  return std::to_string(static_cast<unsigned long long>(getpid())) + "_" +
         std::to_string(static_cast<unsigned long long>(v));
}

struct temp_dir_t {
  fs::path dir{};
  temp_dir_t() {
    dir = fs::temp_directory_path() / ("test_lattice_catalog_" + random_suffix());
    fs::create_directories(dir);
  }
  ~temp_dir_t() {
    std::error_code ec;
    fs::remove_all(dir, ec);
  }
};

static void write_text_file(const fs::path& path, std::string_view payload) {
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  REQUIRE(static_cast<bool>(out));
  out << payload;
  REQUIRE(static_cast<bool>(out));
}

int main() {
  temp_dir_t tmp{};
  const fs::path catalog_path = tmp.dir / "catalog" / "lattice_catalog.idydb";
  constexpr const char* kPass = "wave-catalog-test-pass";

  wave_cell_coord_t coord{};
  coord.contract_hash = "contract_hash_123";
  coord.wave_hash = "wave_hash_456";

  wave_execution_profile_t profile{};
  profile.binding_id = "bind.train.vicreg";
  profile.wave_id = "wave.train.main";
  profile.device = "cpu";
  profile.sampler = "sequential";
  profile.record_type = "kline";
  profile.dtype = "float32";
  profile.seed = "42";
  profile.determinism_policy = "deterministic";

  const std::string profile_id_1 = compute_profile_id(profile);
  const std::string profile_id_2 = compute_profile_id(profile);
  REQUIRE(!profile_id_1.empty());
  REQUIRE(profile_id_1 == profile_id_2);

  const std::string cell_id_1 = compute_cell_id(coord.contract_hash, coord.wave_hash,
                                                profile);
  const std::string cell_id_2 = compute_cell_id(coord.contract_hash, coord.wave_hash,
                                                profile);
  REQUIRE(!cell_id_1.empty());
  REQUIRE(cell_id_1 == cell_id_2);

  wave_artifact_link_t artifact_link{};
  artifact_link.aggregate_schema = "wave.sink.artifact_link.v1";
  artifact_link.artifact_ids = {"a1", "a2"};
  artifact_link.numeric_summary = {{"loss", 0.123}, {"accuracy", 0.99}};
  artifact_link.text_summary = {{"status", "ok"}, {"schema", "report.v1"}};
  artifact_link.joined_kv_report =
      "schema=wave.report.v1\n"
      "run_id=run_runtime_001\n"
      "source.runtime.projection.run_id=run_runtime_001\n"
      "source.runtime.projection.schema=wave.source.runtime.projection.v2\n"
      "source.runtime.symbol=BTCUSDT\n"
      "source.runtime.request.from_ratio=0.125\n";
  artifact_link.aggregate_sha256 = "deadbeef";

  std::string encoded{};
  std::string error;
  REQUIRE(encode_artifact_link_payload(artifact_link, &encoded, &error));
  REQUIRE(!encoded.empty());

  wave_artifact_link_t decoded{};
  REQUIRE(decode_artifact_link_payload(encoded, &decoded, &error));
  REQUIRE(decoded.aggregate_schema == artifact_link.aggregate_schema);
  REQUIRE(decoded.artifact_ids.size() == artifact_link.artifact_ids.size());
  REQUIRE(decoded.joined_kv_report == artifact_link.joined_kv_report);

  lattice_catalog_store_t catalog{};
  lattice_catalog_store_t::options_t opts{};
  opts.catalog_path = catalog_path;
  opts.passphrase = kPass;
  opts.encrypted = true;
  opts.projection_version = 2;
  REQUIRE(catalog.open(opts, &error));

  wave_trial_t trial{};
  trial.trial_id = "trial_001";
  trial.started_at_ms = 1711111000000ULL;
  trial.finished_at_ms = 1711111000123ULL;
  trial.ok = true;
  trial.total_steps = 64;
  trial.board_hash = "board_hash_abc";
  trial.run_id = "run_hash_001";
  trial.state_snapshot_id = "snapshot_state_001";

  wave_projection_t projection{};
  projection.projection_version = 2;
  projection.projector_build_id = "wave.projector.v2";
  projection.axis_num = {
      {"epochs", 10.0},
      {"source_count", 1.0},
      {"source.runtime.request.from_ratio", 0.125},
      {"source.runtime.request.to_ratio", 0.5},
      {"source.runtime.request.span_ratio", 0.375},
      {"source.runtime.effective.coverage_ratio", 0.375},
      {"source.runtime.flags.clipped_left", 0.0},
      {"source.runtime.flags.clipped_right", 0.0},
      {"source.channels.active_count", 2.0},
      {"source.channels.total_count", 3.0},
      {"source.channels.active_ratio", 2.0 / 3.0},
      {"source.channel.1m.active", 1.0},
      {"source.channel.5m.active", 0.0},
      {"source.channel.1h.active", 1.0},
  };
  projection.axis_txt = {
      {"sampler", "sequential"},
      {"mode", "train|run"},
      {"source.runtime.symbol", "BTCUSDT"},
  };
  projection.tags = {{"determinism_policy", "deterministic"},
                     {"wave_id", profile.wave_id},
                     {"source.runtime.range_basis", "effective_intersection"},
                     {"source.runtime.interval_semantics", "half_open_utc_day"}};

  const std::vector<std::string> ratio_keys = {
      "source.runtime.request.from_ratio",
      "source.runtime.request.to_ratio",
      "source.runtime.request.span_ratio",
      "source.runtime.effective.coverage_ratio",
      "source.channels.active_ratio",
  };
  for (const auto& key : ratio_keys) {
    const double v = axis_num_must(projection, key);
    REQUIRE(v >= 0.0);
    REQUIRE(v <= 1.0);
  }
  const double clipped_left =
      axis_num_must(projection, "source.runtime.flags.clipped_left");
  const double clipped_right =
      axis_num_must(projection, "source.runtime.flags.clipped_right");
  REQUIRE(clipped_left == 0.0 || clipped_left == 1.0);
  REQUIRE(clipped_right == 0.0 || clipped_right == 1.0);
  for (const auto& [k, _] : projection.axis_num) {
    REQUIRE(k.find("_ms") == std::string::npos);
  }

  wave_cell_t cell{};
  REQUIRE(catalog.record_trial(coord, profile, trial, artifact_link, projection, &cell,
                               &error));
  REQUIRE(cell.cell_id == cell_id_1);
  REQUIRE(cell.trial_count == 1);
  REQUIRE(cell.state == "ready");
  REQUIRE(cell.projection_version == 2);

  wave_cell_t resolved{};
  REQUIRE(catalog.resolve_cell(coord, profile, &resolved, &error));
  REQUIRE(resolved.cell_id == cell.cell_id);
  REQUIRE(resolved.trial_count == 1);
  REQUIRE(resolved.projection_version == 2);

  std::vector<wave_trial_t> trials{};
  REQUIRE(catalog.list_trials(cell.cell_id, 10, 0, true, &trials, &error));
  REQUIRE(trials.size() == 1);
  REQUIRE(trials[0].trial_id == trial.trial_id);
  REQUIRE(trials[0].state_snapshot_id == trial.state_snapshot_id);

  matrix_query_t query{};
  query.contract_hash = coord.contract_hash;
  query.wave_hash = coord.wave_hash;
  query.axis_num_eq = {{"epochs", 10}};
  query.axis_txt_eq = {{"sampler", "sequential"}};
  query.tag_eq = {{"determinism_policy", "deterministic"}};
  query.limit = 10;

  std::vector<wave_cell_t> matches{};
  REQUIRE(catalog.query_matrix(query, &matches, &error));
  REQUIRE(matches.size() == 1);
  REQUIRE(matches[0].cell_id == cell.cell_id);
  REQUIRE(matches[0].last_trial_id == trial.trial_id);

  matrix_query_t runtime_query{};
  runtime_query.contract_hash = coord.contract_hash;
  runtime_query.wave_hash = coord.wave_hash;
  runtime_query.axis_num_eq = {
      {"source.runtime.request.from_ratio", 0.125},
      {"source.runtime.request.to_ratio", 0.5},
      {"source.runtime.request.span_ratio", 0.375},
      {"source.runtime.effective.coverage_ratio", 0.375},
      {"source.runtime.flags.clipped_left", 0.0},
      {"source.runtime.flags.clipped_right", 0.0},
      {"source.channel.1m.active", 1.0},
      {"source.channel.5m.active", 0.0},
  };
  runtime_query.axis_txt_eq = {{"source.runtime.symbol", "BTCUSDT"}};
  runtime_query.tag_eq = {
      {"source.runtime.range_basis", "effective_intersection"},
      {"source.runtime.interval_semantics", "half_open_utc_day"},
  };
  runtime_query.limit = 10;
  std::vector<wave_cell_t> runtime_matches{};
  REQUIRE(catalog.query_matrix(runtime_query, &runtime_matches, &error));
  REQUIRE(runtime_matches.size() == 1);
  REQUIRE(runtime_matches.front().cell_id == cell.cell_id);

  matrix_query_t raw_ms_query{};
  raw_ms_query.contract_hash = coord.contract_hash;
  raw_ms_query.wave_hash = coord.wave_hash;
  raw_ms_query.axis_num_eq = {{"source.runtime.debug.domain_from_ms", 1.0}};
  raw_ms_query.limit = 10;
  std::vector<wave_cell_t> raw_ms_matches{};
  REQUIRE(catalog.query_matrix(raw_ms_query, &raw_ms_matches, &error));
  REQUIRE(raw_ms_matches.empty());

  wave_trial_t failed_trial{};
  failed_trial.trial_id = "trial_002";
  failed_trial.started_at_ms = 1711111000222ULL;
  failed_trial.finished_at_ms = 1711111000333ULL;
  failed_trial.ok = false;
  failed_trial.error = "sink unavailable";
  failed_trial.total_steps = 64;
  failed_trial.board_hash = "board_hash_abc";
  failed_trial.run_id = "run_hash_002";
  failed_trial.state_snapshot_id = "snapshot_state_002";

  wave_artifact_link_t failed_link = artifact_link;
  failed_link.numeric_summary = {{"loss", 0.555}};
  failed_link.text_summary = {{"status", "error"}};
  failed_link.joined_kv_report =
      "schema=wave.report.v1\n"
      "status=error\n"
      "note=sink unavailable\n"
      "run_id=run_runtime_001\n"
      "source.runtime.projection.run_id=run_runtime_001\n"
      "source.runtime.projection.schema=wave.source.runtime.projection.v2\n"
      "source.runtime.symbol=BTCUSDT\n"
      "source.runtime.request.from_ratio=0.125\n";
  failed_link.aggregate_sha256 = "feedface";

  REQUIRE(catalog.record_trial(coord, profile, failed_trial, failed_link, projection,
                               &cell, &error));
  REQUIRE(cell.trial_count == 2);
  REQUIRE(cell.state == "error");

  matrix_query_t latest_success_query = query;
  latest_success_query.latest_success_only = true;
  latest_success_query.state_snapshot_id = "snapshot_state_001";
  std::vector<wave_cell_t> latest_success_cells{};
  REQUIRE(catalog.query_matrix(latest_success_query, &latest_success_cells, &error));
  REQUIRE(latest_success_cells.size() == 1);
  REQUIRE(latest_success_cells.front().state == "ready");
  REQUIRE(latest_success_cells.front().last_trial_id == trial.trial_id);
  REQUIRE(latest_success_cells.front().artifact_link.joined_kv_report ==
          artifact_link.joined_kv_report);

  matrix_query_t latest_any_query = query;
  latest_any_query.latest_success_only = false;
  latest_any_query.state_snapshot_id = "snapshot_state_002";
  std::vector<wave_cell_t> latest_any_cells{};
  REQUIRE(catalog.query_matrix(latest_any_query, &latest_any_cells, &error));
  REQUIRE(latest_any_cells.size() == 1);
  REQUIRE(latest_any_cells.front().state == "error");
  REQUIRE(latest_any_cells.front().last_trial_id == failed_trial.trial_id);
  REQUIRE(latest_any_cells.front().artifact_link.joined_kv_report ==
          failed_link.joined_kv_report);

  wave_artifact_link_t provenance{};
  REQUIRE(catalog.provenance(cell.cell_id, &provenance, &error));
  REQUIRE(provenance.joined_kv_report == failed_link.joined_kv_report);

  REQUIRE(catalog.close(&error));

  lattice_catalog_store_t reopened{};
  REQUIRE(reopened.open(opts, &error));
  wave_cell_t reopened_cell{};
  REQUIRE(reopened.resolve_cell(coord, profile, &reopened_cell, &error));
  REQUIRE(reopened_cell.cell_id == cell.cell_id);
  REQUIRE(reopened_cell.trial_count == 2);
  std::vector<wave_trial_t> reopened_trials{};
  REQUIRE(
      reopened.list_trials(reopened_cell.cell_id, 10, 0, true, &reopened_trials, &error));
  REQUIRE(reopened_trials.size() == 2);
  REQUIRE(reopened_trials[0].state_snapshot_id == failed_trial.state_snapshot_id);
  REQUIRE(reopened_trials[1].state_snapshot_id == trial.state_snapshot_id);
  REQUIRE(reopened.close(&error));

  lattice_catalog_store_t runtime_catalog{};
  opts.passphrase = kPass;
  REQUIRE(runtime_catalog.open(opts, &error));

  const fs::path runtime_store = tmp.dir / "runtime_store";
  const std::string canonical_runtime_path =
      "tsi.wikimyei.representation.vicreg.0x0042";

  write_text_file(
      runtime_store / "reports" / "runtime_missing_run_id.lls",
      "schema = piaabo.torch_compat.network_analytics.v4\n"
      "canonical_base = tsi.wikimyei.representation.vicreg.0x0042\n"
      "metric.loss = 0.77\n");

  REQUIRE(!runtime_catalog.ingest_runtime_reports(runtime_store, &error));
  REQUIRE(error.find("missing run_id") != std::string::npos);
  fs::remove(runtime_store / "reports" / "runtime_missing_run_id.lls");

  write_text_file(
      runtime_store / "reports" / "runtime_new.lls",
      "schema = piaabo.torch_compat.network_analytics.v4\n"
      "run_id = run_runtime_001\n"
      "canonical_base = tsi.wikimyei.representation.vicreg.0x0042\n"
      "metric.loss(-inf,+inf):double = 0.25\n"
      "metric.phase:str = eval\n");
  write_text_file(
      runtime_store / "reports" / "source_data_analytics.lls",
      "schema = piaabo.torch_compat.data_analytics.v1\n"
      "run_id = run_runtime_001\n"
      "canonical_base = tsi.source.dataloader.BTCUSDT\n"
      "source_label = tsi.source.dataloader.BTCUSDT\n"
      "sample_count = 256\n"
      "source_entropic_load = 7.5\n");
  write_text_file(
      runtime_store / "reports" / "runtime_legacy_schema.lls",
      "schema = piaabo.torch_compat.network_analytics.v3\n"
      "run_id = run_runtime_000\n"
      "canonical_base = tsi.wikimyei.representation.vicreg.0x0042\n"
      "metric.loss = 0.99\n");
  write_text_file(
      runtime_store / "reports" / "runtime_wrong_ext.kv",
      "schema = piaabo.torch_compat.network_analytics.v4\n"
      "run_id = run_runtime_002\n"
      "canonical_base = tsi.wikimyei.representation.vicreg.0x0042\n"
      "metric.loss = 0.50\n");

  REQUIRE(runtime_catalog.ingest_runtime_reports(runtime_store, &error));

  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> runtime_artifacts{};
  REQUIRE(runtime_catalog.list_runtime_artifacts(canonical_runtime_path, "", 0, 0,
                                                 true, &runtime_artifacts, &error));
  REQUIRE(runtime_artifacts.size() == 1);
  REQUIRE(runtime_artifacts.front().schema ==
          "piaabo.torch_compat.network_analytics.v4");
  REQUIRE(runtime_artifacts.front().path.find(".lls") != std::string::npos);

  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> source_artifacts{};
  REQUIRE(runtime_catalog.list_runtime_artifacts("tsi.source.dataloader",
                                                 "", 0, 0, true,
                                                 &source_artifacts, &error));
  bool saw_data_analytics = false;
  bool saw_source_runtime = false;
  for (const auto& row : source_artifacts) {
    REQUIRE(row.canonical_path == "tsi.source.dataloader");
    if (row.schema == "piaabo.torch_compat.data_analytics.v1") {
      saw_data_analytics = true;
    }
    if (row.schema == "wave.source.runtime.projection.v2") {
      saw_source_runtime = true;
      REQUIRE(row.payload_json.find("wave_cursor_resolution=run") !=
              std::string::npos);
      REQUIRE(row.payload_json.find("intersection_cursor=tsi.source.dataloader|") !=
              std::string::npos);
    }
  }
  REQUIRE(saw_data_analytics);
  REQUIRE(saw_source_runtime);

  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t>
      source_artifacts_symbol_cursor{};
  REQUIRE(runtime_catalog.list_runtime_artifacts(
      "tsi.source.dataloader.BTCUSDT", "", 0, 0, true,
      &source_artifacts_symbol_cursor, &error));
  REQUIRE(source_artifacts_symbol_cursor.size() == source_artifacts.size());
  for (const auto& row : source_artifacts_symbol_cursor) {
    REQUIRE(row.canonical_path == "tsi.source.dataloader");
  }

  REQUIRE(runtime_catalog.close(&error));

  lattice_catalog_store_t wrong_pass{};
  opts.passphrase = "wrong-passphrase";
  REQUIRE(!wrong_pass.open(opts, &error));

  std::cout << "[OK] test_lattice_catalog\n";
  return 0;
}
