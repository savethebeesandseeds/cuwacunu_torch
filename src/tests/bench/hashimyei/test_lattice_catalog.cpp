#include <hero/lattice_hero/lattice_catalog.h>
#include <hero/lattice_hero/hero_lattice_tools.h>

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
using cuwacunu::hero::wave::decode_cell_report_payload;
using cuwacunu::hero::wave::encode_cell_report_payload;
using cuwacunu::hero::wave::matrix_query_t;
using cuwacunu::hero::wave::lattice_cell_report_t;
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

[[nodiscard]] static bool find_projection_num(
    const wave_projection_t& projection, std::string_view key, double* out) {
  if (!out) return false;
  for (const auto& [k, v] : projection.projection_num) {
    if (k == key) {
      *out = v;
      return true;
    }
  }
  return false;
}

[[nodiscard]] static double projection_num_must(const wave_projection_t& projection,
                                                std::string_view key) {
  double out = 0.0;
  REQUIRE(find_projection_num(projection, key, &out));
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

[[nodiscard]] static std::string extract_line_value(std::string_view payload,
                                                    std::string_view key) {
  std::size_t cursor = 0;
  while (cursor < payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string_view::npos) line_end = payload.size();
    std::string_view line = payload.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    const std::size_t eq = line.find('=');
    if (eq != std::string_view::npos) {
      std::string_view lhs = line.substr(0, eq);
      while (!lhs.empty() &&
             std::isspace(static_cast<unsigned char>(lhs.back())) != 0) {
        lhs.remove_suffix(1);
      }
      if (lhs.rfind(key, 0) == 0) {
        std::string_view rhs = line.substr(eq + 1);
        while (!rhs.empty() &&
               std::isspace(static_cast<unsigned char>(rhs.front())) != 0) {
          rhs.remove_prefix(1);
        }
        return std::string(rhs);
      }
    }
    if (line_end == payload.size()) break;
    cursor = line_end + 1;
  }
  return {};
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

  lattice_cell_report_t report{};
  report.report_schema = "wave.cell.report.v1";
  report.source_report_fragment_ids = {"a1", "a2"};
  report.summary_num = {{"loss", 0.123}, {"accuracy", 0.99}};
  report.summary_txt = {{"status", "ok"}, {"report_schema", "report.v1"}};
  report.report_lls =
      "/* synthetic cell report transport: wave.cell.report.v1 */\n"
      "report_transport_schema=wave.cell.report.v1\n"
      "run_id=run_runtime_001\n"
      "/* embedded source.runtime.projection.v2 */\n"
      "source.runtime.projection.run_id=run_runtime_001\n"
      "source.runtime.projection.schema:str = wave.source.runtime.projection.v2\n"
      "source.runtime.symbol:str = BTCUSDT\n"
      "source.runtime.request.from_ratio(0,1):double = 0.125000000000\n";
  report.report_sha256 = "deadbeef";

  std::string encoded{};
  std::string error;
  REQUIRE(encode_cell_report_payload(report, &encoded, &error));
  REQUIRE(!encoded.empty());

  lattice_cell_report_t decoded{};
  REQUIRE(decode_cell_report_payload(encoded, &decoded, &error));
  REQUIRE(decoded.report_schema == report.report_schema);
  REQUIRE(decoded.source_report_fragment_ids.size() == report.source_report_fragment_ids.size());
  REQUIRE(decoded.report_lls == report.report_lls);

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
  trial.campaign_hash = "campaign_hash_abc";
  trial.run_id = "run_hash_001";
  trial.state_snapshot_id = "snapshot_state_001";

  wave_projection_t projection{};
  projection.projection_version = 2;
  projection.projector_build_id = "wave.projector.v2";
  projection.projection_num = {
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
  projection.projection_txt = {
      {"sampler", "sequential"},
      {"mode", "train|run"},
      {"source.runtime.symbol", "BTCUSDT"},
      {"determinism_policy", "deterministic"},
      {"wave_id", profile.wave_id},
      {"source.runtime.range_basis", "effective_intersection"},
      {"source.runtime.interval_semantics", "half_open_utc_day"},
  };

  const std::vector<std::string> ratio_keys = {
      "source.runtime.request.from_ratio",
      "source.runtime.request.to_ratio",
      "source.runtime.request.span_ratio",
      "source.runtime.effective.coverage_ratio",
      "source.channels.active_ratio",
  };
  for (const auto& key : ratio_keys) {
    const double v = projection_num_must(projection, key);
    REQUIRE(v >= 0.0);
    REQUIRE(v <= 1.0);
  }
  const double clipped_left =
      projection_num_must(projection, "source.runtime.flags.clipped_left");
  const double clipped_right =
      projection_num_must(projection, "source.runtime.flags.clipped_right");
  REQUIRE(clipped_left == 0.0 || clipped_left == 1.0);
  REQUIRE(clipped_right == 0.0 || clipped_right == 1.0);
  for (const auto& [k, _] : projection.projection_num) {
    REQUIRE(k.find("_ms") == std::string::npos);
  }

  wave_cell_t cell{};
  REQUIRE(catalog.record_trial(coord, profile, trial, report, projection, &cell,
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
  query.projection_num_eq = {{"epochs", 10}};
  query.projection_txt_eq = {{"sampler", "sequential"},
                             {"determinism_policy", "deterministic"}};
  query.limit = 10;

  std::vector<wave_cell_t> matches{};
  REQUIRE(catalog.query_matrix(query, &matches, &error));
  REQUIRE(matches.size() == 1);
  REQUIRE(matches[0].cell_id == cell.cell_id);
  REQUIRE(matches[0].last_trial_id == trial.trial_id);

  matrix_query_t runtime_query{};
  runtime_query.contract_hash = coord.contract_hash;
  runtime_query.wave_hash = coord.wave_hash;
  runtime_query.projection_num_eq = {
      {"source.runtime.request.from_ratio", 0.125},
      {"source.runtime.request.to_ratio", 0.5},
      {"source.runtime.request.span_ratio", 0.375},
      {"source.runtime.effective.coverage_ratio", 0.375},
      {"source.runtime.flags.clipped_left", 0.0},
      {"source.runtime.flags.clipped_right", 0.0},
      {"source.channel.1m.active", 1.0},
      {"source.channel.5m.active", 0.0},
  };
  runtime_query.projection_txt_eq = {{"source.runtime.symbol", "BTCUSDT"},
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
  raw_ms_query.projection_num_eq = {{"source.runtime.debug.domain_from_ms", 1.0}};
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
  failed_trial.campaign_hash = "campaign_hash_abc";
  failed_trial.run_id = "run_hash_002";
  failed_trial.state_snapshot_id = "snapshot_state_002";

  lattice_cell_report_t failed_report = report;
  failed_report.summary_num = {{"loss", 0.555}};
  failed_report.summary_txt = {{"status", "error"}};
  failed_report.report_lls =
      "/* synthetic cell report transport: wave.cell.report.v1 */\n"
      "report_transport_schema=wave.cell.report.v1\n"
      "status=error\n"
      "note=sink unavailable\n"
      "run_id=run_runtime_001\n"
      "/* embedded source.runtime.projection.v2 */\n"
      "source.runtime.projection.run_id=run_runtime_001\n"
      "source.runtime.projection.schema:str = wave.source.runtime.projection.v2\n"
      "source.runtime.symbol:str = BTCUSDT\n"
      "source.runtime.request.from_ratio(0,1):double = 0.125000000000\n";
  failed_report.report_sha256 = "feedface";

  REQUIRE(catalog.record_trial(coord, profile, failed_trial, failed_report, projection,
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
  REQUIRE(latest_success_cells.front().report.report_lls ==
          report.report_lls);

  matrix_query_t latest_any_query = query;
  latest_any_query.latest_success_only = false;
  latest_any_query.state_snapshot_id = "snapshot_state_002";
  std::vector<wave_cell_t> latest_any_cells{};
  REQUIRE(catalog.query_matrix(latest_any_query, &latest_any_cells, &error));
  REQUIRE(latest_any_cells.size() == 1);
  REQUIRE(latest_any_cells.front().state == "error");
  REQUIRE(latest_any_cells.front().last_trial_id == failed_trial.trial_id);
  REQUIRE(latest_any_cells.front().report.report_lls ==
          failed_report.report_lls);

  lattice_cell_report_t report_snapshot{};
  REQUIRE(catalog.get_cell_report(cell.cell_id, &report_snapshot, &error));
  REQUIRE(report_snapshot.report_lls == failed_report.report_lls);

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
      "schema:str = piaabo.torch_compat.network_analytics.v4\n"
      "canonical_path:str = tsi.wikimyei.representation.vicreg.0x0042\n"
      "metric.loss(-inf,+inf):double = 0.770000000000\n");

  REQUIRE(!runtime_catalog.ingest_runtime_report_fragments(runtime_store, &error));
  REQUIRE(error.find("missing run_id") != std::string::npos);
  fs::remove(runtime_store / "reports" / "runtime_missing_run_id.lls");

  write_text_file(
      runtime_store / "reports" / "runtime_namespaced_schema.lls",
      "source.runtime.projection.schema:str = wave.source.runtime.projection.v2\n"
      "run_id:str = run_runtime_001\n"
      "canonical_path:str = tsi.source.dataloader.BTCUSDT\n");

  REQUIRE(!runtime_catalog.ingest_runtime_report_fragments(runtime_store, &error));
  REQUIRE(error.find("top-level schema key") != std::string::npos);
  fs::remove(runtime_store / "reports" / "runtime_namespaced_schema.lls");

  write_text_file(
      runtime_store / "reports" / "runtime_new.lls",
      "schema:str = piaabo.torch_compat.network_analytics.v4\n"
      "run_id:str = run_runtime_001\n"
      "canonical_path:str = tsi.wikimyei.representation.vicreg.0x0042\n"
      "contract_hash:str = contract_hash_123\n"
      "network_global_entropic_capacity(0,+inf):double = 9.250000000000\n"
      "metric.loss(-inf,+inf):double = 0.25\n"
      "metric.phase:str = eval\n");
  write_text_file(
      runtime_store / "reports" / "status.latest.lls",
      "schema:str = tsi.wikimyei.representation.vicreg.status.v1\n"
      "report_kind:str = status\n"
      "canonical_path:str = tsi.wikimyei.representation.vicreg.0x0042\n"
      "hashimyei:str = 0x0042\n"
      "run_id:str = run_runtime_001\n"
      "trained_steps(0,+inf):uint = 12\n");
  write_text_file(
      runtime_store / "reports" / "source_data_analytics.lls",
      "schema:str = piaabo.torch_compat.data_analytics.v1\n"
      "run_id:str = run_runtime_001\n"
      "canonical_path:str = tsi.source.dataloader.BTCUSDT\n"
      "contract_hash:str = contract_hash_123\n"
      "source_label:str = tsi.source.dataloader.BTCUSDT\n"
      "sample_count(0,+inf):uint = 256\n"
      "source_entropic_load(0,+inf):double = 7.500000000000\n");
  write_text_file(
      runtime_store / "reports" / "runtime_unsupported_schema.lls",
      "schema:str = piaabo.torch_compat.network_analytics.v3\n"
      "run_id:str = run_runtime_000\n"
      "canonical_path:str = tsi.wikimyei.representation.vicreg.0x0042\n"
      "metric.loss(-inf,+inf):double = 0.990000000000\n");
  write_text_file(
      runtime_store / "reports" / "runtime_wrong_ext.kv",
      "schema:str = piaabo.torch_compat.network_analytics.v4\n"
      "run_id:str = run_runtime_002\n"
      "canonical_path:str = tsi.wikimyei.representation.vicreg.0x0042\n"
      "metric.loss(-inf,+inf):double = 0.500000000000\n");

  REQUIRE(runtime_catalog.ingest_runtime_report_fragments(runtime_store, &error));

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> runtime_report_fragments{};
  REQUIRE(runtime_catalog.list_runtime_report_fragments(canonical_runtime_path, "", 0, 0,
                                                 true, &runtime_report_fragments, &error));
  REQUIRE(runtime_report_fragments.size() == 2);
  bool saw_network_runtime = false;
  bool saw_status_runtime = false;
  for (const auto& row : runtime_report_fragments) {
    REQUIRE(row.path.find(".lls") != std::string::npos);
    if (row.schema == "piaabo.torch_compat.network_analytics.v4") {
      saw_network_runtime = true;
    }
    if (row.schema == "tsi.wikimyei.representation.vicreg.status.v1") {
      saw_status_runtime = true;
    }
  }
  REQUIRE(saw_network_runtime);
  REQUIRE(saw_status_runtime);

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t>
      runtime_report_fragments_family{};
  REQUIRE(runtime_catalog.list_runtime_report_fragments(
      "tsi.wikimyei.representation.vicreg", "", 0, 0, true,
      &runtime_report_fragments_family, &error));
  REQUIRE(runtime_report_fragments_family.size() == runtime_report_fragments.size());
  bool saw_family_exact_hash = false;
  for (const auto& row : runtime_report_fragments_family) {
    if (row.canonical_path == canonical_runtime_path) {
      saw_family_exact_hash = true;
      break;
    }
  }
  REQUIRE(saw_family_exact_hash);

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> source_report_fragments{};
  REQUIRE(runtime_catalog.list_runtime_report_fragments("tsi.source.dataloader",
                                                 "", 0, 0, true,
                                                 &source_report_fragments, &error));
  bool saw_data_analytics = false;
  bool saw_source_runtime = false;
  for (const auto& row : source_report_fragments) {
    REQUIRE(row.canonical_path == "tsi.source.dataloader");
    if (row.schema == "piaabo.torch_compat.data_analytics.v1") {
      saw_data_analytics = true;
    }
    if (row.schema == "wave.source.runtime.projection.v2") {
      saw_source_runtime = true;
      REQUIRE(row.payload_json.find(
                  "source.runtime.request.from_ratio(0,1):double = "
                  "0.125000000000") != std::string::npos);
      REQUIRE(row.payload_json.find("wave_cursor_resolution:str = run") !=
              std::string::npos);
      REQUIRE(row.payload_json.find(
                  "intersection_cursor:str = tsi.source.dataloader|") !=
              std::string::npos);
    }
  }
  REQUIRE(saw_data_analytics);
  REQUIRE(saw_source_runtime);

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t>
      source_report_fragments_symbol_cursor{};
  REQUIRE(runtime_catalog.list_runtime_report_fragments(
      "tsi.source.dataloader.BTCUSDT", "", 0, 0, true,
      &source_report_fragments_symbol_cursor, &error));
  REQUIRE(source_report_fragments_symbol_cursor.size() == source_report_fragments.size());
  for (const auto& row : source_report_fragments_symbol_cursor) {
    REQUIRE(row.canonical_path == "tsi.source.dataloader");
  }

  std::string matched_wave_cursor{};
  for (const auto& row : runtime_report_fragments) {
    if (row.schema == "piaabo.torch_compat.network_analytics.v4") {
      matched_wave_cursor = extract_line_value(row.payload_json, "wave_cursor");
      break;
    }
  }
  REQUIRE(!matched_wave_cursor.empty());
  std::uint64_t matched_wave_cursor_u64 = 0;
  REQUIRE(lattice_catalog_store_t::parse_runtime_wave_cursor_scalar(
      matched_wave_cursor, &matched_wave_cursor_u64));

  cuwacunu::hero::wave::runtime_view_report_t comparison_view{};
  REQUIRE(runtime_catalog.get_runtime_view_lls(
      "entropic_capacity_comparison", "run_runtime_001", 0, false, "",
      &comparison_view, &error));
  REQUIRE(comparison_view.view_kind == "entropic_capacity_comparison");
  REQUIRE(comparison_view.canonical_path ==
          "tsi.analysis.entropic_capacity_comparison");
  REQUIRE(comparison_view.match_count == 1);
  REQUIRE(comparison_view.ambiguity_count == 0);
  REQUIRE(comparison_view.view_lls.find("match_count=1") != std::string::npos);
  REQUIRE(comparison_view.view_lls.find("capacity_ratio=") != std::string::npos);
  REQUIRE(comparison_view.view_lls.find("source_entropic_load=7.500000000000") !=
          std::string::npos);
  REQUIRE(comparison_view.view_lls.find(
              "network_entropic_capacity=9.250000000000") !=
          std::string::npos);

  cuwacunu::hero::wave::runtime_view_report_t filtered_view{};
  REQUIRE(runtime_catalog.get_runtime_view_lls(
      "entropic_capacity_comparison", "run_runtime_001", matched_wave_cursor_u64,
      true, "contract_hash_123", &filtered_view, &error));
  REQUIRE(filtered_view.match_count == 1);
  REQUIRE(filtered_view.ambiguity_count == 0);

  cuwacunu::hero::wave::runtime_view_report_t empty_wave_view{};
  REQUIRE(runtime_catalog.get_runtime_view_lls(
      "entropic_capacity_comparison", "run_runtime_001",
      matched_wave_cursor_u64 + 1, true, "", &empty_wave_view, &error));
  REQUIRE(empty_wave_view.match_count == 0);
  REQUIRE(empty_wave_view.ambiguity_count == 0);
  REQUIRE(empty_wave_view.view_lls.find("match_count=0") != std::string::npos);

  cuwacunu::hero::wave::runtime_view_report_t empty_contract_view{};
  REQUIRE(runtime_catalog.get_runtime_view_lls(
      "entropic_capacity_comparison", "run_runtime_001", 0, false,
      "contract_hash_missing", &empty_contract_view, &error));
  REQUIRE(empty_contract_view.match_count == 0);
  REQUIRE(empty_contract_view.ambiguity_count == 0);

  write_text_file(
      runtime_store / "reports" / "runtime_new_ambiguous.lls",
      "schema:str = piaabo.torch_compat.network_analytics.v4\n"
      "run_id:str = run_runtime_001\n"
      "canonical_path:str = tsi.wikimyei.representation.vicreg.0x0043\n"
      "contract_hash:str = contract_hash_123\n"
      "network_global_entropic_capacity(0,+inf):double = 8.500000000000\n"
      "metric.loss(-inf,+inf):double = 0.31\n");
  REQUIRE(runtime_catalog.ingest_runtime_report_fragments(runtime_store, &error));

  cuwacunu::hero::wave::runtime_view_report_t ambiguous_view{};
  REQUIRE(runtime_catalog.get_runtime_view_lls(
      "entropic_capacity_comparison", "run_runtime_001", 0, false, "",
      &ambiguous_view, &error));
  REQUIRE(ambiguous_view.match_count == 0);
  REQUIRE(ambiguous_view.ambiguity_count == 1);
  REQUIRE(ambiguous_view.view_lls.find("kind=ambiguity") != std::string::npos);
  REQUIRE(ambiguous_view.view_lls.find("source_count=1") != std::string::npos);
  REQUIRE(ambiguous_view.view_lls.find("network_count=2") != std::string::npos);

  const std::string intersection_cursor = "tsi.source.dataloader|123456";
  const std::string intersection_report =
      "/* synthetic report_lls transport: hashimyei.joined_report.v1 */\n"
      "report_transport_schema=hashimyei.joined_report.v1\n"
      "canonical_path=tsi.source.dataloader\n"
      "source_report_fragment_count=2\n";
  REQUIRE(runtime_catalog.upsert_runtime_intersection_report(
      intersection_cursor, "tsi.source.dataloader", "run_runtime_001",
      1711111000999ULL, intersection_report, &error));

  std::string restored_report{};
  std::string restored_path{};
  std::string restored_run{};
  REQUIRE(runtime_catalog.get_runtime_intersection_report(
      intersection_cursor, &restored_report, &restored_path, &restored_run, &error));
  REQUIRE(restored_report == intersection_report);
  REQUIRE(restored_path == "tsi.source.dataloader");
  REQUIRE(restored_run == "run_runtime_001");

  REQUIRE(runtime_catalog.close(&error));

  cuwacunu::hero::lattice_mcp::app_context_t lattice_app{};
  lattice_app.store_root = runtime_store;
  lattice_app.lattice_catalog_path = catalog_path;
  lattice_app.hashimyei_catalog_path = tmp.dir / "runtime_hashimyei_catalog.idydb";

  std::string tool_result{};
  std::string tool_error{};
  REQUIRE(cuwacunu::hero::lattice_mcp::execute_tool_json(
      "hero.lattice.get_view_lls",
      "{\"view_kind\":\"entropic_capacity_comparison\",\"run_id\":\"run_runtime_001\"}",
      &lattice_app, &tool_result, &tool_error));
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":false") != std::string::npos);
  REQUIRE(tool_result.find("\"view_kind\":\"entropic_capacity_comparison\"") !=
          std::string::npos);
  REQUIRE(tool_result.find("\"ambiguity_count\":1") != std::string::npos);
  REQUIRE(tool_result.find("kind=ambiguity") != std::string::npos);

  tool_result.clear();
  tool_error.clear();
  REQUIRE(cuwacunu::hero::lattice_mcp::execute_tool_json(
      "hero.lattice.get_view_lls",
      "{\"view_kind\":\"entropic_capacity_comparison\",\"run_id\":\"run_runtime_001\",\"wave_cursor\":\"999999999\",\"contract_hash\":\"contract_hash_missing\"}",
      &lattice_app, &tool_result, &tool_error));
  REQUIRE(tool_error.empty());
  REQUIRE(tool_result.find("\"isError\":false") != std::string::npos);
  REQUIRE(tool_result.find("\"match_count\":0") != std::string::npos);

  lattice_catalog_store_t wrong_pass{};
  opts.passphrase = "wrong-passphrase";
  REQUIRE(!wrong_pass.open(opts, &error));

  std::cout << "[OK] test_lattice_catalog\n";
  return 0;
}
