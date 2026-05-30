#include <hero/lattice_hero/source_runtime_projection.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using cuwacunu::hero::wave::build_source_runtime_projection_fragment;
using cuwacunu::hero::wave::source_runtime_channel_state_t;
using cuwacunu::hero::wave::source_runtime_projection_fragment_t;
using cuwacunu::hero::wave::source_runtime_projection_input_t;

static void require_impl(bool ok, const char* expr, const char* file, int line) {
  if (!ok) {
    std::cerr << "[TEST FAIL] " << file << ":" << line << "  (" << expr << ")\n";
    std::exit(1);
  }
}
#define REQUIRE(x) require_impl((x), #x, __FILE__, __LINE__)

static void require_close_impl(double a, double b, double eps, const char* expr_a,
                               const char* expr_b, const char* file, int line) {
  if (std::fabs(a - b) > eps) {
    std::cerr << "[TEST FAIL] " << file << ":" << line << "  (" << expr_a
              << " ~= " << expr_b << ", got " << a << " vs " << b << ")\n";
    std::exit(1);
  }
}
#define REQUIRE_CLOSE(a, b, eps) \
  require_close_impl((a), (b), (eps), #a, #b, __FILE__, __LINE__)

[[nodiscard]] static bool find_projection_num(
    const source_runtime_projection_fragment_t& fragment, std::string_view key,
    double* out) {
  if (!out) return false;
  for (const auto& [k, v] : fragment.projection_num) {
    if (k == key) {
      *out = v;
      return true;
    }
  }
  return false;
}

[[nodiscard]] static bool has_projection_txt(
    const source_runtime_projection_fragment_t& fragment, std::string_view key,
    std::string_view expected) {
  for (const auto& [k, v] : fragment.projection_txt) {
    if (k == key && v == expected) return true;
  }
  return false;
}

[[nodiscard]] static bool has_projection_txt_key(
    const source_runtime_projection_fragment_t& fragment, std::string_view key) {
  for (const auto& [k, _] : fragment.projection_txt) {
    if (k == key) return true;
  }
  return false;
}

[[nodiscard]] static double projection_num_must(
    const source_runtime_projection_fragment_t& fragment, std::string_view key) {
  double v = 0.0;
  REQUIRE(find_projection_num(fragment, key, &v));
  return v;
}

static void require_ratios_and_flags_are_bounded(
    const source_runtime_projection_fragment_t& fragment) {
  const std::vector<std::string> ratio_keys = {
      "source.runtime.request.from_ratio",
      "source.runtime.request.to_ratio",
      "source.runtime.request.span_ratio",
      "source.runtime.effective.coverage_ratio",
      "source.channels.active_ratio",
  };
  for (const auto& key : ratio_keys) {
    const double v = projection_num_must(fragment, key);
    REQUIRE(v >= 0.0);
    REQUIRE(v <= 1.0);
  }

  const double clipped_left =
      projection_num_must(fragment, "source.runtime.flags.clipped_left");
  const double clipped_right =
      projection_num_must(fragment, "source.runtime.flags.clipped_right");
  REQUIRE(clipped_left == 0.0 || clipped_left == 1.0);
  REQUIRE(clipped_right == 0.0 || clipped_right == 1.0);
}

static void require_no_raw_ms_axes(
    const source_runtime_projection_fragment_t& fragment) {
  for (const auto& [key, _] : fragment.projection_num) {
    REQUIRE(key.find("_ms") == std::string::npos);
  }
}

static void test_valid_in_range_request() {
  source_runtime_projection_input_t input{};
  input.symbol = "BTCUSDT";
  input.from_date_ddmmyyyy = "03.01.2024";
  input.to_date_ddmmyyyy = "05.01.2024";
  input.domain_from_ms = 1704153600000.0;  // 2024-01-02T00:00:00Z
  input.domain_to_ms = 1704844800000.0;    // 2024-01-10T00:00:00Z (exclusive)
  input.channel_states = {
      {.interval = "1m", .active = true},
      {.interval = "5m", .active = false},
      {.interval = "1h", .active = true},
  };

  source_runtime_projection_fragment_t fragment{};
  std::string error;
  REQUIRE(build_source_runtime_projection_fragment(input, &fragment, &error));
  REQUIRE(error.empty());

  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.from_ratio"), 0.125,
                1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.to_ratio"), 0.5,
                1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.span_ratio"), 0.375,
                1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.effective.coverage_ratio"),
                0.375, 1e-12);
  REQUIRE(projection_num_must(fragment, "source.runtime.flags.clipped_left") == 0.0);
  REQUIRE(projection_num_must(fragment, "source.runtime.flags.clipped_right") == 0.0);
  REQUIRE(projection_num_must(fragment, "source.channels.active_count") == 2.0);
  REQUIRE(projection_num_must(fragment, "source.channels.total_count") == 3.0);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.channels.active_ratio"),
                (2.0 / 3.0), 1e-12);
  REQUIRE(projection_num_must(fragment, "source.channel.1m.active") == 1.0);
  REQUIRE(projection_num_must(fragment, "source.channel.5m.active") == 0.0);
  REQUIRE(projection_num_must(fragment, "source.channel.1h.active") == 1.0);
  REQUIRE(has_projection_txt(fragment, "source.runtime.symbol", "BTCUSDT"));
  REQUIRE(has_projection_txt(fragment, "source.runtime.range_basis",
                             "effective_intersection"));
  REQUIRE(has_projection_txt(fragment, "source.runtime.interval_semantics",
                             "half_open_utc_day"));
  REQUIRE(has_projection_txt_key(fragment, "source.runtime.range_basis"));
  REQUIRE(has_projection_txt_key(fragment, "source.runtime.interval_semantics"));
  REQUIRE(!fragment.projection_lls.empty());
  REQUIRE(fragment.projection_lls.find("source.runtime.projection.schema:str =") !=
          std::string::npos);
  REQUIRE(fragment.projection_lls.find("source.runtime.debug.domain_from_ms") !=
          std::string::npos);

  require_ratios_and_flags_are_bounded(fragment);
  require_no_raw_ms_axes(fragment);

  cuwacunu::hero::wave::source_runtime_projection_report_identity_t identity{};
  identity.canonical_path = "tsi.source.dataloader";
  identity.source_label = "BTCUSDT";
  identity.binding_id = "bind.train.vicreg";
  identity.has_wave_cursor = true;
  identity.wave_cursor = 123456ULL;

  std::string runtime_report_text{};
  REQUIRE(cuwacunu::hero::wave::emit_source_runtime_projection_runtime_report(
      fragment, identity, &runtime_report_text, &error));
  REQUIRE(!runtime_report_text.empty());
  REQUIRE(runtime_report_text.find("schema:str = wave.source.runtime.projection.v2") !=
          std::string::npos);
  REQUIRE(runtime_report_text.find("canonical_path:str = tsi.source.dataloader") !=
          std::string::npos);
  REQUIRE(runtime_report_text.find(
              "source_label:str = BTCUSDT") !=
          std::string::npos);
  REQUIRE(runtime_report_text.find("wave_cursor[0,+inf):uint = 123456") !=
          std::string::npos);

  cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t document{};
  REQUIRE(cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
      runtime_report_text, &document, &error));
  std::unordered_map<std::string, std::string> kv{};
  REQUIRE(cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
      document, &kv, &error));
  REQUIRE(kv["schema"] == "wave.source.runtime.projection.v2");
  REQUIRE(kv["canonical_path"] == "tsi.source.dataloader");
}

