#include "structured_readout_repair_gate.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <clocale>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace srr_gate =
    cuwacunu::tests::structured_readout_repair_gate;
namespace psm_gate =
    cuwacunu::tests::pooling_structure_mechanism_map_gate;

namespace {

constexpr std::array<std::string_view, 3> kSeeds{"17", "31", "47"};
constexpr std::array<std::string_view, 4> kArms{
    "channel", "offline_cdsb", "shadow", "encoder"};
constexpr std::array<std::string_view, 4> kFamilies{
    "multiscale_state", "order_regime", "cross_channel", "future"};
constexpr std::array<int, 12> kTargetFamily{
    0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 3};
constexpr std::array<int, 4> kSamples{32, 64, 128, 256};
constexpr std::array<double, 6> kRidgeGrid{
    1.0e-5, 1.0e-4, 1.0e-3, 1.0e-2, 1.0e-1, 1.0};
constexpr std::array<std::string_view, 6> kDatasets{
    "probe_train", "probe_validation", "test", "reversed_train",
    "reversed_validation", "reversed_test"};
constexpr std::size_t kRows = 256;
constexpr std::size_t kTargets = 12;
constexpr std::size_t kOrderRows = 512;
constexpr std::size_t kBootstrapReplicates = 512;
constexpr uint64_t kBootstrapSeed = 8387496322364763509ULL;
constexpr uint64_t kContinuousFitShuffleTag = 0x7273736d5f74726eULL;
constexpr uint64_t kContinuousValidationShuffleTag = 0x7273736d5f76616cULL;
constexpr uint64_t kContinuousTestShuffleTag = 0x7273736d5f746573ULL;
constexpr uint64_t kOrderFitShuffleTag = 0x7273736d5f6f7472ULL;
constexpr uint64_t kOrderValidationShuffleTag = 0x7273736d5f6f7661ULL;
constexpr uint64_t kOrderTestShuffleTag = 0x7273736d5f6f7465ULL;
constexpr double kTolerance = 1.0e-12;

struct PermutationSpec {
  const char *name;
  std::size_t rows;
  uint64_t tag;
  const char *hash;
};

constexpr std::array<PermutationSpec, 9> kExpectedPermutations{{
    {"continuous_fit", 256, kContinuousFitShuffleTag,
     "d87414c3b552ad82"},
    {"continuous_validation", 128, kContinuousValidationShuffleTag,
     "8028fd2c0a7a18e2"},
    {"continuous_test", 256, kContinuousTestShuffleTag,
     "6934a415235cbb82"},
    {"order_fit_n_32", 64, kOrderFitShuffleTag, "fd6b0fe50afa70c2"},
    {"order_fit_n_64", 128, kOrderFitShuffleTag, "f478db41e84d1182"},
    {"order_fit_n_128", 256, kOrderFitShuffleTag, "f5b3055f6c1d7ce2"},
    {"order_fit_n_256", 512, kOrderFitShuffleTag, "384f290f72bfd2ce"},
    {"order_validation", 256, kOrderValidationShuffleTag,
     "652fbc6549c6e3e2"},
    {"order_test", 512, kOrderTestShuffleTag, "5c55fc87092c5f46"},
}};

constexpr std::string_view kParentSha256 =
    "8243798d5af03d66257cbd1fd9da49a16ff7d6ba3f9e6bc54b5568dae41aa8b9";
constexpr std::size_t kParentBytes = 255304;

constexpr std::array<std::array<double, 3>, 3> kExpectedAulc{{
    {0.51029806802395417, 0.51214336890601575, 0.53534605970628402},
    {0.60310336284296084, 0.58334872682440442, 0.59273298270071495},
    {0.59528657538535634, 0.57992865599289245, 0.57468040681240407},
}};
constexpr std::array<std::array<double, 3>, 3> kExpectedOrderAulc{{
    {0.56884765625, 0.5849609375, 0.56982421875},
    {0.92236328125, 0.93701171875, 0.92919921875},
    {0.97021484375, 0.93603515625, 0.96435546875},
}};
constexpr std::array<std::array<double, 4>, 3> kExpectedFamilyAulc{{
    {0.4045698296448732, 0.47928054446074547, 0.53872892886456747,
     0.65447069254481927},
    {0.59464087875418847, 0.508459546484064,
     0.45963705102330704, 0.80950928689588075},
    {0.57462964827738061, 0.50661709817337297,
     0.43553250772990609, 0.81641493007354382},
}};

struct Records {
  std::map<std::string, std::string> values{};
  std::set<std::string> accessed_keys{};
  std::vector<std::string> errors{};
  std::size_t machine_line_count{0};
  std::size_t duplicate_key_count{0};
  std::size_t recognized_runtime_noise_count{0};
  std::size_t malformed_line_count{0};
  std::size_t numeric_value_count{0};
  std::size_t nonfinite_value_count{0};
  std::size_t csv_value_count{0};
  std::size_t csv_nonfinite_value_count{0};
  std::size_t reference_comparison_count{0};
  std::size_t reference_comparison_failure_count{0};
  std::size_t reference_error_count{0};
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

[[nodiscard]] bool ends_with(std::string_view value,
                             std::string_view suffix) {
  return value.size() >= suffix.size() &&
         value.substr(value.size() - suffix.size()) == suffix;
}

[[nodiscard]] bool recognized_runtime_noise(std::string_view line) {
  return line.find("[source_runtime_t] initializing static-global source "
                   "snapshot (single mutable cache updated by explicit "
                   "runtime call)") != std::string_view::npos ||
         line.find("[source_runtime_t] finalizing static-global source "
                   "snapshot (last_config_path=<none>)") !=
             std::string_view::npos;
}

[[nodiscard]] Records parse_records(std::string_view raw,
                                    bool reject_malformed = false) {
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
    } else if (!line.empty() && recognized_runtime_noise(line)) {
      ++result.recognized_runtime_noise_count;
    } else if (!line.empty()) {
      ++result.malformed_line_count;
      if (reject_malformed) {
        result.errors.push_back("malformed non-machine log line at index " +
                                std::to_string(result.malformed_line_count));
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

void fail_reference(Records &records, const std::string &message) {
  fail(records, message);
  ++records.reference_error_count;
}

[[nodiscard]] const std::string &required(Records &records,
                                          const std::string &key) {
  records.accessed_keys.insert(key);
  const auto found = records.values.find(key);
  if (found == records.values.end()) {
    fail(records, "missing machine key: " + key);
    static const std::string empty{};
    return empty;
  }
  return found->second;
}

[[nodiscard]] double number(Records &records, const std::string &key) {
  const std::string &text = required(records, key);
  double result = 0.0;
  if (!parse_complete_double(text, result) || !std::isfinite(result)) {
    fail(records, "invalid finite number: " + key + "=" + text);
    return 0.0;
  }
  return result;
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

[[nodiscard]] bool close(double left, double right,
                         double tolerance = kTolerance) {
  return std::isfinite(left) && std::isfinite(right) &&
         std::abs(left - right) <= tolerance;
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
  if (boolean(records, key) != expected) {
    fail(records, "unexpected boolean: " + key);
  }
}

void expect_close(Records &records, const std::string &key, double expected,
                  double tolerance = kTolerance) {
  const double observed = number(records, key);
  if (!close(observed, expected, tolerance)) {
    std::ostringstream message;
    message << std::setprecision(17) << "numeric mismatch: " << key
            << " observed=" << observed << " expected=" << expected;
    fail(records, message.str());
  }
}

void compare_text(Records &left, Records &right, const std::string &left_key,
                  const std::string &right_key) {
  const bool external_reference = &left != &right;
  if (external_reference) {
    ++left.reference_comparison_count;
  }
  const std::size_t left_errors_before = left.errors.size();
  const std::size_t right_errors_before = right.errors.size();
  const std::string &lhs = required(left, left_key);
  const std::string &rhs = required(right, right_key);
  if (lhs != rhs) {
    fail(left, "parent text mismatch: " + left_key + " != " + right_key);
  }
  if (external_reference &&
      (left.errors.size() != left_errors_before ||
       right.errors.size() != right_errors_before)) {
    ++left.reference_comparison_failure_count;
    left.reference_error_count += left.errors.size() - left_errors_before;
  }
}

void compare_number(Records &left, Records &right,
                    const std::string &left_key,
                    const std::string &right_key,
                    double tolerance = kTolerance) {
  const bool external_reference = &left != &right;
  if (external_reference) {
    ++left.reference_comparison_count;
  }
  const std::size_t left_errors_before = left.errors.size();
  const std::size_t right_errors_before = right.errors.size();
  const double lhs = number(left, left_key);
  const double rhs = number(right, right_key);
  if (!close(lhs, rhs, tolerance)) {
    fail(left, "parent numeric mismatch: " + left_key + " != " + right_key);
  }
  if (external_reference &&
      (left.errors.size() != left_errors_before ||
       right.errors.size() != right_errors_before)) {
    ++left.reference_comparison_failure_count;
    left.reference_error_count += left.errors.size() - left_errors_before;
  }
}

[[nodiscard]] std::vector<double> parse_csv(Records &records,
                                             const std::string &key,
                                             std::size_t expected_values) {
  const std::string &raw = required(records, key);
  std::vector<double> result;
  result.reserve(expected_values);
  std::size_t begin = 0;
  while (begin <= raw.size() && !raw.empty()) {
    const std::size_t comma = raw.find(',', begin);
    const std::size_t end = comma == std::string::npos ? raw.size() : comma;
    const std::string_view item(raw.data() + begin, end - begin);
    double value = 0.0;
    ++records.csv_value_count;
    if (!parse_complete_double(item, value) || !std::isfinite(value)) {
      ++records.csv_nonfinite_value_count;
      fail(records, "invalid CSV number: " + key + " at index " +
                        std::to_string(result.size()));
      value = 0.0;
    }
    result.push_back(value);
    if (comma == std::string::npos) {
      break;
    }
    begin = comma + 1;
  }
  if (result.size() != expected_values) {
    fail(records, "CSV cardinality mismatch: " + key + " observed=" +
                      std::to_string(result.size()) + " expected=" +
                      std::to_string(expected_values));
    result.resize(expected_values, 0.0);
  }
  return result;
}

// Compact, dependency-free SHA-256.  This lets the auditor prove that its
// parent-log argument is the exact PSM-1 artifact frozen by the protocol.
class Sha256 {
public:
  void update(const uint8_t *data, std::size_t size) {
    for (std::size_t index = 0; index < size; ++index) {
      block_[block_size_++] = data[index];
      if (block_size_ == block_.size()) {
        transform(block_.data());
        bit_count_ += 512;
        block_size_ = 0;
      }
    }
  }

  [[nodiscard]] std::string finish() {
    bit_count_ += static_cast<uint64_t>(block_size_) * 8ULL;
    block_[block_size_++] = 0x80;
    if (block_size_ > 56) {
      while (block_size_ < 64) {
        block_[block_size_++] = 0;
      }
      transform(block_.data());
      block_size_ = 0;
    }
    while (block_size_ < 56) {
      block_[block_size_++] = 0;
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
      block_[block_size_++] =
          static_cast<uint8_t>((bit_count_ >> shift) & 0xffU);
    }
    transform(block_.data());
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (uint32_t value : state_) {
      output << std::setw(8) << value;
    }
    return output.str();
  }

private:
  [[nodiscard]] static uint32_t rotate(uint32_t value, unsigned bits) {
    return (value >> bits) | (value << (32U - bits));
  }

  void transform(const uint8_t *block) {
    static constexpr std::array<uint32_t, 64> constants{
        0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
        0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
        0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
        0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
        0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
        0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
        0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
        0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
        0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
        0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
        0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
        0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
        0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
        0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
        0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
        0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};
    std::array<uint32_t, 64> words{};
    for (std::size_t index = 0; index < 16; ++index) {
      const std::size_t offset = 4 * index;
      words[index] = static_cast<uint32_t>(block[offset]) << 24U |
                     static_cast<uint32_t>(block[offset + 1]) << 16U |
                     static_cast<uint32_t>(block[offset + 2]) << 8U |
                     static_cast<uint32_t>(block[offset + 3]);
    }
    for (std::size_t index = 16; index < words.size(); ++index) {
      const uint32_t s0 = rotate(words[index - 15], 7) ^
                          rotate(words[index - 15], 18) ^
                          (words[index - 15] >> 3U);
      const uint32_t s1 = rotate(words[index - 2], 17) ^
                          rotate(words[index - 2], 19) ^
                          (words[index - 2] >> 10U);
      words[index] = words[index - 16] + s0 + words[index - 7] + s1;
    }
    uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
    for (std::size_t index = 0; index < words.size(); ++index) {
      const uint32_t s1 = rotate(e, 6) ^ rotate(e, 11) ^ rotate(e, 25);
      const uint32_t choose = (e & f) ^ (~e & g);
      const uint32_t t1 = h + s1 + choose + constants[index] + words[index];
      const uint32_t s0 = rotate(a, 2) ^ rotate(a, 13) ^ rotate(a, 22);
      const uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
      const uint32_t t2 = s0 + majority;
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
  }

  std::array<uint32_t, 8> state_{
      0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
      0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
  std::array<uint8_t, 64> block_{};
  std::size_t block_size_{0};
  uint64_t bit_count_{0};
};

[[nodiscard]] std::string sha256(std::string_view bytes) {
  Sha256 hash;
  hash.update(reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size());
  return hash.finish();
}

[[nodiscard]] std::string read_bytes(const std::filesystem::path &path);

[[nodiscard]] std::filesystem::path find_repository_root(
    const std::filesystem::path &executable) {
  std::filesystem::path candidate =
      std::filesystem::canonical(executable).parent_path();
  while (!candidate.empty()) {
    if (std::filesystem::exists(
            candidate / "src/tests/bench/wikimyei/representation/encoding/"
                        "mtf_jepa_mae_vicreg/"
                        "STRUCTURED_READOUT_REPAIR_PROTOCOL.md") &&
        std::filesystem::exists(candidate / ".build/tests")) {
      return std::filesystem::canonical(candidate);
    }
    const auto parent = candidate.parent_path();
    if (parent == candidate) {
      break;
    }
    candidate = parent;
  }
  throw std::runtime_error("cannot locate cuwacunu repository root");
}

[[nodiscard]] std::size_t count_exact_line(std::string_view raw,
                                           std::string_view expected) {
  std::size_t result = 0;
  std::size_t begin = 0;
  while (begin < raw.size()) {
    const std::size_t newline = raw.find('\n', begin);
    const std::size_t end =
        newline == std::string_view::npos ? raw.size() : newline;
    std::string_view line = raw.substr(begin, end - begin);
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    result += line == expected;
    if (newline == std::string_view::npos) {
      break;
    }
    begin = newline + 1;
  }
  return result;
}

struct ParentEvidence {
  bool artifacts_exact{false};
  bool log_exact{false};
  bool classification_exact{false};
  bool attempt_count_exact{false};
  bool audit_pass{false};
  bool authorizations_false{false};
  std::size_t artifact_count{0};
};

struct ManifestEvidence {
  bool exact{false};
  bool exact_path_set{false};
  bool canonical_containment{false};
  bool protocol_digest_exact{false};
  bool runtime_binding_exact{false};
  bool auditor_binary_exact{false};
  bool mechanics_log_exact{false};
  bool preflight_log_exact{false};
  std::size_t entry_count{0};
  std::size_t error_count{0};
  std::size_t bytes{0};
  std::string sha256{};
  std::size_t auditor_binary_bytes{0};
  std::string auditor_binary_sha256{};
};

struct Matrix {
  std::size_t rows{0};
  std::size_t columns{0};
  std::vector<double> values{};

  [[nodiscard]] double operator()(std::size_t row,
                                  std::size_t column) const {
    return values[row * columns + column];
  }
};

void stable_mix(uint64_t &hash, uint64_t value) {
  hash ^= value;
  hash *= 0x100000001b3ULL;
}

[[nodiscard]] std::string hex64(uint64_t value) {
  std::ostringstream output;
  output << std::hex << std::setfill('0') << std::setw(16) << value;
  return output.str();
}

template <typename Value>
void stable_mix_native_bytes(uint64_t &hash, const Value &value) {
  static_assert(std::is_trivially_copyable_v<Value>);
  std::array<uint8_t, sizeof(Value)> bytes{};
  std::memcpy(bytes.data(), &value, sizeof(Value));
  for (const uint8_t byte : bytes) {
    stable_mix(hash, static_cast<uint64_t>(byte));
  }
}

// Mirrors hash_tensor_stable_bytes for the two CPU tensor dtypes represented
// by this dependency-free auditor.  PyTorch's stable ABI values are Long=4
// and Double=7; the frozen permutation hashes exercise the Long path before
// the Double path is trusted for the target binding.
[[nodiscard]] uint64_t stable_int64_vector_hash(
    const std::vector<std::size_t> &values) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  stable_mix(hash, 4ULL); // at::ScalarType::Long
  stable_mix(hash, 1ULL);
  stable_mix(hash, static_cast<uint64_t>(values.size()));
  for (const std::size_t source : values) {
    const int64_t value = static_cast<int64_t>(source);
    stable_mix_native_bytes(hash, value);
  }
  return hash;
}

[[nodiscard]] uint64_t stable_double_matrix_hash(const Matrix &matrix) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  stable_mix(hash, 7ULL); // at::ScalarType::Double
  stable_mix(hash, 2ULL);
  stable_mix(hash, static_cast<uint64_t>(matrix.rows));
  stable_mix(hash, static_cast<uint64_t>(matrix.columns));
  for (const double value : matrix.values) {
    stable_mix_native_bytes(hash, value);
  }
  return hash;
}

[[nodiscard]] Matrix csv_matrix(Records &records, const std::string &key,
                                std::size_t rows, std::size_t columns) {
  return {.rows = rows,
          .columns = columns,
          .values = parse_csv(records, key, rows * columns)};
}

struct Score {
  std::array<double, kTargets> task{};
  std::array<double, 4> family{};
  double macro{0.0};
};

struct TargetStats {
  std::array<double, kTargets> mean{};
  std::array<double, kTargets> sst{};
};

[[nodiscard]] TargetStats target_stats(
    const Matrix &target, const std::vector<std::size_t> *indices = nullptr) {
  TargetStats result{};
  const std::size_t rows = indices == nullptr ? target.rows : indices->size();
  for (std::size_t draw = 0; draw < rows; ++draw) {
    const std::size_t row = indices == nullptr ? draw : (*indices)[draw];
    for (std::size_t task = 0; task < kTargets; ++task) {
      result.mean[task] += target(row, task);
    }
  }
  for (double &value : result.mean) {
    value /= static_cast<double>(rows);
  }
  for (std::size_t draw = 0; draw < rows; ++draw) {
    const std::size_t row = indices == nullptr ? draw : (*indices)[draw];
    for (std::size_t task = 0; task < kTargets; ++task) {
      const double delta = target(row, task) - result.mean[task];
      result.sst[task] += delta * delta;
    }
  }
  return result;
}

[[nodiscard]] Score score(const Matrix &prediction, const Matrix &target,
                          const std::vector<std::size_t> *indices = nullptr,
                          const TargetStats *provided_stats = nullptr) {
  if (prediction.rows != target.rows || prediction.columns != kTargets ||
      target.columns != kTargets) {
    throw std::runtime_error("continuous score shape mismatch");
  }
  const std::size_t rows = indices == nullptr ? target.rows : indices->size();
  const TargetStats local =
      provided_stats == nullptr ? target_stats(target, indices) : TargetStats{};
  const TargetStats &stats = provided_stats == nullptr ? local : *provided_stats;
  std::array<double, kTargets> sse{};
  for (std::size_t draw = 0; draw < rows; ++draw) {
    const std::size_t row = indices == nullptr ? draw : (*indices)[draw];
    for (std::size_t task = 0; task < kTargets; ++task) {
      const double delta = prediction(row, task) - target(row, task);
      sse[task] += delta * delta;
    }
  }
  Score result{};
  std::array<int, 4> family_counts{};
  for (std::size_t task = 0; task < kTargets; ++task) {
    result.task[task] =
        1.0 - sse[task] / std::max(1.0e-12, stats.sst[task]);
    const std::size_t family =
        static_cast<std::size_t>(kTargetFamily[task]);
    result.family[family] += result.task[task];
    ++family_counts[family];
  }
  for (std::size_t family = 0; family < result.family.size(); ++family) {
    result.family[family] /= static_cast<double>(family_counts[family]);
    result.macro += result.family[family];
  }
  result.macro /= static_cast<double>(result.family.size());
  return result;
}

struct ContinuousCurve {
  std::array<Matrix, 4> prediction{};
  std::array<Score, 4> point{};
  std::array<double, 4> family_area{};
  double area{0.0};
};

struct OrderCurve {
  std::array<Matrix, 4> prediction{};
  std::array<double, 4> accuracy{};
  double area{0.0};
};

using ContinuousGrid =
    std::array<std::array<std::array<ContinuousCurve, 2>, 4>, 3>;
using OrderGrid = std::array<std::array<std::array<OrderCurve, 2>, 4>, 3>;

[[nodiscard]] bool known_alpha(double value) {
  return std::find(kRidgeGrid.begin(), kRidgeGrid.end(), value) !=
         kRidgeGrid.end();
}

[[nodiscard]] ContinuousCurve read_continuous_curve(
    Records &records, const std::string &prefix, const Matrix &target) {
  ContinuousCurve result{};
  for (std::size_t point = 0; point < kSamples.size(); ++point) {
    const std::string item =
        prefix + ".n_" + std::to_string(kSamples[point]);
    result.prediction[point] =
        csv_matrix(records, item + ".prediction_csv", kRows, kTargets);
    result.point[point] = score(result.prediction[point], target);
    expect_close(records, item + ".macro_r2", result.point[point].macro);
    for (std::size_t family = 0; family < kFamilies.size(); ++family) {
      expect_close(records,
                   item + ".family_" + std::string(kFamilies[family]) +
                       "_r2",
                   result.point[point].family[family]);
      result.family_area[family] += result.point[point].family[family];
    }
    for (std::size_t task = 0; task < kTargets; ++task) {
      const std::string alpha_key =
          item + ".target_" + std::to_string(task) + ".alpha";
      if (!known_alpha(number(records, alpha_key))) {
        fail(records, "unknown selected alpha: " + alpha_key);
      }
    }
    result.area += result.point[point].macro;
  }
  result.area /= static_cast<double>(kSamples.size());
  expect_close(records, prefix + ".area", result.area);
  for (std::size_t family = 0; family < kFamilies.size(); ++family) {
    result.family_area[family] /= static_cast<double>(kSamples.size());
    expect_close(records,
                 prefix + ".family_" + std::string(kFamilies[family]) +
                     "_aulc",
                 result.family_area[family]);
  }
  return result;
}

[[nodiscard]] double order_accuracy(const Matrix &prediction,
                                    const Matrix &target,
                                    const std::vector<std::size_t> *groups =
                                        nullptr) {
  if (prediction.rows != target.rows || prediction.columns != 1 ||
      target.columns != 1 || target.rows % 2 != 0) {
    throw std::runtime_error("order score shape mismatch");
  }
  std::size_t correct = 0;
  std::size_t rows = target.rows;
  if (groups == nullptr) {
    for (std::size_t row = 0; row < rows; ++row) {
      correct += (prediction(row, 0) >= 0.0) == (target(row, 0) >= 0.0);
    }
  } else {
    rows = 2 * groups->size();
    for (std::size_t group : *groups) {
      for (std::size_t member = 0; member < 2; ++member) {
        const std::size_t row = 2 * group + member;
        correct += (prediction(row, 0) >= 0.0) == (target(row, 0) >= 0.0);
      }
    }
  }
  return static_cast<double>(correct) / static_cast<double>(rows);
}

[[nodiscard]] OrderCurve read_order_curve(Records &records,
                                           const std::string &prefix,
                                           const Matrix &target) {
  OrderCurve result{};
  for (std::size_t point = 0; point < kSamples.size(); ++point) {
    const std::string item =
        prefix + ".n_" + std::to_string(kSamples[point]);
    result.prediction[point] =
        csv_matrix(records, item + ".prediction_csv", kOrderRows, 1);
    result.accuracy[point] =
        order_accuracy(result.prediction[point], target);
    expect_close(records, item + ".accuracy", result.accuracy[point]);
    if (!known_alpha(number(records, item + ".alpha"))) {
      fail(records, "unknown order selected alpha: " + item + ".alpha");
    }
    result.area += result.accuracy[point];
  }
  result.area /= static_cast<double>(kSamples.size());
  expect_close(records, prefix + ".accuracy_aulc", result.area);
  return result;
}

[[nodiscard]] uint64_t splitmix64(uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] std::vector<std::size_t> sattolo(std::size_t rows,
                                                uint64_t tag) {
  std::vector<std::size_t> result(rows);
  std::iota(result.begin(), result.end(), std::size_t{0});
  uint64_t state = splitmix64(tag ^ splitmix64(rows));
  for (std::size_t index = rows - 1; index > 0; --index) {
    state = splitmix64(state);
    const std::size_t selected = state % index;
    std::swap(result[index], result[selected]);
  }
  return result;
}

using BootstrapRows =
    std::array<std::array<std::size_t, kRows>, kBootstrapReplicates>;

[[nodiscard]] BootstrapRows bootstrap_rows() {
  BootstrapRows result{};
  for (std::size_t replicate = 0; replicate < result.size(); ++replicate) {
    uint64_t state =
        splitmix64(kBootstrapSeed ^ splitmix64(replicate));
    for (std::size_t draw = 0; draw < kRows; ++draw) {
      state = splitmix64(state);
      result[replicate][draw] = state % kRows;
    }
  }
  return result;
}

[[nodiscard]] uint64_t bootstrap_rows_hash(const BootstrapRows &rows) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  stable_mix(hash, static_cast<uint64_t>(rows.size()));
  for (const auto &row : rows) {
    const std::vector<std::size_t> values(row.begin(), row.end());
    stable_mix(hash, stable_int64_vector_hash(values));
  }
  return hash;
}

struct Interval {
  double low{0.0};
  double high{0.0};
};

[[nodiscard]] Interval percentile_interval(std::vector<double> values) {
  if (values.empty() ||
      !std::all_of(values.begin(), values.end(),
                   [](double value) { return std::isfinite(value); })) {
    throw std::runtime_error("non-finite bootstrap values");
  }
  std::sort(values.begin(), values.end());
  const auto quantile = [&](double probability) {
    const double position = probability * static_cast<double>(values.size() - 1);
    const std::size_t lower = static_cast<std::size_t>(std::floor(position));
    const std::size_t upper = static_cast<std::size_t>(std::ceil(position));
    const double fraction = position - static_cast<double>(lower);
    return values[lower] + fraction * (values[upper] - values[lower]);
  };
  return {.low = quantile(0.025), .high = quantile(0.975)};
}

[[nodiscard]] double resampled_continuous_area(
    const ContinuousCurve &curve, const Matrix &target,
    const std::vector<std::size_t> &rows, const TargetStats &stats) {
  double result = 0.0;
  for (const Matrix &prediction : curve.prediction) {
    result += score(prediction, target, &rows, &stats).macro;
  }
  return result / static_cast<double>(curve.prediction.size());
}

[[nodiscard]] double resampled_order_area(
    const OrderCurve &curve, const Matrix &target,
    const std::vector<std::size_t> &groups) {
  double result = 0.0;
  for (const Matrix &prediction : curve.prediction) {
    result += order_accuracy(prediction, target, &groups);
  }
  return result / static_cast<double>(curve.prediction.size());
}

using Replicates = std::array<double, kBootstrapReplicates>;
using BootstrapGrid =
    std::array<std::array<std::array<Replicates, 2>, 4>, 3>;

struct ArmSummary {
  double point{0.0};
  Interval interval{};
  std::array<double, 3> seed{};
  std::array<double, 4> family{};
};

struct OrderSummary {
  double point{0.0};
  Interval interval{};
  std::array<double, 3> seed{};
};

[[nodiscard]] ArmSummary arm_summary(const ContinuousGrid &curves,
                                     const BootstrapGrid &boot,
                                     std::size_t arm, std::size_t mode) {
  ArmSummary result{};
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    result.seed[seed] = curves[seed][arm][mode].area;
    result.point += result.seed[seed];
    for (std::size_t family = 0; family < kFamilies.size(); ++family) {
      result.family[family] += curves[seed][arm][mode].family_area[family];
    }
  }
  result.point /= static_cast<double>(kSeeds.size());
  for (double &value : result.family) {
    value /= static_cast<double>(kSeeds.size());
  }
  std::vector<double> replicates(kBootstrapReplicates, 0.0);
  for (std::size_t replicate = 0; replicate < replicates.size(); ++replicate) {
    for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
      replicates[replicate] += boot[seed][arm][mode][replicate];
    }
    replicates[replicate] /= static_cast<double>(kSeeds.size());
  }
  result.interval = percentile_interval(std::move(replicates));
  return result;
}

[[nodiscard]] OrderSummary order_summary(const OrderGrid &curves,
                                         const BootstrapGrid &boot,
                                         std::size_t arm,
                                         std::size_t mode) {
  OrderSummary result{};
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    result.seed[seed] = curves[seed][arm][mode].area;
    result.point += result.seed[seed];
  }
  result.point /= static_cast<double>(kSeeds.size());
  std::vector<double> replicates(kBootstrapReplicates, 0.0);
  for (std::size_t replicate = 0; replicate < replicates.size(); ++replicate) {
    for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
      replicates[replicate] += boot[seed][arm][mode][replicate];
    }
    replicates[replicate] /= static_cast<double>(kSeeds.size());
  }
  result.interval = percentile_interval(std::move(replicates));
  return result;
}

