#include "representation_surface_sufficiency_map_reference_auditor.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace auditor =
    cuwacunu::tests::mtf_surface_sufficiency_map_reference_auditor;

namespace {

constexpr std::string_view kFixtureReferenceSchema = "self_test.jmcd.v1";
constexpr std::string_view kFixtureCandidateSchema = "self_test.rssm.v1";

void expect(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::string replace_once(std::string input, std::string_view from,
                         std::string_view to) {
  const std::size_t position = input.find(from);
  expect(position != std::string::npos,
         "mutation source is absent: " + std::string(from));
  input.replace(position, from.size(), to);
  return input;
}

std::string reference_fixture() {
  return "schema=" + std::string(kFixtureReferenceSchema) + "\n" +
         "seed_17.arm.jepa_mae_only.step_0.probe.area=0.51\n" +
         "seed_31.arm.jepa_mae_only.step_0.geometry.rank=0.73\n" +
         "summary.arm.jepa_mae_only.step_0.probe_area_fixed_seed_mean=0.52\n" +
         "control.raw_equal_width.area=0.60\n" +
         "control.raw_equal_width.final_macro_r2=0.71\n" +
         "unselected.note=ignored\n";
}

std::string candidate_fixture() {
  return replace_once(reference_fixture(),
                      "schema=" + std::string(kFixtureReferenceSchema),
                      "schema=" + std::string(kFixtureCandidateSchema));
}

auditor::AuditContract fixture_contract() {
  const std::string reference = reference_fixture();
  return auditor::fixture_contract(reference,
                                   std::string(kFixtureReferenceSchema),
                                   std::string(kFixtureCandidateSchema));
}

void test_selector_contract() {
  expect(
      auditor::is_accepted_key("seed_17.arm.jepa_mae_only.step_0.probe.area"),
      "seed-17 probe key is selected");
  expect(auditor::is_accepted_key(
             "seed_31.arm.jepa_mae_only.step_0.geometry.rank"),
         "seed-31 geometry key is selected");
  expect(auditor::is_accepted_key(
             "seed_47.arm.jepa_mae_only.step_0.probe.family.future"),
         "seed-47 nested probe key is selected");
  expect(
      auditor::is_accepted_key(
          "summary.arm.jepa_mae_only.step_0.family_future_r2_fixed_seed_mean"),
      "frozen summary family key is selected");
  expect(auditor::is_accepted_key(
             "summary.arm.jepa_mae_only.step_0.geometry."
             "channel_min_active_dimension_fraction_fixed_seed_mean"),
         "frozen summary geometry key is selected");
  expect(
      !auditor::is_accepted_key("seed_19.arm.jepa_mae_only.step_0.probe.area"),
      "unaccepted seed is excluded");
  expect(!auditor::is_accepted_key("seed_17.arm.jepa_only.step_0.probe.area"),
         "unaccepted arm is excluded");
  expect(
      !auditor::is_accepted_key("seed_17.arm.jepa_mae_only.step_1.probe.area"),
      "nonzero step is excluded");
  expect(
      !auditor::is_accepted_key(
          "summary.arm.jepa_mae_only.step_0.family_unknown_r2_fixed_seed_mean"),
      "non-frozen summary key is excluded");
  expect(auditor::is_legacy_raw_key("control.raw_equal_width.area"),
         "legacy raw prefix is selected");
  expect(!auditor::is_legacy_raw_key("control.normalized_raw.area"),
         "normalized raw prefix is excluded from legacy continuity");
}

void test_exact_pass_and_bounded_selection() {
  const std::string reference = reference_fixture();
  const std::string candidate = candidate_fixture();
  const auto contract = fixture_contract();
  auto result = auditor::audit(reference, candidate, contract);
  expect(result.passed, "exact selected records and schemas pass");
  expect(result.reference_contract_pass && result.candidate_contract_pass,
         "both sides report contract pass");
  expect(result.accepted.passed && result.legacy_raw.passed,
         "both frozen namespaces pass independently");
  expect(std::string(auditor::failure_name(result)) == "none",
         "passing audit has no failure classification");

  const std::string with_irrelevant_candidate_line =
      candidate + "rssm.unselected.diagnostic=may_differ\n";
  result = auditor::audit(reference, with_irrelevant_candidate_line, contract);
  expect(result.passed,
         "candidate lines outside both frozen selectors do not affect audit");
}

void test_accepted_namespace_rejections() {
  const auto contract = fixture_contract();
  const std::string baseline = candidate_fixture();
  const std::string line = "seed_17.arm.jepa_mae_only.step_0.probe.area=0.51\n";

  auto candidate = baseline + line;
  auto result = auditor::audit(reference_fixture(), candidate, contract);
  expect(!result.passed && result.accepted.candidate_contract_pass == false &&
             result.candidate.accepted.duplicate_key_count == 1 &&
             result.candidate.accepted.duplicate_line_count == 1,
         "accepted duplicate is counted and rejected");
  expect(std::string(auditor::failure_name(result)) ==
             "accepted_step_zero_reference_not_reproduced",
         "accepted duplicate uses accepted-reference failure");

  candidate = replace_once(baseline, line, "");
  result = auditor::audit(reference_fixture(), candidate, contract);
  expect(!result.passed && result.accepted.candidate_missing_key_count == 1 &&
             result.accepted.candidate_extra_key_count == 0,
         "accepted missing key is counted and rejected");

  candidate =
      baseline + "seed_47.arm.jepa_mae_only.step_0.probe.extra_metric=9\n";
  result = auditor::audit(reference_fixture(), candidate, contract);
  expect(!result.passed && result.accepted.candidate_missing_key_count == 0 &&
             result.accepted.candidate_extra_key_count == 1,
         "accepted extra key is counted and rejected");

  candidate = replace_once(baseline, "probe.area=0.51", "probe.area=0.59");
  result = auditor::audit(reference_fixture(), candidate, contract);
  expect(!result.passed && result.accepted.candidate_key_set_exact &&
             !result.accepted.candidate_value_set_exact,
         "accepted value change with an unchanged key set is rejected");
}

void test_legacy_namespace_rejections() {
  const auto contract = fixture_contract();
  const std::string baseline = candidate_fixture();
  const std::string line = "control.raw_equal_width.area=0.60\n";

  auto candidate = baseline + line;
  auto result = auditor::audit(reference_fixture(), candidate, contract);
  expect(!result.passed &&
             result.candidate.legacy_raw.duplicate_key_count == 1 &&
             result.candidate.legacy_raw.duplicate_line_count == 1,
         "legacy duplicate is counted and rejected");
  expect(std::string(auditor::failure_name(result)) ==
             "legacy_raw_reference_not_reproduced",
         "legacy duplicate uses legacy-reference failure");

  candidate = replace_once(baseline, line, "");
  result = auditor::audit(reference_fixture(), candidate, contract);
  expect(!result.passed && result.legacy_raw.candidate_missing_key_count == 1 &&
             result.legacy_raw.candidate_extra_key_count == 0,
         "legacy missing key is counted and rejected");

  candidate = baseline + "control.raw_equal_width.extra_metric=9\n";
  result = auditor::audit(reference_fixture(), candidate, contract);
  expect(!result.passed && result.legacy_raw.candidate_missing_key_count == 0 &&
             result.legacy_raw.candidate_extra_key_count == 1,
         "legacy extra key is counted and rejected");

  candidate = replace_once(baseline, "raw_equal_width.area=0.60",
                           "raw_equal_width.area=0.69");
  result = auditor::audit(reference_fixture(), candidate, contract);
  expect(!result.passed && result.legacy_raw.candidate_key_set_exact &&
             !result.legacy_raw.candidate_value_set_exact,
         "legacy value change with an unchanged key set is rejected");
}

void test_schema_rejections() {
  const auto contract = fixture_contract();
  const std::string baseline = candidate_fixture();

  auto candidate =
      replace_once(baseline, "schema=" + std::string(kFixtureCandidateSchema),
                   "schema=wrong.rssm.v1");
  auto result = auditor::audit(reference_fixture(), candidate, contract);
  expect(!result.passed && !result.candidate_schema_pass,
         "wrong candidate schema is rejected");
  expect(std::string(auditor::failure_name(result)) ==
             "invalid_numeric_or_mechanics",
         "schema mismatch fails as invalid mechanics");

  candidate = replace_once(
      baseline, "schema=" + std::string(kFixtureCandidateSchema) + "\n", "");
  result = auditor::audit(reference_fixture(), candidate, contract);
  expect(!result.passed && !result.candidate_schema_pass &&
             result.candidate.schema_values.empty(),
         "missing candidate schema is rejected");

  candidate =
      baseline + "schema=" + std::string(kFixtureCandidateSchema) + "\n";
  result = auditor::audit(reference_fixture(), candidate, contract);
  expect(!result.passed && !result.candidate_schema_pass &&
             result.candidate.schema_values.size() == 2,
         "duplicate candidate schema is rejected");

  const std::string wrong_reference = replace_once(
      reference_fixture(), "schema=" + std::string(kFixtureReferenceSchema),
      "schema=wrong.jmcd.v1");
  result = auditor::audit(wrong_reference, baseline, contract);
  expect(
      !result.passed && !result.reference_schema_pass &&
          !result.reference_file_identity_pass,
      "wrong reference schema and changed frozen file identity are rejected");
}

void test_reference_identity_and_utf8_rejections() {
  const auto contract = fixture_contract();
  const std::string baseline = candidate_fixture();
  const std::string changed_reference =
      reference_fixture() + "unselected.reference_mutation=1\n";
  auto result = auditor::audit(changed_reference, baseline, contract);
  expect(!result.passed && !result.reference_file_identity_pass &&
             result.reference_schema_pass,
         "reference mutation outside selectors still fails frozen identity");

  std::string invalid_candidate = baseline;
  invalid_candidate.push_back(static_cast<char>(0xff));
  result = auditor::audit(reference_fixture(), invalid_candidate, contract);
  expect(!result.passed && !result.candidate.utf8_valid,
         "invalid candidate UTF-8 is rejected");

  std::string invalid_reference = reference_fixture();
  invalid_reference.push_back(static_cast<char>(0xc0));
  result = auditor::audit(invalid_reference, baseline, contract);
  expect(!result.passed && !result.reference.utf8_valid,
         "invalid reference UTF-8 is rejected");
}

void run_self_tests() {
  test_selector_contract();
  test_exact_pass_and_bounded_selection();
  test_accepted_namespace_rejections();
  test_legacy_namespace_rejections();
  test_schema_rejections();
  test_reference_identity_and_utf8_rejections();
}

std::string read_bytes(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    throw std::runtime_error("cannot open input: " + path.string());
  }
  std::string bytes{std::istreambuf_iterator<char>(input),
                    std::istreambuf_iterator<char>()};
  if (input.bad()) {
    throw std::runtime_error("cannot read input: " + path.string());
  }
  return bytes;
}

struct Cli {
  bool self_test{false};
  std::optional<std::filesystem::path> reference{};
  std::optional<std::filesystem::path> candidate{};
};

Cli parse_cli(int argc, char **argv) {
  if (argc == 1 || (argc == 2 && std::string_view(argv[1]) == "--self-test")) {
    return {.self_test = true};
  }
  if (argc != 5) {
    throw std::invalid_argument(
        "expected --self-test or --reference PATH --candidate PATH");
  }
  Cli cli{};
  for (int index = 1; index < argc; index += 2) {
    const std::string_view option(argv[index]);
    const std::filesystem::path path(argv[index + 1]);
    if (option == "--reference" && !cli.reference.has_value()) {
      cli.reference = path;
    } else if (option == "--candidate" && !cli.candidate.has_value()) {
      cli.candidate = path;
    } else {
      throw std::invalid_argument("unknown or duplicate argument: " +
                                  std::string(option));
    }
  }
  if (!cli.reference.has_value() || !cli.candidate.has_value()) {
    throw std::invalid_argument("both reference and candidate are required");
  }
  return cli;
}

void emit_audit(const auditor::AuditResult &result,
                const std::filesystem::path &reference,
                const std::filesystem::path &candidate) {
  std::cout << std::boolalpha;
  std::cout << "schema=" << auditor::kAuditSchema << '\n';
  std::cout << "reference.path=" << reference.string() << '\n';
  std::cout << "candidate.path=" << candidate.string() << '\n';
  std::cout << "expected.reference_size_bytes=" << auditor::kReferenceSizeBytes
            << '\n';
  std::cout << "expected.reference_sha256=" << auditor::kReferenceSha256
            << '\n';
  std::cout << "expected.reference_schema=" << auditor::kReferenceSchema
            << '\n';
  std::cout << "expected.candidate_schema=" << auditor::kCandidateSchema
            << '\n';
  std::cout << "reference.utf8_valid=" << result.reference.utf8_valid << '\n';
  std::cout << "candidate.utf8_valid=" << result.candidate.utf8_valid << '\n';
  std::cout << "reference.size_bytes=" << result.reference.size_bytes << '\n';
  std::cout << "reference.sha256=" << result.reference.file_sha256 << '\n';
  std::cout << "reference.file_identity_exact="
            << result.reference_file_identity_pass << '\n';
  std::cout << "reference.schema_exact=" << result.reference_schema_pass
            << '\n';
  std::cout << "candidate.schema_exact=" << result.candidate_schema_pass
            << '\n';
  std::cout << "accepted.expected_key_count=" << auditor::kAcceptedKeyCount
            << '\n';
  std::cout << "accepted.expected_key_set_sha256="
            << auditor::kAcceptedKeySetSha256 << '\n';
  std::cout << "accepted.expected_key_value_sha256="
            << auditor::kAcceptedKeyValueSha256 << '\n';
  std::cout << "accepted.reference_key_count="
            << result.reference.accepted.unique_key_count << '\n';
  std::cout << "accepted.reference_duplicate_key_count="
            << result.reference.accepted.duplicate_key_count << '\n';
  std::cout << "accepted.reference_key_set_sha256="
            << result.reference.accepted.key_set_sha256 << '\n';
  std::cout << "accepted.reference_key_value_sha256="
            << result.reference.accepted.key_value_sha256 << '\n';
  std::cout << "accepted.candidate_key_count="
            << result.candidate.accepted.unique_key_count << '\n';
  std::cout << "accepted.candidate_duplicate_key_count="
            << result.candidate.accepted.duplicate_key_count << '\n';
  std::cout << "accepted.candidate_missing_key_count="
            << result.accepted.candidate_missing_key_count << '\n';
  std::cout << "accepted.candidate_extra_key_count="
            << result.accepted.candidate_extra_key_count << '\n';
  std::cout << "accepted.candidate_key_set_sha256="
            << result.candidate.accepted.key_set_sha256 << '\n';
  std::cout << "accepted.candidate_key_value_sha256="
            << result.candidate.accepted.key_value_sha256 << '\n';
  std::cout << "accepted.key_set_exact="
            << result.accepted.candidate_key_set_exact << '\n';
  std::cout << "accepted.key_value_exact="
            << result.accepted.candidate_value_set_exact << '\n';
  std::cout << "accepted.audit_pass=" << result.accepted.passed << '\n';
  std::cout << "legacy_raw.expected_key_count=" << auditor::kLegacyRawKeyCount
            << '\n';
  std::cout << "legacy_raw.expected_key_set_sha256="
            << auditor::kLegacyRawKeySetSha256 << '\n';
  std::cout << "legacy_raw.expected_key_value_sha256="
            << auditor::kLegacyRawKeyValueSha256 << '\n';
  std::cout << "legacy_raw.reference_key_count="
            << result.reference.legacy_raw.unique_key_count << '\n';
  std::cout << "legacy_raw.reference_duplicate_key_count="
            << result.reference.legacy_raw.duplicate_key_count << '\n';
  std::cout << "legacy_raw.reference_key_set_sha256="
            << result.reference.legacy_raw.key_set_sha256 << '\n';
  std::cout << "legacy_raw.reference_key_value_sha256="
            << result.reference.legacy_raw.key_value_sha256 << '\n';
  std::cout << "legacy_raw.candidate_key_count="
            << result.candidate.legacy_raw.unique_key_count << '\n';
  std::cout << "legacy_raw.candidate_duplicate_key_count="
            << result.candidate.legacy_raw.duplicate_key_count << '\n';
  std::cout << "legacy_raw.candidate_missing_key_count="
            << result.legacy_raw.candidate_missing_key_count << '\n';
  std::cout << "legacy_raw.candidate_extra_key_count="
            << result.legacy_raw.candidate_extra_key_count << '\n';
  std::cout << "legacy_raw.candidate_key_set_sha256="
            << result.candidate.legacy_raw.key_set_sha256 << '\n';
  std::cout << "legacy_raw.candidate_key_value_sha256="
            << result.candidate.legacy_raw.key_value_sha256 << '\n';
  std::cout << "legacy_raw.key_set_exact="
            << result.legacy_raw.candidate_key_set_exact << '\n';
  std::cout << "legacy_raw.key_value_exact="
            << result.legacy_raw.candidate_value_set_exact << '\n';
  std::cout << "legacy_raw.audit_pass=" << result.legacy_raw.passed << '\n';
  std::cout << "reference.contract_pass=" << result.reference_contract_pass
            << '\n';
  std::cout << "candidate.contract_pass=" << result.candidate_contract_pass
            << '\n';
  std::cout << "audit.failure=" << auditor::failure_name(result) << '\n';
  std::cout << "audit.pass=" << result.passed << '\n';
  std::cout << "result=" << (result.passed ? "PASS" : "FAIL") << '\n';
}

} // namespace

