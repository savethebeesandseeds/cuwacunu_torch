#include "pooling_structure_mechanism_map_gate.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace gate = cuwacunu::tests::pooling_structure_mechanism_map_gate;

namespace {

constexpr std::array<std::string_view, 3> kSeeds{"17", "31", "47"};
constexpr std::array<std::string_view, 5> kArms{
    "channel", "channel_domain", "channel_domain_scale",
    "channel_domain_scale_bin", "encoder"};
constexpr std::array<std::string_view, 4> kFamilies{
    "multiscale_state", "order_regime", "cross_channel", "future"};
constexpr std::array<double, 3> kExpectedChannelAulc{
    0.51029806802386968, 0.5121433689059538, 0.53534605970626181};
constexpr std::array<double, 3> kExpectedEncoderAulc{
    0.58626145257333262, 0.56999408500250559, 0.58033945194633074};

struct Records {
  std::map<std::string, std::string> values{};
  std::vector<std::string> errors{};
  std::size_t machine_line_count{0};
  std::size_t duplicate_key_count{0};
  std::size_t numeric_value_count{0};
  std::size_t nonfinite_value_count{0};
};

struct ShuffleControls {
  bool continuous{true};
  bool order{true};
};

struct ArmData {
  std::array<double, 3> area{};
  std::array<std::array<double, 4>, 3> family{};
  std::array<double, 3> order{};
};

[[nodiscard]] bool valid_machine_key(std::string_view key) {
  if (key.empty() ||
      !((key.front() >= 'a' && key.front() <= 'z') ||
        (key.front() >= 'A' && key.front() <= 'Z'))) {
    return false;
  }
  return std::all_of(key.begin(), key.end(), [](char value) {
    return (value >= 'a' && value <= 'z') ||
           (value >= 'A' && value <= 'Z') ||
           (value >= '0' && value <= '9') || value == '_' || value == '.';
  });
}

[[nodiscard]] bool parse_complete_double(std::string_view text,
                                         double &value) {
  if (text.empty()) {
    return false;
  }
  std::string owned(text);
  char *end = nullptr;
  errno = 0;
  value = std::strtod(owned.c_str(), &end);
  return end == owned.c_str() + owned.size() && end != owned.c_str() &&
         errno != ERANGE;
}

[[nodiscard]] Records parse_records(std::string_view raw) {
  Records result{};
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
    if (separator != std::string_view::npos &&
        valid_machine_key(line.substr(0, separator))) {
      ++result.machine_line_count;
      const std::string key(line.substr(0, separator));
      const std::string value(line.substr(separator + 1));
      if (!result.values.emplace(key, value).second) {
        ++result.duplicate_key_count;
        result.errors.push_back("duplicate machine key: " + key);
      }
      double numeric = 0.0;
      if (parse_complete_double(value, numeric)) {
        ++result.numeric_value_count;
        if (!std::isfinite(numeric)) {
          ++result.nonfinite_value_count;
          result.errors.push_back("non-finite machine value: " + key);
        }
      }
    }
    if (newline == std::string_view::npos) {
      break;
    }
    begin = newline + 1;
  }
  return result;
}

void fail(Records &records, const std::string &message) {
  records.errors.push_back(message);
}

[[nodiscard]] const std::string &required(Records &records,
                                          const std::string &key) {
  const auto found = records.values.find(key);
  if (found == records.values.end()) {
    fail(records, "missing machine key: " + key);
    static const std::string empty{};
    return empty;
  }
  return found->second;
}

[[nodiscard]] bool boolean(Records &records, const std::string &key) {
  const std::string &value = required(records, key);
  if (value == "true") {
    return true;
  }
  if (value != "false") {
    fail(records, "invalid boolean: " + key + "=" + value);
  }
  return false;
}

[[nodiscard]] double number(Records &records, const std::string &key) {
  const std::string &text = required(records, key);
  double value = 0.0;
  if (!parse_complete_double(text, value) || !std::isfinite(value)) {
    fail(records, "invalid finite number: " + key + "=" + text);
    return 0.0;
  }
  return value;
}

void expect_text(Records &records, const std::string &key,
                 std::string_view expected) {
  const std::string &observed = required(records, key);
  if (observed != expected) {
    fail(records, "unexpected value: " + key + "=" + observed +
                      " expected=" + std::string(expected));
  }
}