[[nodiscard]] psm_gate::ContinuousInput contrast(
    const ContinuousGrid &curves, const BootstrapGrid &boot,
    std::size_t downstream, std::size_t upstream) {
  psm_gate::ContinuousInput result{};
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    result.seed_deltas[seed] =
        curves[seed][downstream][0].area - curves[seed][upstream][0].area;
    result.point += result.seed_deltas[seed];
    for (std::size_t family = 0; family < kFamilies.size(); ++family) {
      result.family_deltas[family] +=
          curves[seed][downstream][0].family_area[family] -
          curves[seed][upstream][0].family_area[family];
    }
  }
  result.point /= static_cast<double>(kSeeds.size());
  for (double &value : result.family_deltas) {
    value /= static_cast<double>(kSeeds.size());
  }
  std::vector<double> replicates(kBootstrapReplicates, 0.0);
  for (std::size_t replicate = 0; replicate < replicates.size(); ++replicate) {
    for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
      replicates[replicate] += boot[seed][downstream][0][replicate] -
                               boot[seed][upstream][0][replicate];
    }
    replicates[replicate] /= static_cast<double>(kSeeds.size());
  }
  const Interval interval = percentile_interval(std::move(replicates));
  result.low = interval.low;
  result.high = interval.high;
  return result;
}