int main(int argc, char **argv) {
  try {
    const Cli cli = parse_cli(argc, argv);
    if (cli.self_test) {
      run_self_tests();
      std::cout << "schema=" << auditor::kAuditSchema << ".self_test\n";
      std::cout << "self_test.exact_pass=true\n";
      std::cout << "self_test.accepted_duplicate_missing_extra_changed_"
                   "rejected=true\n";
      std::cout
          << "self_test.legacy_duplicate_missing_extra_changed_rejected=true\n";
      std::cout << "self_test.schema_rejected=true\n";
      std::cout << "self_test.reference_identity_rejected=true\n";
      std::cout << "self_test.invalid_utf8_rejected=true\n";
      std::cout << "self_test.pass=true\n";
      std::cout << "result=PASS\n";
      return 0;
    }

    const std::string reference_raw = read_bytes(*cli.reference);
    const std::string candidate_raw = read_bytes(*cli.candidate);
    const auto result = auditor::audit(reference_raw, candidate_raw);
    emit_audit(result, *cli.reference, *cli.candidate);
    return result.passed ? 0 : 1;
  } catch (const std::invalid_argument &error) {
    std::cerr << "representation_surface_sufficiency_map_reference_auditor="
                 "FAIL\n";
    std::cerr << "reason=" << error.what() << '\n';
    return 2;
  } catch (const std::exception &error) {
    std::cerr << "representation_surface_sufficiency_map_reference_auditor="
                 "FAIL\n";
    std::cerr << "reason=" << error.what() << '\n';
    return 1;
  }
}