void expect_bool(Records &records, const std::string &key, bool expected) {
  const bool observed = boolean(records, key);
  if (observed != expected) {
    fail(records, "unexpected boolean: " + key);
  }
}

[[nodiscard]] bool close(double left, double right,
                         double tolerance = 1.0e-12) {
  return std::isfinite(left) && std::isfinite(right) &&
         std::abs(left - right) <= tolerance;
}

void expect_close(Records &records, const std::string &key, double expected) {
  const double observed = number(records, key);
  if (!close(observed, expected)) {
    fail(records, "numeric mismatch: " + key);
  }
}

[[nodiscard]] std::string seed_arm_prefix(std::string_view seed,
                                          std::string_view arm) {
  return "psm.seed_" + std::string(seed) + ".arm." + std::string(arm);
}

[[nodiscard]] std::string summary_arm_prefix(std::string_view arm) {
  return "psm.summary.arm." + std::string(arm);
}

[[nodiscard]] ArmData read_arm(Records &records, std::string_view arm) {
  ArmData result{};
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    const std::string prefix = seed_arm_prefix(kSeeds[seed], arm);
    result.area[seed] = number(records, prefix + ".probe.area");
    result.order[seed] =
        number(records, prefix + ".order_probe.accuracy_aulc");
    for (std::size_t family = 0; family < kFamilies.size(); ++family) {
      result.family[seed][family] =
          number(records, prefix + ".probe.family_" +
                              std::string(kFamilies[family]) + "_aulc");
    }
  }
  return result;
}

[[nodiscard]] double mean(const std::array<double, 3> &values) {
  return (values[0] + values[1] + values[2]) / 3.0;
}

void verify_arm_summary(Records &records, std::string_view arm,
                        const ArmData &data) {
  const std::string prefix = summary_arm_prefix(arm);
  expect_close(records, prefix + ".aulc.point", mean(data.area));
  expect_close(records, prefix + ".order.point", mean(data.order));
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    expect_close(records, prefix + ".aulc.seed_" + std::string(kSeeds[seed]),
                 data.area[seed]);
    expect_close(records, prefix + ".order.seed_" + std::string(kSeeds[seed]),
                 data.order[seed]);
  }
  for (std::size_t family = 0; family < kFamilies.size(); ++family) {
    const double expected = (data.family[0][family] + data.family[1][family] +
                             data.family[2][family]) /
                            3.0;
    expect_close(records, prefix + ".family_" +
                              std::string(kFamilies[family]) + "_aulc",
                 expected);
  }
}

[[nodiscard]] gate::ContinuousInput
recompute_continuous(Records &records, const std::string &prefix,
                     const ArmData &downstream, const ArmData &upstream) {
  gate::ContinuousInput result{};
  result.low = number(records, prefix + ".bootstrap_95_low");
  result.high = number(records, prefix + ".bootstrap_95_high");
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    result.seed_deltas[seed] = downstream.area[seed] - upstream.area[seed];
    result.point += result.seed_deltas[seed];
    expect_close(records, prefix + ".seed_" + std::string(kSeeds[seed]) +
                              "_delta",
                 result.seed_deltas[seed]);
  }
  result.point /= 3.0;
  expect_close(records, prefix + ".point", result.point);
  for (std::size_t family = 0; family < kFamilies.size(); ++family) {
    for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
      result.family_deltas[family] +=
          downstream.family[seed][family] - upstream.family[seed][family];
    }
    result.family_deltas[family] /= 3.0;
    expect_close(records, prefix + ".family_" +
                              std::string(kFamilies[family]) + "_delta",
                 result.family_deltas[family]);
  }
  const auto evaluated = gate::evaluate_continuous(result);
  expect_text(records, prefix + ".classification",
              gate::continuous_classification_name(evaluated.classification));
  return result;
}

[[nodiscard]] gate::OrderInput recompute_order(Records &records,
                                                std::string_view arm,
                                                const ArmData &data) {
  const std::string prefix = summary_arm_prefix(arm) + ".order";
  gate::OrderInput result{};
  result.point = mean(data.order);
  result.low = number(records, prefix + ".bootstrap_95_low");
  result.high = number(records, prefix + ".bootstrap_95_high");
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    result.seed_points[seed] = data.order[seed];
  }
  expect_close(records, prefix + ".point", result.point);
  const auto evaluated = gate::evaluate_order(result);
  expect_text(records, prefix + ".classification",
              gate::order_classification_name(evaluated.classification));
  return result;
}