[[nodiscard]] std::string continuous_classification(
    const psm_gate::ContinuousInput &input) {
  if (!std::isfinite(input.point) || !std::isfinite(input.low) ||
      !std::isfinite(input.high) || input.low > input.high ||
      !std::all_of(input.seed_deltas.begin(), input.seed_deltas.end(),
                   [](double value) { return std::isfinite(value); }) ||
      !std::all_of(input.family_deltas.begin(), input.family_deltas.end(),
                   [](double value) { return std::isfinite(value); })) {
    return "invalid_numeric";
  }
  const int positive = static_cast<int>(std::count_if(
      input.seed_deltas.begin(), input.seed_deltas.end(),
      [](double value) { return value > 0.0; }));
  const int noninferior = static_cast<int>(std::count_if(
      input.seed_deltas.begin(), input.seed_deltas.end(),
      [](double value) { return value > -0.02; }));
  const bool families = std::all_of(
      input.family_deltas.begin(), input.family_deltas.end(),
      [](double value) { return value > -0.05; });
  if (input.point >= 0.02 && input.low > 0.0 && positive >= 2) {
    return "material_gain";
  }
  if (input.low > -0.02 && noninferior >= 2 && families) {
    return "noninferior";
  }
  return "unresolved";
}

[[nodiscard]] std::string order_classification(
    const OrderSummary &input) {
  if (!std::isfinite(input.point) || !std::isfinite(input.interval.low) ||
      !std::isfinite(input.interval.high) ||
      input.interval.low > input.interval.high ||
      !std::all_of(input.seed.begin(), input.seed.end(),
                   [](double value) { return std::isfinite(value); })) {
    return "invalid_numeric";
  }
  const int above = static_cast<int>(std::count_if(
      input.seed.begin(), input.seed.end(),
      [](double value) { return value > 0.50; }));
  if (input.point >= 0.60 && input.interval.low > 0.50 && above >= 2) {
    return "order_decodable";
  }
  if (input.interval.high <= 0.55) {
    return "chance_consistent";
  }
  return "order_unresolved";
}

void verify_continuous_summary(Records &records, const std::string &prefix,
                               const ArmSummary &summary,
                               bool shuffled) {
  const std::string item =
      prefix + (shuffled ? ".shuffled_aulc" : ".aulc");
  expect_close(records, item + ".point", summary.point);
  expect_close(records, item + ".bootstrap_95_low", summary.interval.low);
  expect_close(records, item + ".bootstrap_95_high", summary.interval.high);
  if (!shuffled) {
    for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
      expect_close(records, item + ".seed_" + std::string(kSeeds[seed]),
                   summary.seed[seed]);
    }
    for (std::size_t family = 0; family < kFamilies.size(); ++family) {
      expect_close(records,
                   prefix + ".family_" + std::string(kFamilies[family]) +
                       "_aulc",
                   summary.family[family]);
    }
  }
}

void verify_order_summary(Records &records, const std::string &prefix,
                          const OrderSummary &summary) {
  expect_close(records, prefix + ".point", summary.point);
  expect_close(records, prefix + ".bootstrap_95_low", summary.interval.low);
  expect_close(records, prefix + ".bootstrap_95_high", summary.interval.high);
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    expect_close(records, prefix + ".seed_" + std::string(kSeeds[seed]),
                 summary.seed[seed]);
  }
  expect_text(records, prefix + ".classification",
              order_classification(summary));
}

void verify_contrast(Records &records, const std::string &prefix,
                     const psm_gate::ContinuousInput &input) {
  expect_close(records, prefix + ".point", input.point);
  expect_close(records, prefix + ".bootstrap_95_low", input.low);
  expect_close(records, prefix + ".bootstrap_95_high", input.high);
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    expect_close(records,
                 prefix + ".seed_" + std::string(kSeeds[seed]) + "_delta",
                 input.seed_deltas[seed]);
  }
  for (std::size_t family = 0; family < kFamilies.size(); ++family) {
    expect_close(records,
                 prefix + ".family_" + std::string(kFamilies[family]) +
                     "_delta",
                 input.family_deltas[family]);
  }
  expect_text(records, prefix + ".classification",
              continuous_classification(input));
}

[[nodiscard]] std::string seed_arm_prefix(std::string_view seed,
                                          std::string_view arm) {
  return "srr.seed_" + std::string(seed) + ".arm." + std::string(arm);
}

[[nodiscard]] std::string psm_seed_arm_prefix(std::string_view seed,
                                              std::string_view arm) {
  return "psm.seed_" + std::string(seed) + ".arm." + std::string(arm);
}

void compare_reference_curve(Records &srr, Records &parent,
                             std::string_view seed, std::string_view srr_arm,
                             std::string_view psm_arm) {
  const std::string left = seed_arm_prefix(seed, srr_arm);
  const std::string right = psm_seed_arm_prefix(seed, psm_arm);
  for (const std::string_view mode : {"probe", "shuffled_probe"}) {
    const std::string lp = left + "." + std::string(mode);
    const std::string rp = right + "." + std::string(mode);
    compare_number(srr, parent, lp + ".area", rp + ".area");
    for (const std::string_view family : kFamilies) {
      compare_number(srr, parent,
                     lp + ".family_" + std::string(family) + "_aulc",
                     rp + ".family_" + std::string(family) + "_aulc");
    }
    for (int samples : kSamples) {
      const std::string li = lp + ".n_" + std::to_string(samples);
      const std::string ri = rp + ".n_" + std::to_string(samples);
      compare_number(srr, parent, li + ".macro_r2", ri + ".macro_r2");
      for (const std::string_view family : kFamilies) {
        compare_number(srr, parent,
                       li + ".family_" + std::string(family) + "_r2",
                       ri + ".family_" + std::string(family) + "_r2");
      }
      for (std::size_t task = 0; task < kTargets; ++task) {
        compare_text(srr, parent,
                     li + ".target_" + std::to_string(task) + ".alpha",
                     ri + ".target_" + std::to_string(task) + ".alpha");
      }
    }
  }
  for (const std::string_view mode : {"order_probe", "order_shuffled_probe"}) {
    const std::string lp = left + "." + std::string(mode);
    const std::string rp = right + "." + std::string(mode);
    compare_number(srr, parent, lp + ".accuracy_aulc",
                   rp + ".accuracy_aulc");
    for (int samples : kSamples) {
      const std::string li = lp + ".n_" + std::to_string(samples);
      const std::string ri = rp + ".n_" + std::to_string(samples);
      compare_number(srr, parent, li + ".accuracy", ri + ".accuracy");
      compare_text(srr, parent, li + ".alpha", ri + ".alpha");
    }
  }
}

void compare_arm_summary(Records &srr, Records &parent,
                         std::string_view srr_arm,
                         std::string_view psm_arm) {
  const std::string left = "srr.summary.arm." + std::string(srr_arm);
  const std::string right = "psm.summary.arm." + std::string(psm_arm);
  for (const std::string_view scalar :
       {"aulc.point", "aulc.bootstrap_95_low", "aulc.bootstrap_95_high",
        "shuffled_aulc.point", "shuffled_aulc.bootstrap_95_low",
        "shuffled_aulc.bootstrap_95_high", "order.point",
        "order.bootstrap_95_low", "order.bootstrap_95_high",
        "order_shuffled.point", "order_shuffled.bootstrap_95_low",
        "order_shuffled.bootstrap_95_high"}) {
    compare_number(srr, parent, left + "." + std::string(scalar),
                   right + "." + std::string(scalar));
  }
  for (const std::string_view seed : kSeeds) {
    for (const std::string_view item :
         {"aulc.seed_", "order.seed_", "order_shuffled.seed_"}) {
      compare_number(srr, parent,
                     left + "." + std::string(item) + std::string(seed),
                     right + "." + std::string(item) + std::string(seed));
    }
  }
  for (const std::string_view family : kFamilies) {
    compare_number(srr, parent,
                   left + ".family_" + std::string(family) + "_aulc",
                   right + ".family_" + std::string(family) + "_aulc");
  }
  for (const std::string_view item :
       {"continuous_shuffle_pass", "order_shuffle_pass",
        "order.classification", "order_shuffled.classification"}) {
    compare_text(srr, parent, left + "." + std::string(item),
                 right + "." + std::string(item));
  }
}

void compare_contrast(Records &srr, Records &parent,
                      const std::string &left, const std::string &right) {
  for (const std::string_view item :
       {"point", "bootstrap_95_low", "bootstrap_95_high"}) {
    compare_number(srr, parent, left + "." + std::string(item),
                   right + "." + std::string(item));
  }
  for (const std::string_view seed : kSeeds) {
    compare_number(srr, parent,
                   left + ".seed_" + std::string(seed) + "_delta",
                   right + ".seed_" + std::string(seed) + "_delta");
  }
  for (const std::string_view family : kFamilies) {
    compare_number(srr, parent,
                   left + ".family_" + std::string(family) + "_delta",
                   right + ".family_" + std::string(family) + "_delta");
  }
  compare_text(srr, parent, left + ".classification",
               right + ".classification");
}

[[nodiscard]] bool verify_preflight_log(
    Records &audit_records, const std::filesystem::path &repository_root) {
  const auto path = repository_root /
                    ".build/tests/representation_srr_v1_preflight.log";
  Records preflight{};
  try {
    preflight = parse_records(read_bytes(path), /*reject_malformed=*/true);
  } catch (const std::exception &error) {
    fail(audit_records, std::string("cannot verify SRR preflight log: ") +
                            error.what());
    return false;
  }
  expect_text(preflight, "schema",
              "wikimyei.mtf_jepa_mae_vicreg.srr_preflight.v1");
  expect_bool(preflight, "srr.preflight.scientific_rows_used", false);
  expect_bool(preflight, "srr.preflight.target_constructed", false);
  expect_bool(preflight, "optimizer_constructed", false);
  expect_close(preflight, "optimizer_steps", 0.0, 0.0);
  expect_close(preflight, "backward_calls", 0.0, 0.0);
  expect_close(preflight, "scientific_probe_fits", 0.0, 0.0);
  expect_text(preflight, "srr.preflight.environment.device", "cuda:0");
  expect_text(preflight, "srr.preflight.environment.dtype", "float32");
  expect_close(preflight, "srr.preflight.environment.cpu_threads", 1.0,
               0.0);
  expect_close(preflight, "srr.preflight.environment.cpu_interop_threads",
               1.0, 0.0);
  for (const std::string_view key :
       {"srr.preflight.environment.deterministic_algorithms",
        "srr.preflight.environment.deterministic_cudnn",
        "srr.preflight.environment.tf32_cublas_disabled",
        "srr.preflight.environment.tf32_cudnn_disabled",
        "srr.preflight.environment.cublas_workspace_exact",
        "srr.preflight.environment.pass",
        "srr.preflight.tokenizer_plan_pass",
        "srr.preflight.token_layout_pass",
        "srr.preflight.partition_pass",
        "srr.preflight.projection_pass",
        "srr.preflight.public_sandwich_exact",
        "srr.preflight.direct_encoder_exact",
        "srr.preflight.shadow_input_unchanged",
        "srr.preflight.audit_contract_exact",
        "srr.preflight.device_contract_exact",
        "srr.preflight.repeated_device_exact",
        "srr.preflight.repeated_capture_exact",
        "srr.preflight.offline_bytes_exact",
        "srr.preflight.parameters_exact", "srr.preflight.generator_exact",
        "srr.preflight.model_mode_exact",
        "srr.preflight.permutations_valid",
        "srr.preflight.order_shuffle_balanced",
        "srr.preflight.bootstrap_valid", "srr.preflight.pass"}) {
    expect_bool(preflight, std::string(key), true);
  }
  expect_bool(preflight, "srr.preflight.environment.deterministic_warn_only",
              false);
  expect_bool(preflight, "srr.preflight.ridge_fixture_executed", false);
  expect_text(preflight, "srr.preflight.token_layout_hash",
              "741bf0dc9bc7fe3f");
  expect_text(preflight, "srr.preflight.projection_hash",
              "ac8a43fd65b2c8a8");
  expect_text(preflight, "srr.preflight.bootstrap_table_hash",
              "408205cac33d403d");
  for (const auto &permutation : kExpectedPermutations) {
    const std::string prefix =
        "srr.preflight.permutation." + std::string(permutation.name);
    expect_close(preflight, prefix + ".rows",
                 static_cast<double>(permutation.rows), 0.0);
    expect_text(preflight, prefix + ".hash", permutation.hash);
    expect_bool(preflight, prefix + ".pass", true);
  }
  const double offline =
      number(preflight, "srr.preflight.offline_equivalence_max_abs");
  const double device =
      number(preflight, "srr.preflight.device_translation_max_abs");
  const double relative_l2 =
      number(preflight, "srr.preflight.device_relative_l2");
  if (offline < 0.0 || offline > srr_gate::kOfflineEquivalenceTolerance) {
    fail(preflight, "preflight offline equivalence threshold failed");
  }
  if (device < 0.0 || device > srr_gate::kDeviceTranslationTolerance) {
    fail(preflight, "preflight device translation threshold failed");
  }
  if (relative_l2 < 0.0) {
    fail(preflight, "preflight device relative L2 is negative");
  }
  const std::string &parameter_hash_before =
      required(preflight, "srr.preflight.parameter_hash_before");
  const std::string &parameter_hash_after =
      required(preflight, "srr.preflight.parameter_hash_after");
  if (parameter_hash_before.size() != 16 ||
      parameter_hash_before != parameter_hash_after ||
      !std::all_of(parameter_hash_before.begin(), parameter_hash_before.end(),
                   [](char value) {
                     return (value >= '0' && value <= '9') ||
                            (value >= 'a' && value <= 'f');
                   })) {
    fail(preflight, "preflight parameter hash before/after mismatch");
  }
  expect_text(
      preflight, "srr.preflight.authoritative_command",
      "CUBLAS_WORKSPACE_CONFIG=:4096:8 .build/tests/quality_wikimyei_mtf_"
      "jepa_mae_vicreg_structured_readout_repair --experiment structured-"
      "readout-repair --device cuda");
  for (const std::string_view key :
       {"training_authorized", "augmentation_change_authorized",
        "long_run_authorized", "production_or_end_to_end_authorized",
        "follow_on_production_repair_authorized"}) {
    expect_bool(preflight, std::string(key), false);
  }
  for (const auto &[key, value] : preflight.values) {
    (void)value;
    if (preflight.accessed_keys.count(key) == 0) {
      fail(preflight, "unrecognized preflight machine key: " + key);
    }
  }
  if (preflight.duplicate_key_count != 0 ||
      preflight.nonfinite_value_count != 0 ||
      preflight.malformed_line_count != 0 ||
      preflight.recognized_runtime_noise_count != 2) {
    fail(preflight, "preflight machine-key uniqueness/finite contract failed");
  }
  for (const std::string &error : preflight.errors) {
    fail(audit_records, "preflight: " + error);
  }
  return preflight.errors.empty();
}

