// SPDX-License-Identifier: MIT
#include "kikijyeba/protocol/pipeline_builder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

namespace builder = cuwacunu::kikijyeba::protocol;
namespace lift = cuwacunu::wikimyei::expression::nodelift::srl::stream;
namespace types = cuwacunu::ujcamei::source::registry::types;

namespace {

using Kline = types::kline_t;

constexpr int64_t kNodeCount = 5;
constexpr int64_t kRecordCount = 96;
constexpr std::size_t kBatchSize = 8;
constexpr std::size_t kBenchmarkBatches = 5;

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  auto dir = std::filesystem::temp_directory_path() /
             ("cuwacunu_graph_first_pipeline_perf_" + label + "_" +
              std::to_string(static_cast<long long>(::getpid())));
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

Kline make_kline(types::ms_t close_time, double price, double scale) {
  Kline out{};
  out.open_time = close_time - 1;
  out.open_price = price;
  out.high_price = price + 0.01 * scale;
  out.low_price = price - 0.01 * scale;
  out.close_price = price + 0.001 * scale;
  out.volume = 1.0 + scale;
  out.close_time = close_time;
  out.quote_asset_volume = (1.0 + scale) * price;
  out.number_of_trades = static_cast<uint64_t>(10 + scale);
  out.taker_buy_base_volume = 0.5 + 0.1 * scale;
  out.taker_buy_quote_volume = (0.5 + 0.1 * scale) * price;
  return out;
}

void write_kline_csv(const std::filesystem::path &path, double base_price,
                     double scale) {
  std::ofstream out(path, std::ios::trunc);
  check(out.is_open(), "failed to open csv output: " + path.string());
  for (int64_t i = 0; i < kRecordCount; ++i) {
    const auto key = static_cast<types::ms_t>(100000 + i);
    make_kline(key, base_price + static_cast<double>(i) * 0.05, scale)
        .to_csv(out, ',');
    out << '\n';
  }
}

void write_text(const std::filesystem::path &path, const std::string &text) {
  std::ofstream out(path, std::ios::trunc);
  check(out.is_open(), "failed to open text output: " + path.string());
  out << text;
}

std::string frame_line(char begin, char end) {
  return std::string(1, begin) + std::string(130, '-') + end + "\n";
}

std::string make_sources_dsl(const std::filesystem::path &dir,
                             const std::vector<std::string> &nodes) {
  std::ostringstream dsl;
  dsl << "CSV_POLICY {\n"
      << "  CSV_BOOTSTRAP_DELTAS = 16;\n"
      << "  CSV_STEP_ABS_TOL = 1e-8;\n"
      << "  CSV_STEP_REL_TOL = 1e-10;\n"
      << "};\n"
      << "DATA_ANALYTICS_POLICY {\n"
      << "  MAX_SAMPLES = 64;\n"
      << "  MAX_FEATURES = 32;\n"
      << "  MASK_EPSILON = 1e-12;\n"
      << "  STANDARDIZE_EPSILON = 1e-8;\n"
      << "};\n"
      << frame_line('/', '\\')
      << "| instrument | interval | record_type | market_type | venue | "
         "base_asset | quote_asset | source_kind | source |\n"
      << frame_line('|', '|');

  int64_t edge_counter = 0;
  for (const auto &base : nodes) {
    for (const auto &quote : nodes) {
      if (base == quote) {
        continue;
      }
      const std::string symbol = base + quote;
      const auto csv = dir / (symbol + ".csv");
      write_kline_csv(csv, 100.0 + static_cast<double>(edge_counter),
                      1.0 + static_cast<double>(edge_counter));
      dsl << "| " << symbol << " | 1m | kline | spot | binance | " << base
          << " | " << quote << " | real | " << csv.string() << " |\n";
      ++edge_counter;
    }
  }
  dsl << frame_line('\\', '/');
  return dsl.str();
}

std::string make_channels_dsl() {
  return std::string(
      "/----------------------------------------------------------------"
      "-----------------------------------------------\\\n"
      "|  interval   |  active   |  record_type | input_length | "
      "future_length | channel_weight | normalization_policy |\n"
      "|----------------------------------------------------------------"
      "-----------------------------------------------|\n"
      "|    1m       |   true    |    kline     |     8        |       "
      "2       |      1.0       |  log_returns         |\n"
      "\\---------------------------------------------------------------"
      "------------------------------------------------/\n");
}

std::string make_graph_dsl(const std::vector<std::string> &nodes,
                           const std::string &fetch_mode) {
  std::ostringstream dsl;
  dsl << "GRAPH_POLICY {\n"
      << "  EDGE_RESOLUTION_POLICY = edge_discovery;\n"
      << "  EDGE_SOURCE_KIND = real;\n"
      << "  FETCH_MODE = " << fetch_mode << ";\n"
      << "  MAX_FETCH_WORKERS = 4;\n"
      << "  PARALLEL_MIN_WORK_ITEMS = 1;\n"
      << "};\n"
      << "/------------------------------------\\\n"
      << "|  node_id  |  node_kind  |  active  |\n"
      << "|------------------------------------|\n";
  for (const auto &node : nodes) {
    dsl << "| " << node << " | asset | true |\n";
  }
  dsl << "\\------------------------------------/\n";
  return dsl.str();
}

std::filesystem::path make_config(const std::filesystem::path &dir,
                                  const std::filesystem::path &sources_dsl,
                                  const std::filesystem::path &channels_dsl,
                                  const std::filesystem::path &graph_dsl,
                                  const std::string &label) {
  const auto config = dir / (label + ".config");
  write_text(
      config,
      std::string(
          "[UJCAMEI]\n"
          "ujcamei_source_registry_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/ujcamei.source.registry.dsl.bnf\n"
          "ujcamei_source_registry_dsl_path = ") +
          sources_dsl.string() +
          "\n"
          "ujcamei_source_retrieval_channels_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "ujcamei.source.retrieval.channels.dsl.bnf\n"
          "ujcamei_source_retrieval_channels_dsl_path = " +
          channels_dsl.string() +
          "\n"
          "kikijyeba_topology_graph_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "kikijyeba.topology.graph.dsl.bnf\n"
          "kikijyeba_topology_graph_dsl_path = " +
          graph_dsl.string() +
          "\n"
          "kikijyeba_settings_wave_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/kikijyeba.settings.wave.dsl.bnf\n"
          "kikijyeba_settings_wave_dsl_path = "
          "/cuwacunu/src/config/kikijyeba.settings.wave.dsl"
          "\n\n"
          "[WIKIMYEI]\n"
          "wikimyei_expression_nodelift_srl_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.expression.nodelift.srl.dsl.bnf\n"
          "wikimyei_expression_nodelift_srl_dsl_path = "
          "/cuwacunu/src/config/wikimyei.expression.nodelift.srl.dsl\n"
          "wikimyei_representation_vicreg_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.vicreg.dsl.bnf\n"
          "wikimyei_representation_vicreg_dsl_path = "
          "/cuwacunu/src/config/wikimyei.representation.vicreg.dsl\n"
          "wikimyei_representation_vicreg_net_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.representation.vicreg.net.bnf\n"
          "wikimyei_representation_vicreg_net_path = "
          "/cuwacunu/src/config/wikimyei.representation.vicreg.net\n"
          "wikimyei_inference_expected_value_mdn_dsl_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.dsl.bnf\n"
          "wikimyei_inference_expected_value_mdn_dsl_path = "
          "/cuwacunu/src/config/"
          "wikimyei.inference.expected_value.mdn.dsl\n\n"
          "wikimyei_inference_expected_value_mdn_net_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.net.bnf\n"
          "wikimyei_inference_expected_value_mdn_net_path = "
          "/cuwacunu/src/config/"
          "wikimyei.inference.expected_value.mdn.net\n\n"
          "[JKIMYEI]\n"
          "wikimyei_representation_vicreg_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/wikimyei.representation.vicreg.jkimyei.bnf\n"
          "wikimyei_representation_vicreg_jkimyei_path = "
          "/cuwacunu/src/config/wikimyei.representation.vicreg.jkimyei\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_bnf_path = "
          "/cuwacunu/src/config/grammar/"
          "wikimyei.inference.expected_value.mdn.jkimyei.bnf\n"
          "wikimyei_inference_expected_value_mdn_jkimyei_path = "
          "/cuwacunu/src/config/"
          "wikimyei.inference.expected_value.mdn.jkimyei\n");
  return config;
}

struct perf_result_t {
  int64_t node_count{0};
  int64_t edge_count{0};
  std::size_t batch_count{0};
  std::size_t anchor_count{0};
  double fetch_ms{0.0};
  double lift_ms{0.0};
};

template <typename Fn> double timed_ms(Fn &&fn) {
  const auto begin = std::chrono::steady_clock::now();
  fn();
  const auto end = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

perf_result_t
run_graph_first_pipeline(const std::filesystem::path &config_path) {
  auto bundle =
      builder::load_graph_first_config_bundle_from_config(config_path.string());
  builder::graph_first_pipeline_builder_options_t options{};
  options.batch_size = kBatchSize;
  options.compute_alignment_diagnostics = false;
  builder::graph_first_pipeline_builder_t<Kline> pipe(bundle, options);

  auto source = pipe.make_graph_source();
  const std::size_t source_size = source.size();
  check(source_size >= kBatchSize, "perf source has too few anchors");
  const std::size_t batches = std::min<std::size_t>(
      kBenchmarkBatches, (source_size + kBatchSize - 1) / kBatchSize);
  const auto srl_graph = pipe.srl_graph();

  perf_result_t out{};
  out.node_count = static_cast<int64_t>(srl_graph.node_ids.size());
  out.edge_count = static_cast<int64_t>(srl_graph.edge_ids.size());
  out.batch_count = batches;

  torch::NoGradGuard no_grad;
  for (std::size_t i = 0; i < batches; ++i) {
    const std::size_t begin_anchor = i * kBatchSize;
    if (begin_anchor >= source_size) {
      break;
    }
    typename decltype(source)::graph_batch_t graph_batch{};
    out.fetch_ms += timed_ms([&] {
      graph_batch = source.get_graph_batch(begin_anchor, kBatchSize);
    });
    out.anchor_count +=
        static_cast<std::size_t>(graph_batch.edge_features.size(0));
    out.lift_ms += timed_ms([&] {
      auto lifted = lift::node_lift_graph_anchor_edge_batch<
          typename Kline::key_type_t>(
          graph_batch, srl_graph,
          cuwacunu::wikimyei::expression::nodelift::srl::nodelift_options_t{},
          /*compute_alignment_diagnostics=*/false, /*lift_future=*/true);
      check(lifted.node_features.defined(), "NodeLift output missing");
      check(lifted.future_node_features.defined(),
            "future NodeLift output missing");
      check(torch::isfinite(lifted.node_features).all().item<bool>(),
            "NodeLift output has non-finite values");
    });
  }
  return out;
}

void print_result(const perf_result_t &r) {
  const double total_ms = r.fetch_ms + r.lift_ms;
  const double batches = static_cast<double>(
      std::max<std::size_t>(static_cast<std::size_t>(1), r.batch_count));
  const double anchors = static_cast<double>(
      std::max<std::size_t>(static_cast<std::size_t>(1), r.anchor_count));
  const double edge_samples = anchors * static_cast<double>(r.edge_count);
  std::cout << std::fixed << std::setprecision(3)
            << "[GraphFirstPipelinePerf] current_pipeline nodes="
            << r.node_count << " edges=" << r.edge_count
            << " batches=" << r.batch_count << " anchors=" << r.anchor_count
            << " fetch_ms_per_batch=" << (r.fetch_ms / batches)
            << " nodelift_ms_per_batch=" << (r.lift_ms / batches)
            << " total_ms_per_batch=" << (total_ms / batches)
            << " anchors_per_sec=" << (anchors * 1000.0 / total_ms)
            << " edge_samples_per_sec="
            << (edge_samples * 1000.0 / std::max(1e-9, r.fetch_ms)) << "\n";
}

} // namespace

int main() {
  try {
    torch::manual_seed(1701);
    const auto dir = make_tmp_dir("warm_cache");
    const std::vector<std::string> nodes{"N0", "N1", "N2", "N3", "N4"};
    check(static_cast<int64_t>(nodes.size()) == kNodeCount,
          "perf fixture node count mismatch");
    const auto sources = dir / "ujcamei.source.registry.dsl";
    const auto channels = dir / "ujcamei.source.retrieval.channels.dsl";
    const auto graph = dir / "graph.dsl";
    write_text(sources, make_sources_dsl(dir, nodes));
    write_text(channels, make_channels_dsl());
    write_text(graph, make_graph_dsl(nodes, "parallel_by_edge"));

    const auto config = make_config(dir, sources, channels, graph, "pipeline");

    // Warm cache creation/materialization before timing steady-state fetch.
    {
      auto bundle =
          builder::load_graph_first_config_bundle_from_config(config.string());
      builder::graph_first_pipeline_builder_options_t options{};
      options.batch_size = kBatchSize;
      options.force_rebuild_cache = true;
      builder::graph_first_pipeline_builder_t<Kline> pipe(bundle, options);
      auto source = pipe.make_graph_source();
      (void)source.size();
    }

    const auto result = run_graph_first_pipeline(config);
    check(result.node_count == kNodeCount, "unexpected graph node count");
    check(result.edge_count == kNodeCount * (kNodeCount - 1),
          "unexpected discovered directed edge count");
    check(result.anchor_count > 0, "throughput smoke produced no anchors");
    print_result(result);
    std::cout << "[GraphFirstPipelinePerf test] all checks passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[GraphFirstPipelinePerf test] failure: " << ex.what() << "\n";
    return 1;
  }
}