[[nodiscard]] ShuffleControls recompute_shuffle_controls(
    Records &records, const std::array<ArmData, 5> &) {
  ShuffleControls result{};
  for (const std::string_view arm : kArms) {
    const std::string prefix = summary_arm_prefix(arm);
    const double shuffled_point =
        number(records, prefix + ".shuffled_aulc.point");
    const double shuffled_high =
        number(records, prefix + ".shuffled_aulc.bootstrap_95_high");
    const bool continuous = shuffled_point <= 0.02 && shuffled_high <= 0.05;
    if (boolean(records, prefix + ".continuous_shuffle_pass") != continuous) {
      fail(records, "continuous shuffle decision mismatch: " + prefix);
    }
    const double order_point =
        number(records, prefix + ".order_shuffled.point");
    const double order_high =
        number(records, prefix + ".order_shuffled.bootstrap_95_high");
    const bool order = order_point <= 0.55 && order_high <= 0.60;
    if (boolean(records, prefix + ".order_shuffle_pass") != order) {
      fail(records, "order shuffle decision mismatch: " + prefix);
    }
    result.continuous = result.continuous && continuous;
    result.order = result.order && order;
  }
  return result;
}

[[nodiscard]] bool all_seed_flag(Records &records, std::string_view suffix) {
  bool result = true;
  for (const std::string_view seed : kSeeds) {
    result = boolean(records,
                     "psm.seed_" + std::string(seed) + "." +
                         std::string(suffix)) &&
             result;
  }
  return result;
}

[[nodiscard]] bool verify_reference_endpoints(Records &records) {
  bool result = true;
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    const std::string prefix =
        "psm.reference.seed_" + std::string(kSeeds[seed]);
    const double channel = number(records, prefix + ".channel_aulc");
    const double encoder = number(records, prefix + ".rssm_encoder_aulc");
    result = close(channel, kExpectedChannelAulc[seed]) &&
             close(encoder, kExpectedEncoderAulc[seed]) && result;
    expect_bool(records, prefix + ".channel_aulc_exact",
                close(channel, kExpectedChannelAulc[seed]));
    expect_bool(records, prefix + ".rssm_encoder_aulc_exact",
                close(encoder, kExpectedEncoderAulc[seed]));
  }
  const double channel_order = number(records, "psm.reference.channel_order_mean");
  const double encoder_order =
      number(records, "psm.reference.rssm_encoder_order_mean");
  result = close(channel_order, 0.57454427083333337) &&
           close(encoder_order, 0.9560546875) && result;
  expect_bool(records, "psm.reference.channel_order_exact",
              close(channel_order, 0.57454427083333337));
  expect_bool(records, "psm.reference.rssm_encoder_order_exact",
              close(encoder_order, 0.9560546875));
  expect_bool(records, "psm.reference.all_exact", result);
  return result;
}

[[nodiscard]] bool no_training_contract(Records &records) {
  const bool result = boolean(records, "module_only") &&
                      !boolean(records, "optimizer_constructed") &&
                      number(records, "optimizer_steps") == 0.0 &&
                      number(records, "backward_calls") == 0.0 &&
                      !boolean(records, "launcher_augmentation") &&
                      !boolean(records, "training_authorized") &&
                      !boolean(records, "long_run_authorized") &&
                      !boolean(records,
                               "production_or_end_to_end_authorized") &&
                      !boolean(records, "follow_on_repair_authorized");
  if (!result) {
    fail(records, "no-training/module-only contract failed");
  }
  return result;
}