[[nodiscard]] ManifestEvidence verify_manifest(
    Records &audit_records, const std::filesystem::path &repository_root,
    std::string_view auditor_binary) {
  ManifestEvidence result{};
  const std::size_t errors_before = audit_records.errors.size();
  const std::string logged_manifest_bytes =
      required(audit_records, "srr.prerun_manifest.bytes");
  const std::string logged_manifest_sha =
      required(audit_records, "srr.prerun_manifest.sha256");
  constexpr std::string_view kFrozenProtocolSha =
      "ad7c9381d58a23e8f3cec27b59b44e6532aa561227ad22d57578cc6ba0a04946";
  constexpr std::string_view kBase =
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/";
  const std::set<std::string> expected_paths{
      ".build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_structured_"
      "readout_repair",
      ".build/tests/representation_psm_v1_audit.log",
      ".build/tests/representation_psm_v1_authoritative.log",
      ".build/tests/representation_psm_v1_prerun.sha256",
      ".build/tests/representation_psm_v1_receipt.sha256",
      ".build/tests/representation_srr_v1_mechanics.log",
      ".build/tests/representation_srr_v1_preflight.log",
      ".build/tests/test_structured_readout_repair_gate",
      ".build/tests/test_structured_readout_repair_log_auditor",
      ".build/tests/test_structured_readout_shadow",
      "src/include/jkimyei/training/representation/"
      "mtf_jepa_mae_vicreg_graph_first_launcher.h",
      "src/include/piaabo/digest/sha256.h",
      "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
      "mtf_jepa_mae_vicreg.h",
      std::string(kBase) + "Makefile",
      std::string(kBase) + "POOLING_STRUCTURE_MECHANISM_MAP_FINDINGS.md",
      std::string(kBase) + "POOLING_STRUCTURE_MECHANISM_MAP_PLAN.md",
      std::string(kBase) + "POOLING_STRUCTURE_MECHANISM_MAP_PROTOCOL.md",
      std::string(kBase) + "POOLING_STRUCTURE_MECHANISM_MAP_PROTOCOL.sha256",
      std::string(kBase) + "STRUCTURED_READOUT_REPAIR_PLAN.md",
      std::string(kBase) + "STRUCTURED_READOUT_REPAIR_PROTOCOL.md",
      std::string(kBase) + "STRUCTURED_READOUT_REPAIR_PROTOCOL.sha256",
      std::string(kBase) + "pooling_structure_mechanism_map_gate.h",
      std::string(kBase) +
          "quality_wikimyei_mtf_jepa_mae_vicreg_representation.cpp",
      std::string(kBase) +
          "quality_wikimyei_mtf_jepa_mae_vicreg_structured_readout_repair.cpp",
      std::string(kBase) + "structured_readout_repair_gate.h",
      std::string(kBase) + "structured_readout_shadow.h",
      std::string(kBase) + "test_structured_readout_repair_gate.cpp",
      std::string(kBase) + "test_structured_readout_repair_log_auditor.cpp",
      std::string(kBase) + "test_structured_readout_shadow.cpp"};
  const auto manifest_path = repository_root /
                             ".build/tests/representation_srr_v1_prerun.sha256";
  std::string raw;
  try {
    raw = read_bytes(manifest_path);
  } catch (const std::exception &error) {
    fail(audit_records, std::string("cannot verify SRR prerun manifest: ") +
                            error.what());
    result.error_count = audit_records.errors.size() - errors_before;
    return result;
  }
  result.bytes = raw.size();
  result.sha256 = sha256(raw);
  result.auditor_binary_bytes = auditor_binary.size();
  result.auditor_binary_sha256 = sha256(auditor_binary);

  std::filesystem::path canonical_root;
  try {
    canonical_root = std::filesystem::canonical(repository_root);
    result.canonical_containment = true;
  } catch (const std::exception &error) {
    fail(audit_records, std::string("cannot canonicalize repository root: ") +
                            error.what());
  }

  std::map<std::string, std::string> metadata;
  std::map<std::string, std::string> listed_digests;
  std::set<std::string> listed_paths;
  std::size_t begin = 0;
  while (begin < raw.size()) {
    const std::size_t newline = raw.find('\n', begin);
    const std::size_t end = newline == std::string::npos ? raw.size() : newline;
    std::string line = raw.substr(begin, end - begin);
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.rfind("# ", 0) == 0) {
      const std::size_t separator = line.find('=', 2);
      if (separator == std::string::npos || separator == 2) {
        fail(audit_records, "malformed SRR manifest metadata line");
      } else {
        const std::string key = line.substr(2, separator - 2);
        if (!metadata.emplace(key, line.substr(separator + 1)).second) {
          fail(audit_records, "duplicate SRR manifest metadata: " + key);
        }
      }
    } else if (!line.empty()) {
      if (line.size() < 67 || line[64] != ' ' || line[65] != ' ') {
        fail(audit_records, "malformed SRR manifest hash line");
      } else {
        const std::string digest = line.substr(0, 64);
        const std::string relative = line.substr(66);
        const bool digest_valid = digest.size() == 64 &&
            std::all_of(digest.begin(), digest.end(), [](char value) {
              return (value >= '0' && value <= '9') ||
                     (value >= 'a' && value <= 'f');
            });
        const std::filesystem::path relative_path(relative);
        const bool lexical_exact =
            !relative.empty() && relative.find('\\') == std::string::npos &&
            !relative_path.is_absolute() &&
            relative_path.generic_string() == relative &&
            relative_path.lexically_normal() == relative_path;
        if (!digest_valid || !lexical_exact ||
            expected_paths.count(relative) != 1 ||
            !listed_paths.insert(relative).second) {
          fail(audit_records, "invalid, unexpected, or duplicate SRR "
                              "manifest entry: " + relative);
        } else {
          ++result.entry_count;
          listed_digests.emplace(relative, digest);
          try {
            const auto canonical_target =
                std::filesystem::canonical(repository_root / relative_path);
            const auto mismatch = std::mismatch(
                canonical_root.begin(), canonical_root.end(),
                canonical_target.begin(), canonical_target.end());
            const bool contained = mismatch.first == canonical_root.end();
            if (!contained ||
                !std::filesystem::is_regular_file(canonical_target)) {
              result.canonical_containment = false;
              fail(audit_records, "SRR manifest target escapes repository or "
                                  "is not a regular file: " + relative);
            } else if (sha256(read_bytes(canonical_target)) != digest) {
              fail(audit_records, "SRR manifest hash mismatch: " + relative);
            }
          } catch (const std::exception &error) {
            result.canonical_containment = false;
            fail(audit_records, "SRR manifest entry unreadable: " + relative +
                                    " error=" + error.what());
          }
        }
      }
    }
    if (newline == std::string::npos) {
      break;
    }
    begin = newline + 1;
  }
  result.exact_path_set = listed_paths == expected_paths &&
                          result.entry_count == expected_paths.size();
  if (!result.exact_path_set) {
    fail(audit_records, "SRR manifest path set is not the exact frozen set");
  }

  const auto require_metadata = [&](std::string_view key,
                                    std::string_view expected) {
    const auto found = metadata.find(std::string(key));
    if (found == metadata.end() || found->second != expected) {
      fail(audit_records, "SRR manifest metadata mismatch: " +
                              std::string(key));
    }
  };
  require_metadata("schema",
                   "wikimyei.mtf_jepa_mae_vicreg.srr_prerun_manifest.v1");
  require_metadata("protocol_sha256", kFrozenProtocolSha);
  require_metadata("scientific_attempt_limit", "1");
  require_metadata("preflight_pass", "true");
  require_metadata("scientific_rows_used_before_seal", "false");
  require_metadata("projection_rssm_hash", "f8c9f35282de2ee0");
  require_metadata("projection_psm_hash", "ac8a43fd65b2c8a8");
  require_metadata("token_layout_hash", "741bf0dc9bc7fe3f");
  require_metadata("bootstrap_table_hash", "408205cac33d403d");
  require_metadata("environment_device", "cuda:0");
  require_metadata("environment_dtype", "float32");
  require_metadata("environment_cpu_threads", "1");
  require_metadata("environment_cpu_interop_threads", "1");
  require_metadata("environment_deterministic_algorithms", "true");
  require_metadata("environment_deterministic_warn_only", "false");
  require_metadata("environment_deterministic_cudnn", "true");
  require_metadata("environment_tf32_cublas_disabled", "true");
  require_metadata("environment_tf32_cudnn_disabled", "true");
  require_metadata("environment_cublas_workspace", ":4096:8");
  for (const auto &permutation : kExpectedPermutations) {
    require_metadata("permutation_" + std::string(permutation.name) +
                         "_rows",
                     std::to_string(permutation.rows));
    require_metadata("permutation_" + std::string(permutation.name) +
                         "_hash",
                     permutation.hash);
  }
  require_metadata(
      "expected_channel_feature_hashes",
      "fe6e0bbf68d0e2bb,8b927bbea2a05e18,24e78b9508ca9ad5,"
      "08d4a488856bfbfa,44009c8af1d66aad,3e3679b5fdaad6f4,"
      "62784cdaa6f2732e,f0b5c7c7af09fff5,ea9be90953eedf4a,"
      "46a4d684c810882f,f3e44c0c317ff150,e4fd7eb5e2c72f03,"
      "424c78606a71382d,f82f08eaec0c6213,37a741a2fbc7e4df,"
      "45d385b28a1e4166,d1892879d98f20c3,602292ee53e190fb");
  require_metadata(
      "expected_cdsb_feature_hashes",
      "fd676633045bf4eb,f5b1f42cff608cf8,91a23982f8cad89e,"
      "bcd5d393929ff872,a29adca524fa637e,49302fd5dca87d0f,"
      "ee2ebca7b09e54df,7e63ad5dd9a2f37f,624cdc1ea7ef3613,"
      "613ca3e7e2c85d9a,2ea912456e086a43,4b11e03466aff5e7,"
      "5faebeb61db5bc58,7bd13bbd5cce225c,991a19ec290ac589,"
      "16a8bebec4dc971a,71ba6bd2a496c0e2,613fec93300fbfaa");
  require_metadata(
      "expected_encoder_feature_hashes",
      "5b7c5c6867b5ff74,e02960bf326459c0,82a99b7c3efc300a,"
      "38960d1504b0326c,bd6a4dfc66c366ee,b9f97e5840ed2695,"
      "073f00492d9e8fb5,6cf977e339103618,35590246ab888f96,"
      "8368c27e52f3d040,902bf5f0d4f7c47d,3c7396f398ba01f9,"
      "5a12b15978382cdc,73060fa5b72f84b8,04a7a048ff5eaef2,"
      "50c222b643466356,5a774b3f3046eb6d,b54ad14f242f294d");
  require_metadata(
      "authoritative_command",
      "CUBLAS_WORKSPACE_CONFIG=:4096:8 .build/tests/quality_wikimyei_mtf_"
      "jepa_mae_vicreg_structured_readout_repair --experiment structured-"
      "readout-repair --device cuda");

  const std::string protocol_relative =
      std::string(kBase) + "STRUCTURED_READOUT_REPAIR_PROTOCOL.md";
  const std::string sidecar_relative =
      std::string(kBase) + "STRUCTURED_READOUT_REPAIR_PROTOCOL.sha256";
  try {
    const std::string protocol_bytes =
        read_bytes(repository_root / protocol_relative);
    const std::string sidecar_bytes =
        read_bytes(repository_root / sidecar_relative);
    const std::string expected_sidecar =
        std::string(kFrozenProtocolSha) +
        "  STRUCTURED_READOUT_REPAIR_PROTOCOL.md\n";
    result.protocol_digest_exact =
        sha256(protocol_bytes) == kFrozenProtocolSha &&
        listed_digests[protocol_relative] == kFrozenProtocolSha &&
        metadata["protocol_sha256"] == kFrozenProtocolSha &&
        sidecar_bytes == expected_sidecar;
  } catch (const std::exception &) {
    result.protocol_digest_exact = false;
  }
  if (!result.protocol_digest_exact) {
    fail(audit_records, "SRR protocol digest/sidecar cross-check failed");
  }

  const std::string auditor_relative =
      ".build/tests/test_structured_readout_repair_log_auditor";
  result.auditor_binary_exact =
      listed_digests[auditor_relative] == result.auditor_binary_sha256;
  if (!result.auditor_binary_exact) {
    fail(audit_records, "executing SRR auditor is not the sealed binary");
  }
  result.runtime_binding_exact =
      logged_manifest_bytes == std::to_string(result.bytes) &&
      logged_manifest_sha == result.sha256;
  if (!result.runtime_binding_exact) {
    fail(audit_records, "authoritative log is not bound to this pre-run "
                        "manifest");
  }

  const auto mechanics_path = repository_root /
                              ".build/tests/representation_srr_v1_mechanics.log";
  try {
    std::string mechanics = read_bytes(mechanics_path);
    std::vector<std::string> lines;
    std::size_t line_begin = 0;
    while (line_begin < mechanics.size()) {
      const std::size_t newline = mechanics.find('\n', line_begin);
      const std::size_t line_end =
          newline == std::string::npos ? mechanics.size() : newline;
      std::string line = mechanics.substr(line_begin, line_end - line_begin);
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      if (!line.empty()) {
        lines.push_back(std::move(line));
      }
      if (newline == std::string::npos) {
        break;
      }
      line_begin = newline + 1;
    }
    const std::vector<std::string> expected_lines{
        "SRR-1 structured readout shadow tests passed",
        "cells=16 tokens_per_channel=24 output_shape=[B,3,32]",
        "training_or_augmentation_used=false",
        "structured_readout_repair_gate=PASS",
        "threshold_faces=material,noninferiority,order,tolerances",
        "precedence_stages=local,manifest,parent,offline,device,controls,quality",
        "srr_log_auditor_self_test=PASS"};
    result.mechanics_log_exact = lines == expected_lines;
  } catch (const std::exception &) {
    result.mechanics_log_exact = false;
  }
  if (!result.mechanics_log_exact) {
    fail(audit_records, "SRR mechanics log is not the exact seven-line log");
  }
  result.preflight_log_exact =
      verify_preflight_log(audit_records, repository_root);
  result.error_count = audit_records.errors.size() - errors_before;
  result.exact = result.error_count == 0 && result.exact_path_set &&
                 result.canonical_containment &&
                 result.protocol_digest_exact &&
                 result.runtime_binding_exact &&
                 result.auditor_binary_exact &&
                 result.mechanics_log_exact && result.preflight_log_exact;
  return result;
}

