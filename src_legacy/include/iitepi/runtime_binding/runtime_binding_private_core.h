// ./include/iitepi/runtime_binding/runtime_binding.h
// SPDX-License-Identifier: MIT
#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "hero/hashimyei_hero/hashimyei_catalog.h"
#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "iitepi/contract_space_t.h"
#include "iitepi/runtime_binding/runtime_binding.contract.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"
#include "tsiemene/tsi.type.registry.h"
#include "tsiemene/tsi.wikimyei.h"

namespace tsiemene {

struct RuntimeBinding {
  std::string campaign_hash{};
  std::string runtime_binding_path{};
  std::string binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::vector<RuntimeBindingContract> contracts{};

  RuntimeBinding() = default;
  RuntimeBinding(RuntimeBinding &&other) noexcept
      : campaign_hash(std::move(other.campaign_hash)),
        runtime_binding_path(std::move(other.runtime_binding_path)),
        binding_id(std::move(other.binding_id)),
        contract_hash(std::move(other.contract_hash)),
        wave_hash(std::move(other.wave_hash)),
        contracts(std::move(other.contracts)) {}
  RuntimeBinding &operator=(RuntimeBinding &&other) noexcept {
    if (this != &other) {
      campaign_hash = std::move(other.campaign_hash);
      runtime_binding_path = std::move(other.runtime_binding_path);
      binding_id = std::move(other.binding_id);
      contract_hash = std::move(other.contract_hash);
      wave_hash = std::move(other.wave_hash);
      contracts = std::move(other.contracts);
    }
    return *this;
  }

  RuntimeBinding(const RuntimeBinding &) = delete;
  RuntimeBinding &operator=(const RuntimeBinding &) = delete;
};

struct runtime_binding_trace_event_t {
  std::string phase{};
  std::uint64_t timestamp_ms{0};
  std::string campaign_hash{};
  std::string binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::string contract_name{};
  std::string component_name{};
  std::string resolved_record_type{};
  std::string resolved_sampler{};
  std::string run_id{};
  std::string source_runtime_cursor{};
  std::uint64_t contract_index{0};
  std::uint64_t contract_count{0};
  std::uint64_t epochs{0};
  std::uint64_t epoch{0};
  std::uint64_t epoch_steps{0};
  std::uint64_t total_steps{0};
  std::uint64_t optimizer_steps{0};
  std::uint64_t trained_epochs{0};
  std::uint64_t trained_samples{0};
  std::uint64_t skipped_batches{0};
  double epoch_loss_mean{std::numeric_limits<double>::quiet_NaN()};
  double last_committed_loss_mean{std::numeric_limits<double>::quiet_NaN()};
  double loss_inv_mean{std::numeric_limits<double>::quiet_NaN()};
  double loss_var_mean{std::numeric_limits<double>::quiet_NaN()};
  double loss_cov_raw_mean{std::numeric_limits<double>::quiet_NaN()};
  double loss_cov_weighted_mean{std::numeric_limits<double>::quiet_NaN()};
  double learning_rate{std::numeric_limits<double>::quiet_NaN()};
  bool ok{false};
  std::string error{};
};

using runtime_binding_trace_emit_fn_t =
    void (*)(const runtime_binding_trace_event_t &, void *);

struct runtime_binding_trace_sink_t {
  runtime_binding_trace_emit_fn_t emit{nullptr};
  void *user{nullptr};
};

struct runtime_binding_trace_context_t {
  std::string campaign_hash{};
  std::string binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::string contract_name{};
  std::uint64_t contract_index{0};
  std::uint64_t contract_count{0};
  std::uint64_t epochs{0};
};

[[nodiscard]] inline runtime_binding_trace_sink_t &
runtime_binding_trace_sink_slot() {
  static thread_local runtime_binding_trace_sink_t sink{};
  return sink;
}

[[nodiscard]] inline runtime_binding_trace_sink_t
swap_runtime_binding_trace_sink(runtime_binding_trace_sink_t next) {
  auto &slot = runtime_binding_trace_sink_slot();
  runtime_binding_trace_sink_t previous = slot;
  slot = next;
  return previous;
}

struct runtime_binding_trace_scope_t {
  runtime_binding_trace_scope_t() = default;
  explicit runtime_binding_trace_scope_t(runtime_binding_trace_sink_t sink)
      : previous(swap_runtime_binding_trace_sink(sink)), active(true) {}
  runtime_binding_trace_scope_t(const runtime_binding_trace_scope_t &) = delete;
  runtime_binding_trace_scope_t &
  operator=(const runtime_binding_trace_scope_t &) = delete;
  runtime_binding_trace_scope_t(runtime_binding_trace_scope_t &&other) noexcept
      : previous(other.previous), active(other.active) {
    other.active = false;
  }
  runtime_binding_trace_scope_t &
  operator=(runtime_binding_trace_scope_t &&other) noexcept {
    if (this == &other)
      return *this;
    if (active)
      (void)swap_runtime_binding_trace_sink(previous);
    previous = other.previous;
    active = other.active;
    other.active = false;
    return *this;
  }
  ~runtime_binding_trace_scope_t() {
    if (active)
      (void)swap_runtime_binding_trace_sink(previous);
  }