static void test_left_clip_only() {
  source_runtime_projection_input_t input{};
  input.symbol = "BTCUSDT";
  input.from_date_ddmmyyyy = "03.01.2024";
  input.to_date_ddmmyyyy = "07.01.2024";
  input.domain_from_ms = 1704412800000.0;  // 2024-01-05
  input.domain_to_ms = 1705017600000.0;    // 2024-01-12 (exclusive)
  input.channel_states = {{.interval = "1m", .active = true}};

  source_runtime_projection_fragment_t fragment{};
  std::string error;
  REQUIRE(build_source_runtime_projection_fragment(input, &fragment, &error));
  REQUIRE(error.empty());

  REQUIRE(projection_num_must(fragment, "source.runtime.flags.clipped_left") == 1.0);
  REQUIRE(projection_num_must(fragment, "source.runtime.flags.clipped_right") == 0.0);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.from_ratio"), 0.0,
                1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.to_ratio"),
                (3.0 / 7.0), 1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.span_ratio"),
                (5.0 / 7.0), 1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.effective.coverage_ratio"),
                (3.0 / 7.0), 1e-12);
  require_ratios_and_flags_are_bounded(fragment);
  require_no_raw_ms_axes(fragment);
}

static void test_right_clip_only() {
  source_runtime_projection_input_t input{};
  input.symbol = "BTCUSDT";
  input.from_date_ddmmyyyy = "08.01.2024";
  input.to_date_ddmmyyyy = "12.01.2024";
  input.domain_from_ms = 1704153600000.0;  // 2024-01-02
  input.domain_to_ms = 1704844800000.0;    // 2024-01-10 (exclusive)
  input.channel_states = {{.interval = "1m", .active = true}};

  source_runtime_projection_fragment_t fragment{};
  std::string error;
  REQUIRE(build_source_runtime_projection_fragment(input, &fragment, &error));
  REQUIRE(error.empty());

  REQUIRE(projection_num_must(fragment, "source.runtime.flags.clipped_left") == 0.0);
  REQUIRE(projection_num_must(fragment, "source.runtime.flags.clipped_right") == 1.0);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.from_ratio"), 0.75,
                1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.to_ratio"), 1.0,
                1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.span_ratio"),
                (5.0 / 8.0), 1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.effective.coverage_ratio"),
                0.25, 1e-12);
  require_ratios_and_flags_are_bounded(fragment);
  require_no_raw_ms_axes(fragment);
}