[[nodiscard]] ParentEvidence verify_parent(
    Records &parent, std::string_view raw,
    const std::filesystem::path &repository_root) {
  ParentEvidence evidence{};
  const std::size_t errors_before = parent.errors.size();
  evidence.log_exact = raw.size() == kParentBytes &&
                       sha256(raw) == kParentSha256;
  if (!evidence.log_exact) {
    fail(parent, "sealed parent authoritative log bytes/SHA-256 mismatch");
  }
  expect_text(parent, "schema", "wikimyei.mtf_jepa_mae_vicreg.psm.v1");
  expect_text(parent, "module_only", "true");
  expect_text(parent, "device", "cuda:0");
  expect_text(parent, "model_seeds", "17,31,47");
  expect_text(parent, "arms", "channel,channel_domain,channel_domain_scale,"
                              "channel_domain_scale_bin,encoder");
  expect_text(parent, "representation_width", "96");
  expect_bool(parent, "optimizer_constructed", false);
  expect_close(parent, "optimizer_steps", 0.0, 0.0);
  expect_close(parent, "backward_calls", 0.0, 0.0);
  expect_bool(parent, "launcher_augmentation", false);
  expect_bool(parent, "psm.attempt.consumed", true);
  expect_text(parent, "execution_status", "psm_measurements_complete");
  expect_text(parent, "psm.summary.gate.classification",
              "coarse_position_separation_sufficient");
  for (const std::string_view key :
       {"training_authorized", "long_run_authorized",
        "production_or_end_to_end_authorized", "follow_on_repair_authorized"}) {
    expect_bool(parent, std::string(key), false);
  }
  for (const std::string_view key :
       {"psm.summary.validity.numeric_inputs",
        "psm.summary.validity.capture_and_identity_exact",
        "psm.summary.validity.parameters_and_rng_unchanged",
        "psm.summary.validity.partitions_valid",
        "psm.summary.validity.projection_valid",
        "psm.summary.validity.deterministic_tables_valid",
        "psm.summary.validity.references_reproduced",
        "psm.summary.validity.continuous_shuffle_pass",
        "psm.summary.validity.order_shuffle_pass",
        "psm.summary.validity.mechanics_valid",
        "psm.summary.gate.boundary_reproduced"}) {
    expect_bool(parent, std::string(key), true);
  }
  expect_text(parent, "psm.projection.rssm_hash", "f8c9f35282de2ee0");
  expect_text(parent, "psm.projection.psm_hash", "ac8a43fd65b2c8a8");
  expect_text(parent, "psm.token_layout.hash", "741bf0dc9bc7fe3f");
  expect_text(parent, "psm.bootstrap.table_hash", "408205cac33d403d");
  evidence.log_exact =
      evidence.log_exact && parent.errors.size() == errors_before;

  struct Artifact {
    const char *path;
    std::size_t bytes;
    const char *digest;
  };
  constexpr std::array<Artifact, 8> artifacts{{
      {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
       "vicreg/POOLING_STRUCTURE_MECHANISM_MAP_PLAN.md",
       15690,
       "c8fc41261bf3c901fb36213371a08f4e1b601aa288be15ad1d6af3d26e0ee648"},
      {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
       "vicreg/POOLING_STRUCTURE_MECHANISM_MAP_PROTOCOL.md",
       13541,
       "2f574310d79d581dbc39d4040d16431f8e067ae5d3583d6d4a5597b5a8ad72d3"},
      {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
       "vicreg/POOLING_STRUCTURE_MECHANISM_MAP_PROTOCOL.sha256",
       110,
       "9d4c5830cbea9fa6a8a3ccf5c832201cd0e5eb890069f8e2c7024bb14ae3fbc6"},
      {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
       "vicreg/POOLING_STRUCTURE_MECHANISM_MAP_FINDINGS.md",
       8275,
       "8a6a46da7625e3f25e3402d8037336bc0fe0a2de7a377827bc6e6e7f5894196f"},
      {".build/tests/representation_psm_v1_prerun.sha256", 4102,
       "22ea52b1c31916e0da57c436917076805d5482e49e38ae1bdea62cbce31418f2"},
      {".build/tests/representation_psm_v1_authoritative.log", 255304,
       "8243798d5af03d66257cbd1fd9da49a16ff7d6ba3f9e6bc54b5568dae41aa8b9"},
      {".build/tests/representation_psm_v1_audit.log", 1293,
       "f2aab5858536455e1bbd6b5ec0f6aa20e49b22d46abf0b94c6e9a7346a95f85a"},
      {".build/tests/representation_psm_v1_receipt.sha256", 2747,
       "31917fd5763f0f7831a212cd8eb0a685435e20ddb1071c5d6c189c75ee72cf53"},
  }};
  bool artifacts_exact = true;
  for (const Artifact &artifact : artifacts) {
    try {
      const std::string bytes = read_bytes(repository_root / artifact.path);
      const bool exact = bytes.size() == artifact.bytes &&
                         sha256(bytes) == artifact.digest;
      artifacts_exact = artifacts_exact && exact;
      if (!exact) {
        fail(parent, std::string("sealed parent artifact mismatch: ") +
                         artifact.path);
      }
      ++evidence.artifact_count;
    } catch (const std::exception &error) {
      artifacts_exact = false;
      fail(parent, std::string("sealed parent artifact unreadable: ") +
                       artifact.path + " error=" + error.what());
    }
  }
  // The CLI parent log is itself gate evidence, not merely a diagnostic.
  // Conjoin its literal byte/hash check with the canonical on-disk artifact
  // set so a mismatched argument cannot retain a successful parent gate.
  evidence.artifacts_exact = artifacts_exact && evidence.artifact_count == 8 &&
                             evidence.log_exact;
  evidence.classification_exact =
      required(parent, "psm.summary.gate.classification") ==
      "coarse_position_separation_sufficient";
  evidence.attempt_count_exact =
      count_exact_line(raw, "psm.attempt.consumed=true") == 1 &&
      count_exact_line(raw, "psm.attempt.consumed=false") == 0;
  if (!evidence.attempt_count_exact) {
    fail(parent, "parent authoritative attempt count is not exactly one");
  }
  evidence.authorizations_false =
      !boolean(parent, "training_authorized") &&
      !boolean(parent, "long_run_authorized") &&
      !boolean(parent, "production_or_end_to_end_authorized") &&
      !boolean(parent, "follow_on_repair_authorized");
  try {
    Records parent_audit = parse_records(read_bytes(
        repository_root / ".build/tests/representation_psm_v1_audit.log"));
    evidence.audit_pass =
        required(parent_audit, "schema") ==
            "wikimyei.mtf_jepa_mae_vicreg.psm_audit.v1" &&
        boolean(parent_audit, "audit.pass") &&
        number(parent_audit, "audit.authoritative_attempt_count") == 1.0 &&
        required(parent_audit, "audit.authoritative_log.sha256") ==
            kParentSha256 &&
        required(parent_audit, "audit.gate.classification") ==
            "coarse_position_separation_sufficient" &&
        parent_audit.errors.empty();
    if (!evidence.audit_pass) {
      fail(parent, "sealed parent audit did not independently pass");
    }
  } catch (const std::exception &error) {
    fail(parent, std::string("cannot read sealed parent audit: ") +
                     error.what());
  }
  return evidence;
}

[[nodiscard]] bool verify_no_training(Records &srr) {
  const bool result = !boolean(srr, "optimizer_constructed") &&
                      number(srr, "optimizer_steps") == 0.0 &&
                      number(srr, "backward_calls") == 0.0 &&
                      number(srr, "training_loop_calls") == 0.0 &&
                      number(srr, "augmentation_launcher_calls") == 0.0 &&
                      number(srr, "end_to_end_calls") == 0.0 &&
                      !boolean(srr, "training_authorized") &&
                      !boolean(srr, "augmentation_change_authorized") &&
                      !boolean(srr, "long_run_authorized") &&
                      !boolean(srr, "production_or_end_to_end_authorized") &&
                      !boolean(srr,
                               "follow_on_production_repair_authorized");
  if (!result) {
    fail(srr, "SRR no-training/no-end-to-end contract failed");
  }
  return result;
}

[[nodiscard]] bool verify_permuted_matrix(Records &records,
                                          const Matrix &source,
                                          const Matrix &shuffled,
                                          uint64_t tag,
                                          const std::string &label) {
  const auto permutation = sattolo(source.rows, tag);
  bool exact = source.rows == shuffled.rows &&
               source.columns == shuffled.columns;
  if (exact) {
    for (std::size_t row = 0; row < source.rows; ++row) {
      for (std::size_t column = 0; column < source.columns; ++column) {
        exact = exact &&
                source(permutation[row], column) == shuffled(row, column);
      }
    }
  }
  if (!exact) {
    fail(records, label + " does not match frozen Sattolo permutation");
  }
  return exact;
}

[[nodiscard]] std::string parent_arm(std::size_t arm) {
  switch (arm) {
  case 0:
    return "channel";
  case 1:
    return "channel_domain_scale_bin";
  case 3:
    return "encoder";
  default:
    throw std::runtime_error("shadow arm has no parent mapping");
  }
}

[[nodiscard]] OrderSummary as_order_summary(
    const psm_gate::OrderInput &input) {
  return {.point = input.point,
          .interval = {.low = input.low, .high = input.high},
          .seed = input.seed_points};
}

[[nodiscard]] psm_gate::OrderInput as_order_input(const OrderSummary &input) {
  return {.point = input.point,
          .low = input.interval.low,
          .high = input.interval.high,
          .seed_points = input.seed};
}

struct IndependentGate {
  std::string classification{"invalid_mechanics"};
  std::string reason{"invalid_numeric"};
};

[[nodiscard]] IndependentGate
independent_gate(const srr_gate::GateInput &input) {
  const auto continuous_valid = [](const psm_gate::ContinuousInput &value) {
    return continuous_classification(value) != "invalid_numeric";
  };
  const auto order_name = [](const psm_gate::OrderInput &value) {
    return order_classification(as_order_summary(value));
  };
  const bool numeric = continuous_valid(input.offline_minus_channel) &&
                       continuous_valid(input.offline_minus_encoder) &&
                       continuous_valid(input.shadow_minus_channel) &&
                       continuous_valid(input.shadow_minus_encoder) &&
                       order_name(input.channel_order) != "invalid_numeric" &&
                       order_name(input.offline_order) != "invalid_numeric" &&
                       order_name(input.shadow_order) != "invalid_numeric" &&
                       order_name(input.encoder_order) != "invalid_numeric" &&
                       std::isfinite(input.offline_equivalence_max_abs) &&
                       input.offline_equivalence_max_abs >= 0.0 &&
                       std::isfinite(input.device_translation_max_abs) &&
                       input.device_translation_max_abs >= 0.0;
  if (!numeric) {
    return {};
  }
  const bool local = input.validity.no_training_or_end_to_end &&
                     input.validity.local_contracts_exact &&
                     input.validity.parameters_and_rng_unchanged &&
                     input.validity.partition_and_projection_valid &&
                     input.validity.deterministic_tables_valid;
  if (!local) {
    return {.classification = "invalid_mechanics", .reason = "local_contract"};
  }
  if (!input.validity.manifest_exact) {
    return {.classification = "invalid_mechanics", .reason = "manifest"};
  }
  for (const auto &[condition, reason] :
       std::array<std::pair<bool, const char *>, 5>{{
           {input.validity.parent_artifacts_exact, "parent_artifact"},
           {input.validity.parent_classification_exact,
            "parent_classification"},
           {input.validity.parent_attempt_count_exact,
            "parent_attempt_count"},
           {input.validity.parent_audit_pass, "parent_audit"},
           {input.validity.parent_authorizations_false,
            "parent_authorization"},
       }}) {
    if (!condition) {
      return {.classification = "parent_evidence_failure", .reason = reason};
    }
  }
  if (!input.validity.all_reference_keys_exact) {
    return {.classification = "offline_reference_failure",
            .reason = "reference_keys"};
  }
  if (!input.validity.offline_feature_hashes_exact) {
    return {.classification = "offline_reference_failure",
            .reason = "reference_feature_hash"};
  }
  if (!input.validity.offline_bytes_exact) {
    return {.classification = "offline_reference_failure",
            .reason = "offline_byte_identity"};
  }
  if (input.offline_equivalence_max_abs >
      srr_gate::kOfflineEquivalenceTolerance) {
    return {.classification = "offline_reference_failure",
            .reason = "offline_tolerance"};
  }
  if (order_name(input.channel_order) == "order_decodable") {
    return {.classification = "offline_reference_failure",
            .reason = "channel_order_boundary"};
  }
  if (order_name(input.encoder_order) != "order_decodable") {
    return {.classification = "offline_reference_failure",
            .reason = "encoder_order_boundary"};
  }
  if (continuous_classification(input.offline_minus_channel) !=
      "material_gain") {
    return {.classification = "offline_reference_failure",
            .reason = "offline_material_gain"};
  }
  const std::string offline_encoder =
      continuous_classification(input.offline_minus_encoder);
  if (offline_encoder != "noninferior" && offline_encoder != "material_gain") {
    return {.classification = "offline_reference_failure",
            .reason = "offline_noninferiority"};
  }
  if (order_name(input.offline_order) != "order_decodable") {
    return {.classification = "offline_reference_failure",
            .reason = "offline_order_boundary"};
  }
  if (!input.validity.device_contracts_exact) {
    return {.classification = "device_translation_failure",
            .reason = "device_contract"};
  }
  if (input.device_translation_max_abs >
      srr_gate::kDeviceTranslationTolerance) {
    return {.classification = "device_translation_failure",
            .reason = "device_tolerance"};
  }
  constexpr std::array<const char *, 4> continuous_reasons{
      "channel_continuous_shuffle", "offline_cdsb_continuous_shuffle",
      "shadow_continuous_shuffle", "encoder_continuous_shuffle"};
  constexpr std::array<const char *, 4> order_reasons{
      "channel_order_shuffle", "offline_cdsb_order_shuffle",
      "shadow_order_shuffle", "encoder_order_shuffle"};
  for (std::size_t arm = 0; arm < 4; ++arm) {
    if (!input.validity.continuous_shuffle_pass[arm]) {
      return {.classification = "invalid_mechanics",
              .reason = continuous_reasons[arm]};
    }
    if (!input.validity.order_shuffle_pass[arm]) {
      return {.classification = "invalid_mechanics",
              .reason = order_reasons[arm]};
    }
  }
  const bool material =
      continuous_classification(input.shadow_minus_channel) == "material_gain";
  const std::string shadow_encoder =
      continuous_classification(input.shadow_minus_encoder);
  const bool noninferior = shadow_encoder == "noninferior" ||
                           shadow_encoder == "material_gain";
  const bool order = order_name(input.shadow_order) == "order_decodable";
  return material && noninferior && order
             ? IndependentGate{.classification =
                                   "structured_readout_reproduced",
                               .reason = "none"}
             : IndependentGate{.classification = "readout_gate_failure",
                               .reason = "shadow_quality"};
}

