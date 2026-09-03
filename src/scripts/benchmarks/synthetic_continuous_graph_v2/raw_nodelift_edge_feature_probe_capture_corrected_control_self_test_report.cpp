// SPDX-License-Identifier: MIT
// Phase 2B development-only corrected-control raw NodeLift capture successor.
//
// The production capture implementation is included unchanged. This successor
// only replaces the terminal predecessor's stdout self-test transport with an
// exclusive report-file transport. The wrapper build binds the included source
// by SHA-256.

#include <array>
#include <sstream>
#include <string>
#include <string_view>

#define main corrected_control_capture_predecessor_main
#include "raw_nodelift_edge_feature_probe_capture_corrected_control.cpp"
#undef main

namespace {

constexpr std::size_t kMaximumSelfTestPayloadBytes = 16 * 1024;
constexpr std::string_view kSelfTestUsage =
    "--self-test --self-test-report ABS";

class ScopedCoutCapture {
public:
  ScopedCoutCapture() {
    std::cout.flush();
    if (!std::cout) {
      throw std::runtime_error("failed to flush stdout before self-test");
    }
    auto *const previous_buffer = std::cout.rdbuf();
    if (previous_buffer == nullptr) {
      throw std::runtime_error(
          "stdout has no stream buffer before self-test");
    }
    std::cout.rdbuf(capture_.rdbuf());
    previous_buffer_ = previous_buffer;
  }

  ScopedCoutCapture(const ScopedCoutCapture &) = delete;
  ScopedCoutCapture &operator=(const ScopedCoutCapture &) = delete;

  ~ScopedCoutCapture() {
    restore_noexcept();
  }

  [[nodiscard]] std::string finish() {
    std::cout.flush();
    const bool flush_failed = !std::cout;
    restore();
    if (flush_failed) {
      throw std::runtime_error(
          "failed to flush in-memory self-test output buffer");
    }
    std::string payload = capture_.str();
    if (payload.size() > kMaximumSelfTestPayloadBytes) {
      throw std::runtime_error(
          "unexpected self-test output exceeded the bounded payload size");
    }
    return payload;
  }

private:
  void restore() {
    if (previous_buffer_ == nullptr) {
      return;
    }
    std::cout.rdbuf(previous_buffer_);
    previous_buffer_ = nullptr;
  }

  void restore_noexcept() noexcept {
    if (previous_buffer_ != nullptr) {
      try {
        std::cout.rdbuf(previous_buffer_);
        previous_buffer_ = nullptr;
      } catch (...) {
        // Destructors cannot report restoration failure. The normal finish()
        // path restores explicitly; this is the fail-safe unwinding path.
      }
    }
  }

  std::ostringstream capture_;
  std::streambuf *previous_buffer_{nullptr};
};

void reject_unexpected_self_test_payload(const std::string &payload) {
  std::array<std::string_view, 8> lines{};
  std::size_t begin = 0;
  for (auto &line : lines) {
    const auto end = payload.find('\n', begin);
    if (end == std::string::npos) {
      throw std::runtime_error(
          "self-test stdout was not the exact eight-line KV payload");
    }
    line = std::string_view(payload).substr(begin, end - begin);
    begin = end + 1;
  }
  if (begin != payload.size()) {
    throw std::runtime_error(
        "self-test stdout contained content beyond the exact eight-line KV "
        "payload");
  }

  constexpr std::array<std::string_view, 8> expected{
      "schema_id=synthetic_v2_raw_nodelift_edge_feature_probe_corrected_"
      "control_self_test_v1",
      "status=passed",
      "expected_case_count=8",
      "expected_cases=false_structural_padding,oldest_in_capacity_false,"
      "multiple_true_finite,raw96_placement_and_serialization,canonical_"
      "stream_minmax_hash,reject_outside_capacity,reject_nonzero_false,"
      "reject_nonfinite_true",
      "observed_canonical_output_sha256=dadad8ab786ad5205792f4a1aea4eb9b"
      "d154b82c10405d3c5cea36b4423dd5d9",
      "source_binary_binding_required_in_immutable_runner_receipt=true",
      "project_artifact_access=false",
      "status_line=corrected-control mask self-test passed"};
  for (std::size_t index = 0; index < expected.size(); ++index) {
    if (lines[index] != expected[index]) {
      throw std::runtime_error(
          "self-test stdout did not match the frozen eight-line KV payload");
    }
  }
}

[[noreturn]] void reveal_and_rethrow_payload_error(
    const std::string &payload, const std::exception &error) {
  std::cerr << "captured unexpected self-test stdout (" << payload.size()
            << " bytes) follows:\n";
  std::cerr.write(payload.data(), static_cast<std::streamsize>(payload.size()));
  if (payload.empty() || payload.back() != '\n') {
    std::cerr << '\n';
  }
  std::cerr << "end captured unexpected self-test stdout\n";
  throw std::runtime_error(error.what());
}

void run_self_test_to_report(const fs::path &report_path) {
  validate_new_output(report_path, "self-test report");
  ScopedCoutCapture capture;
  run_self_test();
  const std::string payload = capture.finish();
  try {
    reject_unexpected_self_test_payload(payload);
  } catch (const std::exception &error) {
    reveal_and_rethrow_payload_error(payload, error);
  }

  ExclusiveOutput report(report_path);
  report.write_all(payload);
  report.finish_io();
  report.commit();
}

[[nodiscard]] bool mentions_self_test_flag(int argc, char **argv) {
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    if (argument == "--self-test" || argument == "--self-test-report") {
      return true;
    }
  }
  return false;
}

} // namespace

int main(int argc, char **argv) {
  const bool exact_self_test =
      argc == 4 && std::string_view(argv[1]) == "--self-test" &&
      std::string_view(argv[2]) == "--self-test-report";
  if (!exact_self_test) {
    if (mentions_self_test_flag(argc, argv)) {
      std::cerr << "corrected-control raw NodeLift edge feature probe capture: "
                << "self-test requires exactly " << kSelfTestUsage << '\n';
      return 1;
    }
    return corrected_control_capture_predecessor_main(argc, argv);
  }

  try {
    std::locale::global(std::locale::classic());
    fs::path report_path(argv[3]);
    if (!report_path.is_absolute() ||
        report_path.lexically_normal() != report_path ||
        report_path.string().find_first_of("\r\n") != std::string::npos) {
      throw std::runtime_error(
          "self-test report must be an absolute lexically-clean path");
    }
    run_self_test_to_report(report_path);
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "corrected-control raw NodeLift edge feature probe capture: "
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "corrected-control raw NodeLift edge feature probe capture: "
              << error.what() << '\n';
  }
  return 1;
}