static void test_both_sides_clipped() {
  source_runtime_projection_input_t input{};
  input.symbol = "BTCUSDT";
  input.from_date_ddmmyyyy = "01.01.2024";
  input.to_date_ddmmyyyy = "15.01.2024";
  input.domain_from_ms = 1704412800000.0;  // 2024-01-05
  input.domain_to_ms = 1704844800000.0;    // 2024-01-10 (exclusive)
  input.channel_states = {{.interval = "1m", .active = true}};

  source_runtime_projection_fragment_t fragment{};
  std::string error;
  REQUIRE(build_source_runtime_projection_fragment(input, &fragment, &error));
  REQUIRE(error.empty());

  REQUIRE(projection_num_must(fragment, "source.runtime.flags.clipped_left") == 1.0);
  REQUIRE(projection_num_must(fragment, "source.runtime.flags.clipped_right") == 1.0);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.from_ratio"), 0.0,
                1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.to_ratio"), 1.0,
                1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.request.span_ratio"), 1.0,
                1e-12);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.runtime.effective.coverage_ratio"),
                1.0, 1e-12);
  require_ratios_and_flags_are_bounded(fragment);
  require_no_raw_ms_axes(fragment);
}

static void test_date_parse_failure() {
  source_runtime_projection_input_t input{};
  input.symbol = "BTCUSDT";
  input.from_date_ddmmyyyy = "2024-01-01";
  input.to_date_ddmmyyyy = "05.01.2024";
  input.domain_from_ms = 1704153600000.0;
  input.domain_to_ms = 1704844800000.0;
  input.channel_states = {{.interval = "1m", .active = true}};

  source_runtime_projection_fragment_t fragment{};
  std::string error;
  REQUIRE(!build_source_runtime_projection_fragment(input, &fragment, &error));
  REQUIRE(error.find("invalid FROM date") != std::string::npos);
}

static void test_from_greater_than_to_failure() {
  source_runtime_projection_input_t input{};
  input.symbol = "BTCUSDT";
  input.from_date_ddmmyyyy = "10.01.2024";
  input.to_date_ddmmyyyy = "09.01.2024";
  input.domain_from_ms = 1704153600000.0;
  input.domain_to_ms = 1704844800000.0;
  input.channel_states = {{.interval = "1m", .active = true}};

  source_runtime_projection_fragment_t fragment{};
  std::string error;
  REQUIRE(!build_source_runtime_projection_fragment(input, &fragment, &error));
  REQUIRE(error.find("FROM must be <= TO") != std::string::npos);
}

static void test_invalid_domain_failure() {
  source_runtime_projection_input_t input{};
  input.symbol = "BTCUSDT";
  input.from_date_ddmmyyyy = "01.01.2024";
  input.to_date_ddmmyyyy = "02.01.2024";
  input.domain_from_ms = 1704153600000.0;
  input.domain_to_ms = 1704153600000.0;
  input.channel_states = {{.interval = "1m", .active = true}};

  source_runtime_projection_fragment_t fragment{};
  std::string error;
  REQUIRE(!build_source_runtime_projection_fragment(input, &fragment, &error));
  REQUIRE(error.find("domain span must be > 0") != std::string::npos);
}

static void test_per_interval_active_projection_emission() {
  source_runtime_projection_input_t input{};
  input.symbol = "BTCUSDT";
  input.from_date_ddmmyyyy = "03.01.2024";
  input.to_date_ddmmyyyy = "03.01.2024";
  input.domain_from_ms = 1704067200000.0;  // 2024-01-01
  input.domain_to_ms = 1704931200000.0;    // 2024-01-11 (exclusive)
  input.channel_states = {
      {.interval = "1m", .active = false},
      {.interval = "1m", .active = true},   // OR fold to active
      {.interval = "15m", .active = false},
      {.interval = "2h@x", .active = true}  // Sanitized key.
  };

  source_runtime_projection_fragment_t fragment{};
  std::string error;
  REQUIRE(build_source_runtime_projection_fragment(input, &fragment, &error));
  REQUIRE(error.empty());

  REQUIRE(projection_num_must(fragment, "source.channels.total_count") == 3.0);
  REQUIRE(projection_num_must(fragment, "source.channels.active_count") == 2.0);
  REQUIRE_CLOSE(projection_num_must(fragment, "source.channels.active_ratio"),
                (2.0 / 3.0), 1e-12);
  REQUIRE(projection_num_must(fragment, "source.channel.1m.active") == 1.0);
  REQUIRE(projection_num_must(fragment, "source.channel.15m.active") == 0.0);
  REQUIRE(projection_num_must(fragment, "source.channel.2h_x.active") == 1.0);
  require_ratios_and_flags_are_bounded(fragment);
  require_no_raw_ms_axes(fragment);
}

int main() {
  test_valid_in_range_request();
  test_left_clip_only();
  test_right_clip_only();
  test_both_sides_clipped();
  test_date_parse_failure();
  test_from_greater_than_to_failure();
  test_invalid_domain_failure();
  test_per_interval_active_projection_emission();

  std::cout << "[OK] test_source_runtime_projection\n";
  return 0;
}