struct AuditResult {
  std::string classification{"invalid_mechanics"};
  std::string failure_reason{"invalid_numeric"};
  std::size_t continuous_points{0};
  std::size_t order_points{0};
  std::size_t bootstrap_intervals{0};
  std::size_t parent_endpoint_comparisons{0};
  std::size_t reference_feature_hashes{0};
  std::size_t cpu_identities{0};
  std::size_t device_thresholds{0};
  std::size_t attempt_count{0};
  ParentEvidence parent{};
  ManifestEvidence manifest{};
  bool all_reference_keys_exact{false};
  bool reference_feature_hashes_exact{false};
  bool prediction_schema_exact{false};
  bool continuous_target_hash_exact{false};
  bool permutation_tables_exact{false};
  bool bootstrap_table_hash_exact{false};
  bool cpu_identities_exact{false};
  bool schema_closed{false};
  std::size_t unaccessed_key_count{0};
};

[[nodiscard]] AuditResult audit(Records &srr, Records &parent,
                                std::string_view srr_raw,
                                std::string_view parent_raw,
                                const std::filesystem::path &repository_root,
                                std::string_view auditor_binary) {
  AuditResult audit{};
  expect_text(srr, "schema", "wikimyei.mtf_jepa_mae_vicreg.srr.v1");
  expect_text(srr, "experiment", "structured-readout-repair");
  expect_text(srr, "device", "cuda:0");
  expect_text(srr, "dtype", "float32");
  expect_bool(srr, "srr.attempt.consumed", true);
  expect_text(srr, "execution_status", "srr_measurements_complete");
  if (srr.malformed_line_count != 0 ||
      srr.recognized_runtime_noise_count != 2) {
    fail(srr, "authoritative log line schema/runtime-noise contract failed");
  }
  audit.attempt_count =
      count_exact_line(srr_raw, "srr.attempt.consumed=true");
  if (audit.attempt_count != 1 ||
      count_exact_line(srr_raw, "srr.attempt.consumed=false") != 0) {
    fail(srr, "SRR authoritative attempt count is not exactly one");
  }
  const bool no_training = verify_no_training(srr);
  audit.parent = verify_parent(parent, parent_raw, repository_root);
  if (!parent.errors.empty()) {
    audit.parent.artifacts_exact = false;
  }
  audit.manifest = verify_manifest(srr, repository_root, auditor_binary);

  compare_text(srr, parent, "srr.projection.rssm_hash",
               "psm.projection.rssm_hash");
  compare_text(srr, parent, "srr.projection.psm_hash",
               "psm.projection.psm_hash");
  compare_text(srr, parent, "srr.token_layout.hash",
               "psm.token_layout.hash");
  compare_text(srr, parent, "srr.bootstrap.table_hash",
               "psm.bootstrap.table_hash");
  compare_text(srr, parent, "srr.normalization.mean_hash",
               "psm.normalization.mean_hash");
  compare_text(srr, parent, "srr.normalization.inv_std_hash",
               "psm.normalization.inv_std_hash");

  expect_text(srr, "srr.environment.device", "cuda:0");
  expect_text(srr, "srr.environment.dtype", "float32");
  expect_close(srr, "srr.environment.cpu_threads", 1.0, 0.0);
  expect_close(srr, "srr.environment.cpu_interop_threads", 1.0, 0.0);
  for (const std::string_view key :
       {"srr.environment.deterministic_algorithms",
        "srr.environment.deterministic_cudnn",
        "srr.environment.tf32_cublas_disabled",
        "srr.environment.tf32_cudnn_disabled",
        "srr.environment.cublas_workspace_exact", "srr.environment.pass"}) {
    expect_bool(srr, std::string(key), true);
  }
  expect_bool(srr, "srr.environment.deterministic_warn_only", false);

  struct PermutationMapping {
    const char *srr;
    const char *psm;
  };
  constexpr std::array<PermutationMapping, 9> permutation_mappings{{
      {"continuous_fit", "shuffle.fit"},
      {"continuous_validation", "shuffle.validation"},
      {"continuous_test", "shuffle.test"},
      {"order_fit_n_32", "order_shuffle.fit.n_32"},
      {"order_fit_n_64", "order_shuffle.fit.n_64"},
      {"order_fit_n_128", "order_shuffle.fit.n_128"},
      {"order_fit_n_256", "order_shuffle.fit.n_256"},
      {"order_validation", "order_shuffle.validation"},
      {"order_test", "order_shuffle.test"},
  }};
  for (const auto &mapping : permutation_mappings) {
    const std::string left =
        "srr.permutation." + std::string(mapping.srr);
    const std::string right = "psm." + std::string(mapping.psm);
    compare_text(srr, parent, left + ".rows", right + ".rows");
    compare_text(srr, parent, left + ".hash", right + ".hash");
    compare_text(srr, parent, left + ".pass", right + ".pass");
  }
  audit.permutation_tables_exact = true;
  for (const auto &spec : kExpectedPermutations) {
    const auto generated = sattolo(spec.rows, spec.tag);
    const std::string observed_hash =
        hex64(stable_int64_vector_hash(generated));
    const std::string prefix =
        "srr.permutation." + std::string(spec.name);
    const bool exact = generated.size() == spec.rows &&
                       observed_hash == spec.hash &&
                       required(srr, prefix + ".hash") == observed_hash &&
                       number(srr, prefix + ".rows") ==
                           static_cast<double>(spec.rows) &&
                       boolean(srr, prefix + ".pass");
    audit.permutation_tables_exact =
        audit.permutation_tables_exact && exact;
  }
  if (!audit.permutation_tables_exact) {
    fail(srr, "independently reconstructed permutation table mismatch");
  }

  for (const std::string_view dataset :
       {"normalizer", "probe_train", "probe_validation", "test"}) {
    for (const std::string_view state : {"unnormalized", "normalized"}) {
      const std::string left = "srr.data." + std::string(dataset) + "." +
                               std::string(state);
      const std::string right = "psm.data." + std::string(dataset) + "." +
                                std::string(state);
      for (const std::string_view item :
           {"group_ids_hash", "data_hash", "mask_hash", "target_hash",
            "group_begin", "groups"}) {
        compare_text(srr, parent, left + "." + std::string(item),
                     right + "." + std::string(item));
      }
    }
  }
  for (const std::string_view dataset :
       {"reversed_train", "reversed_validation", "reversed_test"}) {
    const std::string left =
        "srr.data." + std::string(dataset) + ".normalized";
    const std::string right =
        "psm.data." + std::string(dataset) + ".normalized";
    for (const std::string_view item :
         {"group_ids_hash", "data_hash", "mask_hash", "target_hash",
          "group_begin", "groups"}) {
      compare_text(srr, parent, left + "." + std::string(item),
                   right + "." + std::string(item));
    }
  }

  const Matrix target = csv_matrix(srr, "srr.audit.target.test_csv", kRows,
                                   kTargets);
  const Matrix shuffled_target = csv_matrix(
      srr, "srr.audit.target.shuffled_test_csv", kRows, kTargets);
  const Matrix order_target =
      csv_matrix(srr, "srr.audit.target.order_test_csv", kOrderRows, 1);
  const Matrix shuffled_order_target = csv_matrix(
      srr, "srr.audit.target.shuffled_order_test_csv", kOrderRows, 1);
  const std::string reconstructed_target_hash =
      hex64(stable_double_matrix_hash(target));
  audit.continuous_target_hash_exact =
      reconstructed_target_hash ==
      required(srr, "srr.data.test.normalized.target_hash");
  if (!audit.continuous_target_hash_exact) {
    fail(srr, "logged continuous target CSV does not match the frozen "
              "normalized target hash");
  }
  const bool target_permutations_exact =
      verify_permuted_matrix(srr, target, shuffled_target,
                             kContinuousTestShuffleTag,
                             "continuous shuffled target") &&
      verify_permuted_matrix(srr, order_target, shuffled_order_target,
                             kOrderTestShuffleTag,
                             "order shuffled target");
  for (std::size_t row = 0; row < kOrderRows; ++row) {
    const double expected = row % 2 == 0 ? 1.0 : -1.0;
    if (order_target(row, 0) != expected) {
      fail(srr, "order target is not the frozen alternating pair label");
      break;
    }
  }

  ContinuousGrid continuous{};
  OrderGrid order{};
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    for (std::size_t arm = 0; arm < kArms.size(); ++arm) {
      const std::string prefix = seed_arm_prefix(kSeeds[seed], kArms[arm]);
      continuous[seed][arm][0] =
          read_continuous_curve(srr, prefix + ".probe", target);
      continuous[seed][arm][1] = read_continuous_curve(
          srr, prefix + ".shuffled_probe", shuffled_target);
      order[seed][arm][0] =
          read_order_curve(srr, prefix + ".order_probe", order_target);
      order[seed][arm][1] = read_order_curve(
          srr, prefix + ".order_shuffled_probe", shuffled_order_target);
      audit.continuous_points += 8;
      audit.order_points += 8;
    }
  }

  std::size_t prediction_csv_count = 0;
  std::size_t target_csv_count = 0;
  for (const auto &[key, value] : srr.values) {
    (void)value;
    if (ends_with(key, ".prediction_csv")) {
      ++prediction_csv_count;
    } else if (key.rfind("srr.audit.target.", 0) == 0 &&
               ends_with(key, "_csv")) {
      ++target_csv_count;
    }
  }
  audit.prediction_schema_exact = prediction_csv_count == 192 &&
                                  target_csv_count == 4;
  if (!audit.prediction_schema_exact) {
    fail(srr, "prediction/target CSV schema count mismatch");
  }

  const BootstrapRows rows = bootstrap_rows();
  audit.bootstrap_table_hash_exact =
      hex64(bootstrap_rows_hash(rows)) == "408205cac33d403d" &&
      required(srr, "srr.bootstrap.table_hash") == "408205cac33d403d";
  if (!audit.bootstrap_table_hash_exact) {
    fail(srr, "independently reconstructed bootstrap table hash mismatch");
  }
  BootstrapGrid continuous_boot{};
  BootstrapGrid order_boot{};
  for (std::size_t mode = 0; mode < 2; ++mode) {
    const Matrix &continuous_target = mode == 0 ? target : shuffled_target;
    const Matrix &binary_target = mode == 0 ? order_target
                                            : shuffled_order_target;
    for (std::size_t replicate = 0; replicate < kBootstrapReplicates;
         ++replicate) {
      const std::vector<std::size_t> sampled(rows[replicate].begin(),
                                             rows[replicate].end());
      const TargetStats stats = target_stats(continuous_target, &sampled);
      for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
        for (std::size_t arm = 0; arm < kArms.size(); ++arm) {
          continuous_boot[seed][arm][mode][replicate] =
              resampled_continuous_area(continuous[seed][arm][mode],
                                        continuous_target, sampled, stats);
          order_boot[seed][arm][mode][replicate] = resampled_order_area(
              order[seed][arm][mode], binary_target, sampled);
        }
      }
    }
  }

  std::array<std::array<ArmSummary, 2>, 4> continuous_summary{};
  std::array<std::array<OrderSummary, 2>, 4> order_summary_values{};
  std::array<bool, 4> continuous_control_by_arm{};
  std::array<bool, 4> order_control_by_arm{};
  bool continuous_shuffles = true;
  bool order_shuffles = true;
  for (std::size_t arm = 0; arm < kArms.size(); ++arm) {
    const std::string prefix =
        "srr.summary.arm." + std::string(kArms[arm]);
    for (std::size_t mode = 0; mode < 2; ++mode) {
      continuous_summary[arm][mode] =
          arm_summary(continuous, continuous_boot, arm, mode);
      order_summary_values[arm][mode] =
          order_summary(order, order_boot, arm, mode);
      verify_continuous_summary(srr, prefix,
                                continuous_summary[arm][mode], mode == 1);
      verify_order_summary(srr,
                           prefix + (mode == 0 ? ".order"
                                               : ".order_shuffled"),
                           order_summary_values[arm][mode]);
      audit.bootstrap_intervals += 2;
    }
    const bool continuous_pass =
        continuous_summary[arm][1].point <= 0.02 &&
        continuous_summary[arm][1].interval.high <= 0.05;
    const bool order_pass = order_summary_values[arm][1].point <= 0.55 &&
                            order_summary_values[arm][1].interval.high <= 0.60;
    continuous_control_by_arm[arm] = continuous_pass;
    order_control_by_arm[arm] = order_pass;
    expect_bool(srr, prefix + ".continuous_shuffle_pass", continuous_pass);
    expect_bool(srr, prefix + ".order_shuffle_pass", order_pass);
    continuous_shuffles = continuous_shuffles && continuous_pass;
    order_shuffles = order_shuffles && order_pass;
  }

  struct NamedContrast {
    const char *name;
    std::size_t downstream;
    std::size_t upstream;
  };
  constexpr std::array<NamedContrast, 5> contrast_plan{{
      {"encoder_minus_channel", 3, 0},
      {"offline_cdsb_minus_channel", 1, 0},
      {"offline_cdsb_minus_encoder", 1, 3},
      {"shadow_minus_channel", 2, 0},
      {"shadow_minus_encoder", 2, 3},
  }};
  std::array<psm_gate::ContinuousInput, contrast_plan.size()> contrasts{};
  for (std::size_t index = 0; index < contrast_plan.size(); ++index) {
    contrasts[index] = contrast(continuous, continuous_boot,
                                contrast_plan[index].downstream,
                                contrast_plan[index].upstream);
    verify_contrast(srr,
                    "srr.summary.contrast." +
                        std::string(contrast_plan[index].name),
                    contrasts[index]);
    ++audit.bootstrap_intervals;
  }

  // Exact parent feature identities and capture-source identities.
  bool reference_feature_hashes_exact = true;
  bool cpu_identities_exact = true;
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    for (const std::string_view dataset : kDatasets) {
      const std::string sp = "srr.seed_" + std::string(kSeeds[seed]) +
                             ".capture." + std::string(dataset);
      const std::string pp = "psm.seed_" + std::string(kSeeds[seed]) +
                             ".capture." + std::string(dataset);
      compare_text(srr, parent, sp + ".encoder_hash", pp + ".encoder_hash");
      compare_text(srr, parent, sp + ".served_hash", pp + ".served_hash");
      compare_text(srr, parent, sp + ".metadata_structure_hash",
                   pp + ".metadata_structure_hash");
      for (std::size_t arm : {std::size_t{0}, std::size_t{1},
                              std::size_t{3}}) {
        const std::size_t feature_errors_before = srr.errors.size();
        const std::string observed =
            sp + ".arm." + std::string(kArms[arm]) + ".float64_hash";
        const std::string expected = sp + ".arm." +
                                     std::string(kArms[arm]) +
                                     ".expected_float64_hash";
        const std::string parent_key = pp + ".arm." + parent_arm(arm) +
                                       ".float64_hash";
        compare_text(srr, parent, observed, parent_key);
        compare_text(srr, parent, expected, parent_key);
        if (!boolean(srr, sp + ".arm." + std::string(kArms[arm]) +
                              ".parent_hash_exact")) {
          fail_reference(srr, "parent feature-hash receipt is false: " +
                                  sp + ".arm." +
                                  std::string(kArms[arm]));
        }
        reference_feature_hashes_exact =
            reference_feature_hashes_exact &&
            srr.errors.size() == feature_errors_before;
        ++audit.reference_feature_hashes;
      }
      const bool offline_cpu_identity =
          required(srr, sp + ".audit_hash") ==
          required(srr, sp + ".arm.offline_cdsb.float64_hash");
      const bool shadow_cpu_identity =
          required(srr, sp + ".shadow_hash") ==
          required(srr, sp + ".arm.shadow.float64_hash");
      if (!offline_cpu_identity || !shadow_cpu_identity) {
        fail_reference(srr, "CPU64 offline/shadow byte identity mismatch: " +
                                sp);
      }
      cpu_identities_exact = cpu_identities_exact && offline_cpu_identity &&
                             shadow_cpu_identity;
      ++audit.cpu_identities;
      ++audit.device_thresholds;
      const double offline = number(srr, sp + ".offline_equivalence_max_abs");
      const double device = number(srr, sp + ".device_translation_max_abs");
      if (offline < 0.0) {
        fail(srr, "negative capture offline equivalence diagnostic: " + sp);
      }
      if (device < 0.0) {
        fail(srr, "negative capture device translation diagnostic: " + sp);
      }
    }
  }
  audit.reference_feature_hashes_exact = reference_feature_hashes_exact;
  audit.cpu_identities_exact = cpu_identities_exact;
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    for (std::size_t arm : {std::size_t{0}, std::size_t{1},
                            std::size_t{3}}) {
      compare_reference_curve(srr, parent, kSeeds[seed], kArms[arm],
                              parent_arm(arm));
      audit.parent_endpoint_comparisons += 164;
    }
  }
  for (std::size_t arm : {std::size_t{0}, std::size_t{1},
                          std::size_t{3}}) {
    compare_arm_summary(srr, parent, kArms[arm], parent_arm(arm));
    audit.parent_endpoint_comparisons += 29;
  }
  compare_contrast(
      srr, parent, "srr.summary.contrast.encoder_minus_channel",
      "psm.summary.contrast.encoder_minus_channel");
  compare_contrast(
      srr, parent, "srr.summary.contrast.offline_cdsb_minus_channel",
      "psm.summary.contrast.channel_domain_scale_bin_minus_channel");
  compare_contrast(
      srr, parent, "srr.summary.contrast.offline_cdsb_minus_encoder",
      "psm.summary.contrast.channel_domain_scale_bin_minus_encoder");
  audit.parent_endpoint_comparisons += 3 * 11;
  const bool parent_comparison_surface_exact =
      srr.reference_comparison_count == 1857 &&
      srr.reference_comparison_failure_count == 0;
  if (!parent_comparison_surface_exact) {
    if (srr.reference_comparison_count != 1857 &&
        srr.reference_comparison_failure_count == 0) {
      ++srr.reference_comparison_failure_count;
    }
    fail_reference(srr, "complete parent-reference comparison surface "
                        "mismatch");
  }

  bool literal_endpoints = true;
  for (std::size_t reference = 0; reference < 3; ++reference) {
    const std::size_t arm = reference == 0 ? 0 : reference == 1 ? 1 : 3;
    for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
      literal_endpoints =
          literal_endpoints &&
          close(continuous[seed][arm][0].area,
                kExpectedAulc[reference][seed]) &&
          close(order[seed][arm][0].area,
                kExpectedOrderAulc[reference][seed]);
    }
    for (std::size_t family = 0; family < kFamilies.size(); ++family) {
      literal_endpoints =
          literal_endpoints &&
          close(continuous_summary[arm][0].family[family],
                kExpectedFamilyAulc[reference][family]);
    }
  }
  if (!literal_endpoints) {
    fail_reference(srr, "literal frozen C/D/E endpoints were not reproduced");
  }
  audit.all_reference_keys_exact =
      parent_comparison_surface_exact && literal_endpoints;

  const bool parent_boundary =
      continuous_classification(contrasts[0]) == "material_gain" &&
      order_classification(order_summary_values[0][0]) !=
          "order_decodable" &&
      order_classification(order_summary_values[3][0]) ==
          "order_decodable";
  const std::string offline_encoder = continuous_classification(contrasts[2]);
  const bool offline_gate =
      continuous_classification(contrasts[1]) == "material_gain" &&
      (offline_encoder == "noninferior" ||
       offline_encoder == "material_gain") &&
      order_classification(order_summary_values[1][0]) ==
          "order_decodable";
  const bool encoder_material =
      continuous_classification(contrasts[0]) == "material_gain";
  const bool channel_not_order =
      order_classification(order_summary_values[0][0]) != "order_decodable";
  const bool encoder_order =
      order_classification(order_summary_values[3][0]) == "order_decodable";
  const bool offline_material =
      continuous_classification(contrasts[1]) == "material_gain";
  const bool offline_noninferior = offline_encoder == "noninferior" ||
                                   offline_encoder == "material_gain";
  const bool offline_order =
      order_classification(order_summary_values[1][0]) == "order_decodable";
  const bool displayed_subset = literal_endpoints && parent_boundary &&
                                offline_gate;
  expect_bool(srr, "srr.reference.displayed_scalars_exact", literal_endpoints);
  expect_bool(srr, "srr.reference.encoder_material_gain_over_channel",
              encoder_material);
  expect_bool(srr, "srr.reference.channel_not_order_decodable",
              channel_not_order);
  expect_bool(srr, "srr.reference.encoder_order_decodable", encoder_order);
  expect_bool(srr, "srr.reference.offline_material_gain_over_channel",
              offline_material);
  expect_bool(srr, "srr.reference.offline_noninferior_to_encoder",
              offline_noninferior);
  expect_bool(srr, "srr.reference.offline_order_decodable", offline_order);
  expect_bool(srr, "srr.reference.displayed_subset_exact", displayed_subset);
  expect_text(srr, "srr.reference.all_reference_keys_exact", "unchecked");

  double capture_offline_max = 0.0;
  double capture_device_max = 0.0;
  for (const std::string_view seed : kSeeds) {
    for (const std::string_view dataset : kDatasets) {
      const std::string prefix = "srr.seed_" + std::string(seed) +
                                 ".capture." + std::string(dataset);
      capture_offline_max = std::max(
          capture_offline_max,
          number(srr, prefix + ".offline_equivalence_max_abs"));
      capture_device_max = std::max(
          capture_device_max,
          number(srr, prefix + ".device_translation_max_abs"));
    }
  }
  expect_close(srr, "srr.prefit.offline_equivalence_max_abs",
               capture_offline_max);
  expect_close(srr, "srr.prefit.device_translation_max_abs",
               capture_device_max);
  const double relative_l2 =
      number(srr, "srr.prefit.device_relative_l2_max");
  if (relative_l2 < 0.0) {
    fail(srr, "negative device relative L2 diagnostic");
  }

  bool seed_capture = true;
  bool parameters = true;
  bool combined_shadow_contracts = true;
  bool feature_hashes = true;
  for (const std::string_view seed : kSeeds) {
    const std::string prefix = "srr.seed_" + std::string(seed);
    seed_capture = boolean(srr, prefix + ".public_sandwich_exact") &&
                   boolean(srr, prefix + ".direct_encoder_exact") &&
                   boolean(srr, prefix + ".repeated_capture_exact") &&
                   boolean(srr, prefix + ".token_layout_exact") &&
                   seed_capture;
    parameters = boolean(srr, prefix + ".parameters_and_rng_unchanged") &&
                 parameters;
    combined_shadow_contracts =
        boolean(srr, prefix + ".shadow_contracts_exact") &&
        combined_shadow_contracts;
    feature_hashes = boolean(srr, prefix + ".parent_feature_hashes_exact") &&
                     feature_hashes;
  }
  const bool capture = boolean(srr, "srr.data.identity_exact") &&
                       boolean(srr,
                               "srr.data.normalization_preserved_identity") &&
                       boolean(srr,
                               "srr.prefit.cross_seed_token_structure_exact") &&
                       boolean(srr, "srr.prefit.metadata_plan_exact") &&
                       seed_capture;
  const bool partitions = boolean(srr, "srr.tokenizer_plan.pass") &&
                          boolean(srr, "srr.token_layout.pass") &&
                          boolean(srr, "srr.partitions.pass") &&
                          boolean(srr, "srr.projection.pass");
  const bool measured_deterministic =
      boolean(srr, "srr.environment.pass") &&
      boolean(srr, "srr.permutations_valid") &&
      target_permutations_exact &&
      boolean(srr, "srr.order_shuffle_balanced") &&
      boolean(srr, "srr.bootstrap.valid") &&
      boolean(srr, "srr.ridge.pass");
  const bool shadow_input_unchanged =
      boolean(srr, "srr.prefit.shadow_input_unchanged");
  const bool audit_contract_exact =
      boolean(srr, "srr.prefit.audit_contract_exact");
  const bool device_contract_exact =
      boolean(srr, "srr.prefit.device_contract_exact");
  const bool repeated_device_exact =
      boolean(srr, "srr.prefit.repeated_device_exact");
  const bool measured_local_contracts =
      capture && shadow_input_unchanged && audit_contract_exact;
  const bool local_prefit = no_training && measured_local_contracts &&
                            parameters && partitions && measured_deterministic;
  const bool offline_feature_hashes =
      feature_hashes && audit.reference_feature_hashes_exact &&
      boolean(srr, "srr.prefit.parent_feature_hashes_exact");
  const bool offline_bytes =
      boolean(srr, "srr.prefit.offline_bytes_exact") &&
      audit.cpu_identities_exact;
  const bool offline_prefit = offline_feature_hashes && offline_bytes &&
                              capture_offline_max <=
                                  srr_gate::kOfflineEquivalenceTolerance;
  const bool device_contracts =
      device_contract_exact && repeated_device_exact;
  const bool device_prefit = device_contracts &&
                             capture_device_max <=
                                 srr_gate::kDeviceTranslationTolerance;
  if (shadow_input_unchanged && audit_contract_exact &&
      device_contract_exact && repeated_device_exact &&
      !combined_shadow_contracts) {
    fail(srr, "per-seed and aggregate shadow contract receipts disagree");
  }
  expect_bool(srr, "srr.prefit.local_validity_pass", local_prefit);
  expect_bool(srr, "srr.prefit.offline_reference_pass", offline_prefit);
  expect_bool(srr, "srr.prefit.device_translation_pass", device_prefit);
  expect_bool(srr, "srr.prefit.mechanics_pass",
              local_prefit && offline_prefit && device_prefit);
  expect_bool(srr, "srr.summary.validity.continuous_shuffle_pass",
              continuous_shuffles);
  expect_bool(srr, "srr.summary.validity.order_shuffle_pass", order_shuffles);

  const bool material = continuous_classification(contrasts[3]) ==
                        "material_gain";
  const std::string shadow_encoder = continuous_classification(contrasts[4]);
  const bool noninferior = shadow_encoder == "noninferior" ||
                           shadow_encoder == "material_gain";
  const bool order_decodable =
      order_classification(order_summary_values[2][0]) == "order_decodable";
  expect_bool(srr, "srr.summary.gate.material_gain_over_channel", material);
  expect_bool(srr, "srr.summary.gate.noninferior_to_encoder", noninferior);
  expect_bool(srr, "srr.summary.gate.order_decodable", order_decodable);
  expect_bool(srr, "srr.summary.gate.conditional_quality_candidate",
              material && noninferior && order_decodable);
  expect_bool(srr,
              "srr.summary.gate.final_classification_requires_postrun_audit",
              true);
  expect_bool(srr, "srr.summary.preaudit.numeric_inputs", true);
  expect_bool(srr, "srr.summary.preaudit.manifest_exact", false);
  expect_text(srr, "srr.summary.preaudit.parent_evidence", "unchecked");
  expect_text(srr, "srr.summary.preaudit.all_reference_keys_exact",
              "unchecked");
  expect_bool(srr, "srr.summary.preaudit.mechanics", false);
  expect_text(srr, "srr.summary.gate.preaudit_classification",
              "invalid_mechanics");
  expect_text(srr, "srr.summary.gate.preaudit_failure_reason", "manifest");

  for (const auto &[key, value] : srr.values) {
    (void)value;
    if (srr.accessed_keys.count(key) == 0) {
      ++audit.unaccessed_key_count;
      if (audit.unaccessed_key_count <= 16) {
        fail(srr, "unrecognized authoritative machine key: " + key);
      }
    }
  }
  if (audit.unaccessed_key_count > 16) {
    fail(srr, "additional unrecognized authoritative machine keys: " +
                  std::to_string(audit.unaccessed_key_count - 16));
  }
  audit.schema_closed = audit.unaccessed_key_count == 0;
  const std::size_t externally_classified_errors =
      audit.manifest.error_count + srr.reference_error_count;
  const bool audit_integrity_exact =
      srr.errors.size() == externally_classified_errors;
  const bool local_contracts = measured_local_contracts &&
                               audit.schema_closed && audit_integrity_exact;
  const bool deterministic = measured_deterministic &&
                             audit.permutation_tables_exact &&
                             audit.bootstrap_table_hash_exact;

  srr_gate::GateInput gate_input{};
  gate_input.offline_minus_channel = contrasts[1];
  gate_input.offline_minus_encoder = contrasts[2];
  gate_input.shadow_minus_channel = contrasts[3];
  gate_input.shadow_minus_encoder = contrasts[4];
  gate_input.channel_order = as_order_input(order_summary_values[0][0]);
  gate_input.offline_order = as_order_input(order_summary_values[1][0]);
  gate_input.shadow_order = as_order_input(order_summary_values[2][0]);
  gate_input.encoder_order = as_order_input(order_summary_values[3][0]);
  gate_input.offline_equivalence_max_abs = capture_offline_max;
  gate_input.device_translation_max_abs = capture_device_max;
  gate_input.validity = {
      .no_training_or_end_to_end = no_training,
      .local_contracts_exact = local_contracts,
      .parameters_and_rng_unchanged = parameters,
      .partition_and_projection_valid = partitions,
      .deterministic_tables_valid = deterministic,
      .manifest_exact = audit.manifest.exact,
      .parent_artifacts_exact = audit.parent.artifacts_exact,
      .parent_classification_exact = audit.parent.classification_exact,
      .parent_attempt_count_exact = audit.parent.attempt_count_exact,
      .parent_audit_pass = audit.parent.audit_pass,
      .parent_authorizations_false = audit.parent.authorizations_false,
      .all_reference_keys_exact = audit.all_reference_keys_exact,
      .offline_feature_hashes_exact = offline_feature_hashes,
      .offline_bytes_exact = offline_bytes,
      .device_contracts_exact = device_contracts,
      .continuous_shuffle_pass = continuous_control_by_arm,
      .order_shuffle_pass = order_control_by_arm};
  const auto gate = srr_gate::evaluate(gate_input);
  const auto independent = independent_gate(gate_input);
  audit.classification =
      srr_gate::terminal_classification_name(gate.classification);
  audit.failure_reason = srr_gate::failure_reason_name(gate.failure_reason);
  if (audit.classification != independent.classification ||
      audit.failure_reason != independent.reason) {
    fail(srr, "frozen gate and independent precedence evaluation disagree");
    audit.classification = "invalid_mechanics";
    audit.failure_reason = "local_contract";
  }
  return audit;
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
  const auto malformed = parse_records("a=1\nnot-a-record\n", true);
  expect(malformed.malformed_line_count == 1 && !malformed.errors.empty(),
         "closed line parser fixture");
  expect(sha256("abc") ==
             "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
         "SHA-256 fixture");

  Matrix target{.rows = 4, .columns = 12};
  target.values.resize(48);
  for (std::size_t row = 0; row < target.rows; ++row) {
    for (std::size_t task = 0; task < target.columns; ++task) {
      target.values[row * target.columns + task] =
          static_cast<double>(row + 2 * task);
    }
  }
  const Score perfect = score(target, target);
  expect(close(perfect.macro, 1.0), "perfect R2 fixture");
  expect(hex64(stable_double_matrix_hash(target)) == "eb95ceb20bd3fe88",
         "stable Double tensor hash fixture");
  const auto permutation = sattolo(256, kContinuousTestShuffleTag);
  expect(std::set<std::size_t>(permutation.begin(), permutation.end()).size() ==
             256,
         "Sattolo cardinality fixture");
  for (std::size_t index = 0; index < permutation.size(); ++index) {
    expect(permutation[index] != index, "Sattolo fixed-point fixture");
  }
  const BootstrapRows rows = bootstrap_rows();
  expect(rows.size() == 512 &&
             std::all_of(rows.begin(), rows.end(), [](const auto &row) {
               return std::all_of(row.begin(), row.end(),
                                  [](std::size_t value) { return value < 256; });
             }),
         "bootstrap rows fixture");
  expect(hex64(bootstrap_rows_hash(rows)) == "408205cac33d403d",
         "bootstrap table hash fixture");
  for (const auto &spec : kExpectedPermutations) {
    expect(hex64(stable_int64_vector_hash(sattolo(spec.rows, spec.tag))) ==
               spec.hash,
           "permutation stable hash fixture: " + std::string(spec.name));
  }

  psm_gate::ContinuousInput gain{
      .point = 0.03,
      .low = 0.01,
      .high = 0.05,
      .seed_deltas = {0.02, 0.03, 0.04},
      .family_deltas = {0.01, 0.01, 0.01, 0.01}};
  psm_gate::ContinuousInput parity{
      .point = 0.0,
      .low = -0.01,
      .high = 0.01,
      .seed_deltas = {0.0, 0.0, 0.0},
      .family_deltas = {0.0, 0.0, 0.0, 0.0}};
  OrderSummary order{.point = 0.70,
                     .interval = {.low = 0.60, .high = 0.80},
                     .seed = {0.65, 0.70, 0.75}};
  srr_gate::GateInput gate{};
  gate.offline_minus_channel = gain;
  gate.offline_minus_encoder = parity;
  gate.shadow_minus_channel = gain;
  gate.shadow_minus_encoder = parity;
  gate.channel_order = {.point = 0.50,
                        .low = 0.45,
                        .high = 0.55,
                        .seed_points = {0.49, 0.50, 0.51}};
  gate.offline_order = as_order_input(order);
  gate.shadow_order = as_order_input(order);
  gate.encoder_order = as_order_input(order);
  gate.validity = {.no_training_or_end_to_end = true,
                   .local_contracts_exact = true,
                   .parameters_and_rng_unchanged = true,
                   .partition_and_projection_valid = true,
                   .deterministic_tables_valid = true,
                   .manifest_exact = true,
                   .parent_artifacts_exact = true,
                   .parent_classification_exact = true,
                   .parent_attempt_count_exact = true,
                   .parent_audit_pass = true,
                   .parent_authorizations_false = true,
                   .all_reference_keys_exact = true,
                   .offline_feature_hashes_exact = true,
                   .offline_bytes_exact = true,
                   .device_contracts_exact = true,
                   .continuous_shuffle_pass = {true, true, true, true},
                   .order_shuffle_pass = {true, true, true, true}};
  expect(independent_gate(gate).classification ==
             "structured_readout_reproduced",
         "terminal precedence fixture");
  gate.validity.parent_artifacts_exact = false;
  expect(independent_gate(gate).classification == "parent_evidence_failure" &&
             independent_gate(gate).reason == "parent_artifact",
         "parent precedence fixture");
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
    std::setlocale(LC_NUMERIC, "C");
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
      run_self_test();
      std::cout << "srr_log_auditor_self_test=PASS\n";
      return 0;
    }
    if (argc != 3) {
      throw std::runtime_error(
          "usage: test_structured_readout_repair_log_auditor "
          "<srr-authoritative.log> <parent-psm-authoritative.log>|--self-test");
    }
    std::filesystem::path executable_path;
    if (std::filesystem::exists("/proc/self/exe")) {
      executable_path = std::filesystem::canonical("/proc/self/exe");
    } else {
      executable_path = std::filesystem::canonical(argv[0]);
    }
    const std::string auditor_raw = read_bytes(executable_path);
    const auto repository_root = find_repository_root(executable_path);
    const std::string srr_raw = read_bytes(argv[1]);
    const std::string parent_raw = read_bytes(argv[2]);
    Records srr = parse_records(srr_raw, /*reject_malformed=*/true);
    Records parent = parse_records(parent_raw);
    if (std::filesystem::canonical(argv[1]) !=
        std::filesystem::canonical(
            repository_root /
            ".build/tests/representation_srr_v1_authoritative.log")) {
      fail(srr, "SRR authoritative log argument is not the canonical artifact");
    }
    if (std::filesystem::canonical(argv[2]) !=
        std::filesystem::canonical(
            repository_root /
            ".build/tests/representation_psm_v1_authoritative.log")) {
      fail(parent, "parent authoritative log argument is not the canonical "
                   "artifact");
    }
    AuditResult result = audit(srr, parent, srr_raw, parent_raw,
                               repository_root, auditor_raw);
    const bool pass = srr.errors.empty() && parent.errors.empty();
    if (!pass &&
        (result.classification == "structured_readout_reproduced" ||
         result.classification == "readout_gate_failure")) {
      result.classification = "invalid_mechanics";
      result.failure_reason = "local_contract";
    }

    std::cout << std::boolalpha << std::setprecision(17);
    std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.srr_audit.v1\n";
    std::cout << "audit.srr_machine_line_count=" << srr.machine_line_count
              << '\n';
    std::cout << "audit.parent_machine_line_count="
              << parent.machine_line_count << '\n';
    std::cout << "audit.duplicate_key_count="
              << srr.duplicate_key_count + parent.duplicate_key_count << '\n';
    std::cout << "audit.nonfinite_scalar_count="
              << srr.nonfinite_value_count + parent.nonfinite_value_count
              << '\n';
    std::cout << "audit.csv_value_count=" << srr.csv_value_count << '\n';
    std::cout << "audit.csv_nonfinite_value_count="
              << srr.csv_nonfinite_value_count << '\n';
    std::cout << "audit.malformed_line_count=" << srr.malformed_line_count
              << '\n';
    std::cout << "audit.recognized_runtime_noise_count="
              << srr.recognized_runtime_noise_count << '\n';
    std::cout << "audit.srr_accessed_key_count=" << srr.accessed_keys.size()
              << '\n';
    std::cout << "audit.srr_unaccessed_key_count="
              << result.unaccessed_key_count << '\n';
    std::cout << "audit.srr_schema_closed=" << result.schema_closed << '\n';
    std::cout << "audit.unique_finite_machine_keys="
              << (srr.duplicate_key_count == 0 &&
                  parent.duplicate_key_count == 0 &&
                  srr.nonfinite_value_count == 0 &&
                  parent.nonfinite_value_count == 0 &&
                  srr.csv_nonfinite_value_count == 0 &&
                  srr.malformed_line_count == 0 && result.schema_closed)
              << '\n';
    std::cout << "audit.authoritative_log.bytes=" << srr_raw.size() << '\n';
    std::cout << "audit.authoritative_log.sha256=" << sha256(srr_raw)
              << '\n';
    std::cout << "audit.parent_authoritative_log.bytes="
              << parent_raw.size() << '\n';
    std::cout << "audit.parent_authoritative_log.sha256="
              << sha256(parent_raw) << '\n';
    std::cout << "audit.parent_authoritative_log_bytes_exact="
              << (parent_raw.size() == kParentBytes) << '\n';
    std::cout << "audit.parent_authoritative_log_sha256_exact="
              << (sha256(parent_raw) == kParentSha256) << '\n';
    std::cout << "audit.authoritative_attempt_count=" << result.attempt_count
              << '\n';
    std::cout << "audit.manifest_exact=" << result.manifest.exact << '\n';
    std::cout << "audit.manifest_entry_count="
              << result.manifest.entry_count << '\n';
    std::cout << "audit.prerun_manifest.bytes=" << result.manifest.bytes
              << '\n';
    std::cout << "audit.prerun_manifest.sha256=" << result.manifest.sha256
              << '\n';
    std::cout << "audit.manifest_exact_path_set="
              << result.manifest.exact_path_set << '\n';
    std::cout << "audit.manifest_canonical_containment="
              << result.manifest.canonical_containment << '\n';
    std::cout << "audit.protocol_digest_exact="
              << result.manifest.protocol_digest_exact << '\n';
    std::cout << "audit.prerun_manifest_runtime_binding_exact="
              << result.manifest.runtime_binding_exact << '\n';
    std::cout << "audit.auditor_binary.bytes="
              << result.manifest.auditor_binary_bytes << '\n';
    std::cout << "audit.auditor_binary.sha256="
              << result.manifest.auditor_binary_sha256 << '\n';
    std::cout << "audit.auditor_binary_manifest_exact="
              << result.manifest.auditor_binary_exact << '\n';
    std::cout << "audit.mechanics_log_exact="
              << result.manifest.mechanics_log_exact << '\n';
    std::cout << "audit.preflight_log_exact="
              << result.manifest.preflight_log_exact << '\n';
    std::cout << "audit.parent_artifact_count="
              << result.parent.artifact_count << '\n';
    std::cout << "audit.parent_artifacts_exact="
              << result.parent.artifacts_exact << '\n';
    std::cout << "audit.parent_log_exact=" << result.parent.log_exact << '\n';
    std::cout << "audit.parent_classification_exact="
              << result.parent.classification_exact << '\n';
    std::cout << "audit.parent_attempt_count_exact="
              << result.parent.attempt_count_exact << '\n';
    std::cout << "audit.parent_audit_pass=" << result.parent.audit_pass
              << '\n';
    std::cout << "audit.parent_authorizations_false="
              << result.parent.authorizations_false << '\n';
    std::cout << "audit.parent_evidence_exact="
              << (result.parent.artifacts_exact &&
                  result.parent.log_exact &&
                  result.parent.classification_exact &&
                  result.parent.attempt_count_exact &&
                  result.parent.audit_pass &&
                  result.parent.authorizations_false)
              << '\n';
    std::cout << "audit.prediction_schema_exact="
              << result.prediction_schema_exact << '\n';
    std::cout << "audit.continuous_target_hash_exact="
              << result.continuous_target_hash_exact << '\n';
    std::cout << "audit.permutation_tables_reconstructed_exact="
              << result.permutation_tables_exact << '\n';
    std::cout << "audit.bootstrap_table_hash_reconstructed_exact="
              << result.bootstrap_table_hash_exact << '\n';
    std::cout << "audit.continuous_ladder_points_recomputed="
              << result.continuous_points << '\n';
    std::cout << "audit.order_ladder_points_recomputed="
              << result.order_points << '\n';
    std::cout << "audit.bootstrap_replicates=" << kBootstrapReplicates
              << '\n';
    std::cout << "audit.bootstrap_intervals_recomputed="
              << result.bootstrap_intervals << '\n';
    std::cout << "audit.bootstrap_replicate_evaluations="
              << result.bootstrap_intervals * kBootstrapReplicates << '\n';
    std::cout << "audit.parent_endpoint_comparisons="
              << result.parent_endpoint_comparisons << '\n';
    std::cout << "audit.parent_reference_field_comparisons="
              << srr.reference_comparison_count << '\n';
    std::cout << "audit.all_reference_keys_exact="
              << result.all_reference_keys_exact << '\n';
    std::cout << "audit.reference_feature_hashes_checked="
              << result.reference_feature_hashes << '\n';
    std::cout << "audit.reference_feature_hashes_exact="
              << result.reference_feature_hashes_exact << '\n';
    std::cout << "audit.cpu_identities_checked=" << result.cpu_identities
              << '\n';
    std::cout << "audit.cpu_identities_exact="
              << result.cpu_identities_exact << '\n';
    std::cout << "audit.device_thresholds_checked="
              << result.device_thresholds << '\n';
    std::cout << "audit.raw_preaudit_classification="
              << required(srr, "srr.summary.gate.preaudit_classification")
              << '\n';
    std::cout << "audit.final_classification="
              << result.classification << '\n';
    std::cout << "audit.failure_reason=" << result.failure_reason << '\n';
    std::cout << "audit.gate.classification=" << result.classification
              << '\n';
    std::cout << "audit.gate.failure_reason=" << result.failure_reason
              << '\n';
    std::cout << "audit.limitation=metrics_and_bootstrap_recomputed_from_"
                 "logged_predictions;ridge_fits_and_smallest_alpha_tie_not_"
                 "recomputed;sealed_psm_parent_omits_raw_predictions_so_"
                 "parent_bootstrap_is_authenticated_and_compared_but_not_"
                 "recomputed_from_parent_log_alone;single_consumed_attempt_"
                 "is_proved_within_the_sealed_authoritative_log\n";
    std::cout << "audit.error_count="
              << srr.errors.size() + parent.errors.size() << '\n';
    std::size_t error_index = 0;
    for (const std::string &error : parent.errors) {
      std::cout << "audit.error_" << error_index++ << "=parent: " << error
                << '\n';
    }
    for (const std::string &error : srr.errors) {
      std::cout << "audit.error_" << error_index++ << "=srr: " << error
                << '\n';
    }
    std::cout << "audit.pass=" << pass << '\n';
    return pass ? 0 : 3;
  } catch (const std::exception &error) {
    std::cerr << "srr_log_auditor=FAIL error=" << error.what() << '\n';
    return 2;
  }
}
