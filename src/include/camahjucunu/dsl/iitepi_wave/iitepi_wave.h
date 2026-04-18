/* iitepi_wave.h */
#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace cuwacunu {
namespace camahjucunu {

struct iitepi_wave_wikimyei_decl_t {
  std::string binding_id{};
  std::string family{};
  std::string wikimyei_path{};
  bool train{false};
  bool has_train{false};
  bool halt_train{false};
  std::string profile_id{};
};

struct iitepi_wave_source_decl_t {
  std::string binding_id{};
  std::string family{};
  std::string source_path{};
  std::string symbol{};
  std::string from{};
  std::string to{};
  std::uint64_t workers{0};
  bool force_rebuild_cache{true};
  std::uint64_t range_warn_batches{256};
};

enum class iitepi_wave_evaluation_training_window_e : std::uint8_t {
  IncomingBatch = 0,
};

enum class iitepi_wave_evaluation_report_policy_e : std::uint8_t {
  EpochEndLog = 0,
};

enum class iitepi_wave_evaluation_objective_e : std::uint8_t {
  FutureTargetDimsNll = 0,
};

struct iitepi_wave_evaluation_policy_t {
  iitepi_wave_evaluation_training_window_e training_window{
      iitepi_wave_evaluation_training_window_e::IncomingBatch};
  iitepi_wave_evaluation_report_policy_e report_policy{
      iitepi_wave_evaluation_report_policy_e::EpochEndLog};
  iitepi_wave_evaluation_objective_e objective{
      iitepi_wave_evaluation_objective_e::FutureTargetDimsNll};
};

// Legacy probe aliases remain because the wave DSL still reserves PROBE blocks,
// even though transfer-matrix evaluation is now modeled as a VICReg-owned evaluator.
using iitepi_wave_probe_training_window_e = iitepi_wave_evaluation_training_window_e;
using iitepi_wave_probe_report_policy_e = iitepi_wave_evaluation_report_policy_e;
using iitepi_wave_probe_objective_e = iitepi_wave_evaluation_objective_e;
using iitepi_wave_probe_policy_t = iitepi_wave_evaluation_policy_t;

struct iitepi_wave_probe_decl_t {
  std::string binding_id{};
  std::string family{};
  std::string probe_path{};
  bool has_runtime{false};
  iitepi_wave_evaluation_policy_t policy{};
};

struct iitepi_wave_sink_decl_t {
  std::string binding_id{};
  std::string family{};
  std::string sink_path{};
};

enum class iitepi_wave_mode_flag_e : std::uint64_t {
  None = 0,
  Run = (1ull << 0),
  Train = (1ull << 1),
  Debug = (1ull << 8),
};

[[nodiscard]] constexpr std::uint64_t iitepi_wave_mode_value(
    iitepi_wave_mode_flag_e flag) noexcept {
  return static_cast<std::uint64_t>(flag);
}

[[nodiscard]] constexpr bool iitepi_wave_mode_has(
    std::uint64_t mode_flags,
    iitepi_wave_mode_flag_e flag) noexcept {
  return (mode_flags & iitepi_wave_mode_value(flag)) != 0;
}

[[nodiscard]] constexpr std::uint64_t iitepi_wave_mode_known_mask() noexcept {
  return iitepi_wave_mode_value(iitepi_wave_mode_flag_e::Run) |
         iitepi_wave_mode_value(iitepi_wave_mode_flag_e::Train) |
         iitepi_wave_mode_value(iitepi_wave_mode_flag_e::Debug);
}

[[nodiscard]] inline std::string canonical_iitepi_wave_mode(
    std::uint64_t mode_flags) {
  if (mode_flags == 0) return "none";
  std::vector<std::string> tokens{};
  tokens.reserve(4);
  if (iitepi_wave_mode_has(mode_flags, iitepi_wave_mode_flag_e::Run)) {
    tokens.emplace_back("run");
  }
  if (iitepi_wave_mode_has(mode_flags, iitepi_wave_mode_flag_e::Train)) {
    tokens.emplace_back("train");
  }
  if (iitepi_wave_mode_has(mode_flags, iitepi_wave_mode_flag_e::Debug)) {
    tokens.emplace_back("debug");
  }
  const std::uint64_t unknown = mode_flags & ~iitepi_wave_mode_known_mask();
  if (unknown != 0) {
    std::ostringstream oss;
    oss << "0x" << std::hex << unknown;
    tokens.push_back(oss.str());
  }
  std::ostringstream oss;
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    if (i > 0) oss << "|";
    oss << tokens[i];
  }
  return oss.str();
}