[[nodiscard]] int audit_log(std::string_view raw) {
  Records records = parse_records(raw);
  expect_text(records, "schema", "wikimyei.mtf_jepa_mae_vicreg.psm.v1");
  expect_text(records, "device", "cuda:0");
  expect_text(records, "model_seeds", "17,31,47");
  expect_text(records, "arms", "channel,channel_domain,channel_domain_scale,"
                                "channel_domain_scale_bin,encoder");
  expect_text(records, "representation_width", "96");
  expect_text(records, "psm.attempt.consumed", "true");
  expect_text(records, "execution_status", "psm_measurements_complete");
  expect_text(records, "psm.projection.rssm_hash", "f8c9f35282de2ee0");
  expect_text(records, "psm.projection.psm_hash", "ac8a43fd65b2c8a8");

  const bool no_training = no_training_contract(records);
  std::array<ArmData, 5> arms{};
  for (std::size_t arm = 0; arm < kArms.size(); ++arm) {
    arms[arm] = read_arm(records, kArms[arm]);
    verify_arm_summary(records, kArms[arm], arms[arm]);
  }
  const bool references = verify_reference_endpoints(records);
  const auto shuffles = recompute_shuffle_controls(records, arms);
  const bool shuffled_controls = shuffles.continuous && shuffles.order;

  const bool capture = boolean(records, "psm.data.identity_exact") &&
                       all_seed_flag(records, "public_sandwich_exact") &&
                       all_seed_flag(records, "direct_encoder_exact") &&
                       all_seed_flag(records, "repeated_capture_exact") &&
                       all_seed_flag(records, "production_order_exact") &&
                       all_seed_flag(records, "cardinality_exact") &&
                       all_seed_flag(records, "token_layout_exact") &&
                       boolean(records,
                               "psm.prefit.cross_seed_token_structure_exact") &&
                       boolean(records, "psm.prefit.metadata_plan_exact");
  const bool parameters =
      all_seed_flag(records, "parameters_and_rng_unchanged");
  const bool partitions = boolean(records, "psm.partition.pass") &&
                          all_seed_flag(records, "feature_contracts_exact");
  const bool projection = boolean(records, "psm.projection.pass");
  const bool deterministic = boolean(records, "psm.permutations_valid") &&
                             boolean(records, "psm.order_shuffle_balanced") &&
                             boolean(records, "psm.bootstrap.valid") &&
                             boolean(records, "psm.ridge.pass");
  expect_bool(records, "psm.prefit.mechanics_pass",
              capture && parameters && partitions && projection &&
                  deterministic && boolean(records, "psm.tokenizer_plan.pass") &&
                  boolean(records, "psm.token_layout.pass"));

  gate::GateInput input{};
  input.encoder_minus_channel = recompute_continuous(
      records, "psm.summary.contrast.encoder_minus_channel", arms[4], arms[0]);
  input.encoder_order = recompute_order(records, "encoder", arms[4]);
  input.channel_order = recompute_order(records, "channel", arms[0]);
  for (std::size_t candidate = 0; candidate < 3; ++candidate) {
    const std::string arm(kArms[candidate + 1]);
    const std::string prefix = "psm.summary.contrast." + arm;
    input.candidates[candidate].minus_encoder = recompute_continuous(
        records, prefix + "_minus_encoder", arms[candidate + 1], arms[4]);
    input.candidates[candidate].minus_channel = recompute_continuous(
        records, prefix + "_minus_channel", arms[candidate + 1], arms[0]);
    input.candidates[candidate].order =
        recompute_order(records, kArms[candidate + 1], arms[candidate + 1]);
  }
  input.validity = {.no_training_or_end_to_end = no_training,
                    .capture_and_identity_exact = capture,
                    .parameters_and_rng_unchanged = parameters,
                    .partitions_valid = partitions,
                    .projection_valid = projection,
                    .deterministic_tables_valid = deterministic,
                    .references_reproduced = references,
                    .shuffled_controls_pass = shuffled_controls};
  const auto evaluated = gate::evaluate(input);
  expect_bool(records, "psm.summary.validity.numeric_inputs",
              evaluated.numeric_inputs_valid);
  expect_bool(records, "psm.summary.validity.capture_and_identity_exact",
              capture);
  expect_bool(records,
              "psm.summary.validity.parameters_and_rng_unchanged", parameters);
  expect_bool(records, "psm.summary.validity.partitions_valid", partitions);
  expect_bool(records, "psm.summary.validity.projection_valid", projection);
  expect_bool(records, "psm.summary.validity.deterministic_tables_valid",
              deterministic);
  expect_bool(records, "psm.summary.validity.references_reproduced",
              references);
  expect_bool(records, "psm.summary.validity.continuous_shuffle_pass",
              shuffles.continuous);
  expect_bool(records, "psm.summary.validity.order_shuffle_pass",
              shuffles.order);
  expect_bool(records, "psm.summary.validity.mechanics_valid",
              evaluated.mechanics_valid);
  expect_bool(records, "psm.summary.gate.boundary_reproduced",
              evaluated.boundary_reproduced);
  expect_text(records, "psm.summary.gate.classification",
              gate::terminal_classification_name(evaluated.classification));

  const bool passed = records.errors.empty();
  std::cout << std::boolalpha << std::setprecision(17);
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.psm_audit.v1\n";
  std::cout << "audit.machine_line_count=" << records.machine_line_count << '\n';
  std::cout << "audit.duplicate_key_count=" << records.duplicate_key_count
            << '\n';
  std::cout << "audit.numeric_value_count=" << records.numeric_value_count
            << '\n';
  std::cout << "audit.nonfinite_value_count=" << records.nonfinite_value_count
            << '\n';
  std::cout << "audit.unique_finite_machine_keys="
            << (records.duplicate_key_count == 0 &&
                records.nonfinite_value_count == 0)
            << '\n';
  std::cout << "audit.references_exact=" << references << '\n';
  std::cout << "audit.arm_summaries_recomputed=true\n";
  std::cout << "audit.contrasts_recomputed=true\n";
  std::cout << "audit.gate_inputs_recomputed=true\n";
  std::cout << "audit.bootstrap_intervals_recomputed=false\n";
  std::cout << "audit.bootstrap_interval_limitation="
               "authoritative_log_does_not_emit_raw_predictions\n";
  std::cout << "audit.gate.boundary_reproduced="
            << evaluated.boundary_reproduced << '\n';
  std::cout << "audit.gate.classification="
            << gate::terminal_classification_name(evaluated.classification)
            << '\n';
  std::cout << "audit.error_count=" << records.errors.size() << '\n';
  for (std::size_t index = 0; index < records.errors.size(); ++index) {
    std::cout << "audit.error_" << index << '=' << records.errors[index] << '\n';
  }
  std::cout << "audit.pass=" << passed << '\n';
  return passed ? 0 : 3;
}