  runtime_binding_trace_sink_t previous{};
  bool active{false};
};

inline void
emit_runtime_binding_trace_event(runtime_binding_trace_event_t event) {
  if (event.timestamp_ms == 0) {
    event.timestamp_ms = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
  }
  const auto &sink = runtime_binding_trace_sink_slot();
  if (!sink.emit)
    return;
  sink.emit(event, sink.user);
}

[[nodiscard]] inline DirectiveId
pick_start_directive(const Circuit &c) noexcept {
  if (!c.hops || c.hop_count == 0 || !c.hops[0].from.tsi) {
    return directive_id::Step;
  }

  const Tsi &t = *c.hops[0].from.tsi;
  for (const auto &d : t.directives()) {
    if (d.dir == DirectiveDir::In && d.kind.kind == PayloadKind::String)
      return d.id;
  }
  for (const auto &d : t.directives()) {
    if (d.dir == DirectiveDir::In)
      return d.id;
  }
  return directive_id::Step;
}

[[nodiscard]] inline std::string
make_runtime_run_id(const RuntimeBindingContract &contract) {
  const std::uint64_t now_ms = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  std::ostringstream out;
  out << "run."
      << (contract.spec.contract_hash.empty() ? "unknown"
                                              : contract.spec.contract_hash)
      << "." << now_ms;
  return out.str();
}

[[nodiscard]] inline std::string lowercase_copy(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return s;
}

[[nodiscard]] inline std::string
compact_runtime_source_path_label(std::string_view raw_path) {
  std::string trimmed_input(raw_path);
  const auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };
  trimmed_input.erase(
      trimmed_input.begin(),
      std::find_if(trimmed_input.begin(), trimmed_input.end(), not_space));
  trimmed_input.erase(
      std::find_if(trimmed_input.rbegin(), trimmed_input.rend(), not_space)
          .base(),
      trimmed_input.end());
  const std::string trimmed = lowercase_copy(trimmed_input);
  const std::filesystem::path path(trimmed_input);
  const std::string filename = path.filename().string();
  if (filename.empty())
    return path.lexically_normal().string();
  if (trimmed.find("/.campaigns/") != std::string::npos) {
    return filename;
  }
  if (trimmed.find("/tmp/iitepi.internal.") != std::string::npos &&
      filename.find(".runtime_binding.dsl") != std::string::npos) {
    return "iitepi.runtime_binding.dsl";
  }
  return path.lexically_normal().string();
}

[[nodiscard]] inline std::uint64_t now_ms_utc_runtime_binding() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

[[nodiscard]] inline bool
parse_runtime_run_started_at_ms(std::string_view run_id, std::uint64_t *out) {
  if (!out)
    return false;
  const std::size_t dot = run_id.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= run_id.size())
    return false;
  std::uint64_t value = 0;
  const auto *begin = run_id.data() + dot + 1;
  const auto *end = run_id.data() + run_id.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end)
    return false;
  *out = value;
  return true;
}

inline bool ensure_runtime_context_wave_cursor(RuntimeContext *ctx) {
  if (!ctx)
    return false;
  if (ctx->has_wave_cursor)
    return true;
  std::uint64_t run_started_at_ms = 0;
  if (!parse_runtime_run_started_at_ms(ctx->run_id, &run_started_at_ms)) {
    return false;
  }
  std::uint64_t packed_wave_cursor = 0;
  if (!cuwacunu::piaabo::latent_lineage_state::pack_runtime_wave_cursor(
          run_started_at_ms, 0, 0, &packed_wave_cursor)) {
    return false;
  }
  ctx->has_wave_cursor = true;
  ctx->wave_cursor = packed_wave_cursor;
  return true;
}