[[nodiscard]] inline bool parse_iitepi_wave_mode_flags(
    std::string mode_expression,
    std::uint64_t* out_mode_flags,
    bool* out_legacy_single_token = nullptr,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (out_legacy_single_token) *out_legacy_single_token = false;
  if (!out_mode_flags) {
    if (error) *error = "output mode flags pointer is null";
    return false;
  }

  auto trim_ascii_copy = [](std::string s) {
    std::size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    std::size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
  };
  auto lower_ascii_copy = [](std::string s) {
    for (char& c : s) {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
  };
  auto parse_numeric_token = [](std::string token, std::uint64_t* out) {
    if (!out) return false;
    token.erase(
        std::remove(token.begin(), token.end(), '_'),
        token.end());
    if (token.empty()) return false;
    if (token.size() > 2 && token[0] == '0' &&
        (token[1] == 'x' || token[1] == 'X')) {
      std::uint64_t value = 0;
      const char* b = token.data() + 2;
      const char* e = token.data() + token.size();
      if (b == e) return false;
      const auto r = std::from_chars(b, e, value, 16);
      if (r.ec != std::errc{} || r.ptr != e) return false;
      *out = value;
      return true;
    }
    if (token.size() > 2 && token[0] == '0' &&
        (token[1] == 'b' || token[1] == 'B')) {
      std::uint64_t value = 0;
      for (std::size_t i = 2; i < token.size(); ++i) {
        const char c = token[i];
        if (c != '0' && c != '1') return false;
        if (value > (std::numeric_limits<std::uint64_t>::max() >> 1)) {
          return false;
        }
        value = (value << 1) | static_cast<std::uint64_t>(c - '0');
      }
      *out = value;
      return true;
    }
    std::uint64_t value = 0;
    const char* b = token.data();
    const char* e = token.data() + token.size();
    const auto r = std::from_chars(b, e, value, 10);
    if (r.ec != std::errc{} || r.ptr != e) return false;
    *out = value;
    return true;
  };

  mode_expression = trim_ascii_copy(std::move(mode_expression));
  if (mode_expression.empty()) {
    if (error) *error = "mode expression is empty";
    return false;
  }

  std::vector<std::string> tokens{};
  std::string current{};
  auto flush = [&]() {
    const std::string token = trim_ascii_copy(current);
    if (!token.empty()) tokens.push_back(token);
    current.clear();
  };
  for (const char c : mode_expression) {
    if (std::isspace(static_cast<unsigned char>(c)) || c == '|' || c == ',' ||
        c == '+' || c == '^') {
      flush();
      continue;
    }
    current.push_back(c);
  }
  flush();

  if (tokens.empty()) {
    if (error) *error = "mode expression does not contain mode tokens";
    return false;
  }

  std::uint64_t mode_flags = 0;
  bool saw_numeric = false;
  for (const auto& raw : tokens) {
    const std::string token = lower_ascii_copy(trim_ascii_copy(raw));
    if (token == "run") {
      mode_flags |= iitepi_wave_mode_value(iitepi_wave_mode_flag_e::Run);
      continue;
    }
    if (token == "train") {
      mode_flags |= iitepi_wave_mode_value(iitepi_wave_mode_flag_e::Train);
      continue;
    }
    if (token == "debug") {
      mode_flags |= iitepi_wave_mode_value(iitepi_wave_mode_flag_e::Debug);
      continue;
    }
    std::uint64_t numeric = 0;
    if (!parse_numeric_token(token, &numeric)) {
      if (error) {
        *error = "unsupported mode token '" + raw +
                 "' (expected run|train|debug or numeric bitmask)";
      }
      return false;
    }
    mode_flags |= numeric;
    saw_numeric = true;
  }

  if (out_legacy_single_token && tokens.size() == 1 && !saw_numeric) {
    const std::string token = lower_ascii_copy(trim_ascii_copy(tokens.front()));
    *out_legacy_single_token = (token == "run" || token == "train");
  }
  *out_mode_flags = mode_flags;
  return true;
}

struct iitepi_wave_t {
  std::string name{};
  std::string circuit_name{};
  std::string mode{};
  std::uint64_t mode_flags{0};
  std::string determinism_policy{"non_deterministic"};
  std::string sampler{};
  std::uint64_t epochs{0};
  std::uint64_t batch_size{0};
  std::uint64_t max_batches_per_epoch{0};
  std::vector<iitepi_wave_wikimyei_decl_t> wikimyeis{};
  std::vector<iitepi_wave_source_decl_t> sources{};
  std::vector<iitepi_wave_probe_decl_t> probes{};
  std::vector<iitepi_wave_sink_decl_t> sinks{};
};

struct iitepi_wave_set_t {
  std::vector<iitepi_wave_t> waves{};
  std::string str() const;
};

namespace dsl {

class iitepiWavePipeline {
 public:
  explicit iitepiWavePipeline(std::string grammar_text);
  iitepi_wave_set_t decode(std::string instruction);

 private:
  std::mutex current_mutex_;
  std::string grammar_text_;
};

iitepi_wave_set_t decode_iitepi_wave_from_dsl(
    std::string grammar_text,
    std::string instruction_text);

} /* namespace dsl */

} /* namespace camahjucunu */
} /* namespace cuwacunu */