void expect(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void run_self_test() {
  const auto clean = parse_records("schema=x\na=1\nb=true\n");
  expect(clean.errors.empty() && clean.machine_line_count == 3 &&
             clean.numeric_value_count == 1,
         "clean parser fixture");
  const auto duplicate = parse_records("a=1\na=2\n");
  expect(duplicate.duplicate_key_count == 1 && !duplicate.errors.empty(),
         "duplicate parser fixture");
  const auto nonfinite = parse_records("a=nan\n");
  expect(!nonfinite.errors.empty(), "non-finite parser fixture");

  gate::GateInput input{};
  input.validity = {.no_training_or_end_to_end = true,
                    .capture_and_identity_exact = true,
                    .parameters_and_rng_unchanged = true,
                    .partitions_valid = true,
                    .projection_valid = true,
                    .deterministic_tables_valid = true,
                    .references_reproduced = true,
                    .shuffled_controls_pass = true};
  input.encoder_minus_channel =
      {.point = 0.03,
       .low = 0.01,
       .high = 0.05,
       .seed_deltas = {0.02, 0.03, 0.04},
       .family_deltas = {0.01, 0.01, 0.01, 0.01}};
  input.encoder_order =
      {.point = 0.70,
       .low = 0.60,
       .high = 0.80,
       .seed_points = {0.65, 0.70, 0.75}};
  input.channel_order =
      {.point = 0.50,
       .low = 0.45,
       .high = 0.55,
       .seed_points = {0.49, 0.50, 0.51}};
  input.candidates[0].minus_encoder =
      {.point = 0.0,
       .low = -0.01,
       .high = 0.01,
       .seed_deltas = {0.0, 0.0, 0.0},
       .family_deltas = {0.0, 0.0, 0.0, 0.0}};
  input.candidates[0].minus_channel = input.encoder_minus_channel;
  input.candidates[0].order = input.encoder_order;
  input.candidates[1] = input.candidates[0];
  input.candidates[2] = input.candidates[0];
  const auto evaluated = gate::evaluate(input);
  expect(evaluated.classification ==
             gate::TerminalClassification::domain_separation_sufficient,
         "independent terminal gate fixture");
}

[[nodiscard]] std::string read_bytes(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    throw std::runtime_error("cannot open log: " + path.string());
  }
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

} // namespace

int main(int argc, char **argv) {
  try {
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
      run_self_test();
      std::cout << "psm_log_auditor_self_test=PASS\n";
      return 0;
    }
    if (argc != 2) {
      throw std::runtime_error(
          "usage: test_pooling_structure_mechanism_map_log_auditor "
          "<authoritative.log>|--self-test");
    }
    return audit_log(read_bytes(argv[1]));
  } catch (const std::exception &error) {
    std::cerr << "psm_log_auditor=FAIL error=" << error.what() << '\n';
    return 2;
  }
}
