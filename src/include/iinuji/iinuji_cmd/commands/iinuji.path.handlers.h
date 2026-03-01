#pragma once

#include <array>
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "camahjucunu/dsl/canonical_path/canonical_path.h"

#include "iinuji/iinuji_cmd/commands/iinuji.paths.h"
#include "iinuji/iinuji_cmd/commands/iinuji.screen.h"
#include "iinuji/iinuji_cmd/commands/iinuji.state.flow.h"
#include "iinuji/iinuji_cmd/views/board/commands.h"
#include "iinuji/iinuji_cmd/views/config/commands.h"
#include "iinuji/iinuji_cmd/views/data/commands.h"
#include "iinuji/iinuji_cmd/views/training/commands.h"
#include "iinuji/iinuji_cmd/views/tsiemene/commands.h"
#include "tsiemene/tsi.source.dataloader.h"
#include "tsiemene/tsi.wikimyei.representation.vicreg.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

template <std::size_t N>
[[nodiscard]] constexpr bool unique_dynamic_pattern_ids_constexpr(
    const std::array<canonical_paths::PatternId, N>& ids) {
  for (std::size_t i = 0; i < N; ++i) {
    for (std::size_t j = i + 1; j < N; ++j) {
      if (ids[i] == ids[j]) return false;
    }
  }
  return true;
}

struct IinujiPathHandlers {
  using CallHandlerId = canonical_paths::CallId;

  CmdState& state;
  IinujiScreen screen{state};
  IinujiStateFlow state_flow{state};

  [[nodiscard]] static const std::unordered_map<std::string_view, CallHandlerId>& call_handlers() {
    return canonical_paths::call_map();
  }

  struct DynamicPatternSpec {
    canonical_paths::PatternId id{canonical_paths::PatternId::Count};
    canonical_paths::PatternMatchStyle match_style{canonical_paths::PatternMatchStyle::ExactSegments};
    std::vector<std::string> prefix_segments{};
  };

  inline static constexpr auto kDynamicPatternIds = std::to_array<canonical_paths::PatternId>({
#define IINUJI_CANONICAL_CALL(ID, TEXT, SUMMARY)
#define IINUJI_CANONICAL_PATTERN(ID, TEXT, SUMMARY, MATCH_STYLE) canonical_paths::PatternId::ID,
#include "iinuji/iinuji_cmd/commands/iinuji.paths.def"
#undef IINUJI_CANONICAL_PATTERN
#undef IINUJI_CANONICAL_CALL
  });

  static_assert(
      kDynamicPatternIds.size() == canonical_paths::pattern_count(),
      "iinuji dynamic pattern routing list must cover every IINUJI_CANONICAL_PATTERN exactly once");
  static_assert(
      unique_dynamic_pattern_ids_constexpr(kDynamicPatternIds),
      "iinuji dynamic pattern routing list has duplicate PatternId entries");

  [[nodiscard]] static const std::array<DynamicPatternSpec, canonical_paths::pattern_count()>&
  dynamic_pattern_specs() {
    static const std::array<DynamicPatternSpec, canonical_paths::pattern_count()> kSpecs = []() {
      std::array<DynamicPatternSpec, canonical_paths::pattern_count()> specs{};
      for (std::size_t i = 0; i < specs.size(); ++i) {
        const auto id = static_cast<canonical_paths::PatternId>(i);
        const auto text = canonical_paths::pattern_text(id);
        auto decoded = cuwacunu::camahjucunu::decode_canonical_path(
            std::string(text));
        if (!decoded.ok) {
          throw std::logic_error(
              "invalid dynamic pattern in iinuji.paths.def: " + std::string(text) +
              " error=" + decoded.error);
        }
        if (decoded.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Call) {
          throw std::logic_error(
              "dynamic pattern must be a call path in iinuji.paths.def: " + std::string(text));
        }
        DynamicPatternSpec spec{};
        spec.id = id;
        spec.match_style = canonical_paths::pattern_match_style(id);
        spec.prefix_segments = std::move(decoded.segments);
        if (spec.match_style != canonical_paths::PatternMatchStyle::ExactSegments) {
          if (spec.prefix_segments.empty()) {
            throw std::logic_error(
                "dynamic pattern requires tail segment in iinuji.paths.def: " + std::string(text));
          }
          spec.prefix_segments.pop_back();
        }
        specs[i] = std::move(spec);
      }
      return specs;
    }();
    return kSpecs;
  }

  [[nodiscard]] static bool has_segments_prefix(const cuwacunu::camahjucunu::canonical_path_t& path,
                                                const std::vector<std::string>& prefix_segments) {
    if (path.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Call) return false;
    if (path.segments.size() < prefix_segments.size()) return false;
    for (std::size_t i = 0; i < prefix_segments.size(); ++i) {
      if (path.segments[i] != prefix_segments[i]) return false;
    }
    return true;
  }

  [[nodiscard]] static bool matches_dynamic_pattern(
      const cuwacunu::camahjucunu::canonical_path_t& path,
      const DynamicPatternSpec& spec) {
    if (!has_segments_prefix(path, spec.prefix_segments)) return false;
    const std::size_t prefix_size = spec.prefix_segments.size();
    switch (spec.match_style) {
      case canonical_paths::PatternMatchStyle::ExactSegments:
        return path.segments.size() == prefix_size;
      case canonical_paths::PatternMatchStyle::OptionalTailAtom:
        if (path.segments.size() == prefix_size) return true;
        return path.segments.size() == (prefix_size + 1) && path.args.empty();
      case canonical_paths::PatternMatchStyle::RequireArgsOrTailAtom:
        if (path.segments.size() == (prefix_size + 1) && path.args.empty()) return true;
        return path.segments.size() == prefix_size && !path.args.empty();
    }
    return false;
  }

  [[nodiscard]] static std::optional<canonical_paths::PatternId> match_dynamic_pattern_id(
      const cuwacunu::camahjucunu::canonical_path_t& path) {
    for (const auto& spec : dynamic_pattern_specs()) {
      if (matches_dynamic_pattern(path, spec)) return spec.id;
    }
    return std::nullopt;
  }

  [[nodiscard]] static bool parse_view_bool(std::string value, bool* out, bool* toggle) {
    if (!out || !toggle) return false;
    *toggle = false;
    value = to_lower_copy(value);
    if (value == "toggle") {
      *toggle = true;
      return true;
    }
    if (value == "on" || value == "true" || value == "1") {
      *out = true;
      return true;
    }
    if (value == "off" || value == "false" || value == "0") {
      *out = false;
      return true;
    }
    return false;
  }

  [[nodiscard]] static bool get_arg_value(const cuwacunu::camahjucunu::canonical_path_t& path,
                                          std::string_view key,
                                          std::string* out) {
    if (!out) return false;
    for (const auto& arg : path.args) {
      if (arg.key == key) {
        *out = arg.value;
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] static bool parse_positive_arg(const cuwacunu::camahjucunu::canonical_path_t& path,
                                               std::size_t* out) {
    if (!out) return false;
    std::string raw;
    if (get_arg_value(path, "n", &raw) || get_arg_value(path, "index", &raw) || get_arg_value(path, "value", &raw)) {
      return parse_positive_index(raw, out);
    }
    return false;
  }

  [[nodiscard]] static bool parse_string_arg(const cuwacunu::camahjucunu::canonical_path_t& path,
                                             std::string* out) {
    if (!out) return false;
    return get_arg_value(path, "value", out) || get_arg_value(path, "id", out);
  }

  [[nodiscard]] static bool parse_positive_arg_or_tail(const cuwacunu::camahjucunu::canonical_path_t& path,
                                                       std::size_t prefix_size,
                                                       std::size_t* out) {
    if (parse_positive_arg(path, out)) return true;
    if (!out) return false;
    if (!path.args.empty() || path.segments.size() != prefix_size + 1) return false;
    return canonical_path_tokens::parse_index_atom(path.segments.back(), out);
  }

  [[nodiscard]] static bool parse_string_arg_or_tail(const cuwacunu::camahjucunu::canonical_path_t& path,
                                                     std::size_t prefix_size,
                                                     std::string* out) {
    if (parse_string_arg(path, out)) return true;
    if (!out) return false;
    if (!path.args.empty() || path.segments.size() != prefix_size + 1) return false;
    *out = path.segments.back();
    return !out->empty();
  }

  [[nodiscard]] static int saturating_add_non_negative(int base, int delta) {
    using Limits = std::numeric_limits<int>;
    long long next = static_cast<long long>(base) + static_cast<long long>(delta);
    if (next < 0) return 0;
    if (next > static_cast<long long>(Limits::max())) return Limits::max();
    return static_cast<int>(next);
  }

  [[nodiscard]] static int saturating_add_signed(int base, int delta) {
    using Limits = std::numeric_limits<int>;
    long long next = static_cast<long long>(base) + static_cast<long long>(delta);
    if (next < static_cast<long long>(Limits::min())) return Limits::min();
    if (next > static_cast<long long>(Limits::max())) return Limits::max();
    return static_cast<int>(next);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_data_plot(const cuwacunu::camahjucunu::canonical_path_t& path,
                                  PushInfo&& push_info,
                                  PushWarn&&,
                                  PushErr&& push_err) const {
    return dispatch_data_plot_call(path, push_info, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_data_x(const cuwacunu::camahjucunu::canonical_path_t& path,
                               PushInfo&& push_info,
                               PushWarn&&,
                               PushErr&& push_err) const {
    return dispatch_data_x(path, push_info, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_data_mask(const cuwacunu::camahjucunu::canonical_path_t& path,
                                  PushInfo&& push_info,
                                  PushWarn&&,
                                  PushErr&& push_err) const {
    return dispatch_data_mask(path, push_info, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_data_ch_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                      PushInfo&& push_info,
                                      PushWarn&& push_warn,
                                      PushErr&& push_err) const {
    return dispatch_data_ch_index(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_data_sample_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                          PushInfo&& push_info,
                                          PushWarn&& push_warn,
                                          PushErr&& push_err) const {
    return dispatch_data_sample_index(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_data_dim_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                       PushInfo&& push_info,
                                       PushWarn&& push_warn,
                                       PushErr&& push_err) const {
    return dispatch_data_dim_index(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_data_dim_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                                    PushInfo&& push_info,
                                    PushWarn&& push_warn,
                                    PushErr&& push_err) const {
    return dispatch_data_dim_id(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_board_select_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                           PushInfo&& push_info,
                                           PushWarn&& push_warn,
                                           PushErr&& push_err) const {
    return dispatch_board_select_index(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_training_tab_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                           PushInfo&& push_info,
                                           PushWarn&& push_warn,
                                           PushErr&& push_err) const {
    return dispatch_training_tab_index(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_training_tab_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                                        PushInfo&& push_info,
                                        PushWarn&& push_warn,
                                        PushErr&& push_err) const {
    return dispatch_training_tab_id(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_training_hash_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                            PushInfo&& push_info,
                                            PushWarn&& push_warn,
                                            PushErr&& push_err) const {
    return dispatch_training_hash_index(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_training_hash_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                                         PushInfo&& push_info,
                                         PushWarn&& push_warn,
                                         PushErr&& push_err) const {
    return dispatch_training_hash_id(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_tsi_tab_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                      PushInfo&& push_info,
                                      PushWarn&& push_warn,
                                      PushErr&& push_err) const {
    return dispatch_tsi_tab_index(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_tsi_tab_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                                   PushInfo&& push_info,
                                   PushWarn&& push_warn,
                                   PushErr&& push_err) const {
    return dispatch_tsi_tab_id(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_tsi_dataloader_edit(const cuwacunu::camahjucunu::canonical_path_t& path,
                                            PushInfo&& push_info,
                                            PushWarn&& push_warn,
                                            PushErr&& push_err) const {
    return dispatch_tsi_dataloader_edit(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_tsi_dataloader_delete(const cuwacunu::camahjucunu::canonical_path_t& path,
                                              PushInfo&& push_info,
                                              PushWarn&& push_warn,
                                              PushErr&& push_err) const {
    return dispatch_tsi_dataloader_delete(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_config_tab_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                         PushInfo&& push_info,
                                         PushWarn&& push_warn,
                                         PushErr&& push_err) const {
    return dispatch_config_tab_index(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_pattern_config_tab_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                                      PushInfo&& push_info,
                                      PushWarn&& push_warn,
                                      PushErr&& push_err) const {
    return dispatch_config_tab_id(path, push_info, push_warn, push_err);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_text(const std::string& raw,
                     PushInfo&& push_info,
                     PushWarn&& push_warn,
                     PushErr&& push_err) const {
    auto append_noop = [](const std::string&, const std::string&, const std::string&) {};
    return dispatch_text(raw, push_info, push_warn, push_err, append_noop);
  }

  template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
  bool dispatch_text(const std::string& raw,
                     PushInfo&& push_info,
                     PushWarn&& push_warn,
                     PushErr&& push_err,
                     AppendLog&& append_log) const {
    if (raw.rfind("iinuji.", 0) != 0) return false;

    std::string normalized = raw;
    // UX shorthand: allow argumentless canonical calls without "()".
    if (normalized.find('(') == std::string::npos &&
        normalized.find('@') == std::string::npos) {
      normalized += "()";
    }

    if (state.board.contract_hash.empty()) {
      push_err("board contract hash is unavailable; reload board first");
      return true;
    }
    const std::string contract_hash = state.board.contract_hash;
    auto path =
        cuwacunu::camahjucunu::decode_canonical_path(normalized, contract_hash);
    if (!path.ok) {
      push_err("invalid iinuji path: " + path.error);
      return true;
    }
    std::string verror;
    if (!cuwacunu::camahjucunu::validate_canonical_path(path, &verror)) {
      push_err("invalid iinuji path: " + verror);
      return true;
    }
    if (dispatch(path, push_info, push_warn, push_err, append_log)) return true;

    push_err("unsupported iinuji call: " + path.canonical_identity);
    return true;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_canonical_text(const std::string& canonical_raw,
                               PushInfo&& push_info,
                               PushWarn&& push_warn,
                               PushErr&& push_err) const {
    auto append_noop = [](const std::string&, const std::string&, const std::string&) {};
    return dispatch_canonical_text(canonical_raw, push_info, push_warn, push_err, append_noop);
  }

  template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
  bool dispatch_canonical_text(const std::string& canonical_raw,
                               PushInfo&& push_info,
                               PushWarn&& push_warn,
                               PushErr&& push_err,
                               AppendLog&& append_log) const {
    if (canonical_raw.rfind("iinuji.", 0) != 0) {
      push_err("internal canonical call must start with iinuji.: " + canonical_raw);
      return false;
    }

    if (state.board.contract_hash.empty()) {
      push_err("board contract hash is unavailable; reload board first");
      return false;
    }
    const std::string contract_hash = state.board.contract_hash;
    auto path =
        cuwacunu::camahjucunu::decode_canonical_path(canonical_raw, contract_hash);
    if (!path.ok) {
      push_err("invalid canonical iinuji path: " + path.error);
      return false;
    }
    std::string verror;
    if (!cuwacunu::camahjucunu::validate_canonical_path(path, &verror)) {
      push_err("invalid canonical iinuji path: " + verror);
      return false;
    }
    if (path.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Call) {
      push_err("internal canonical call must be a call path: " + path.canonical);
      return false;
    }
    if (path.canonical != canonical_raw) {
      push_err("internal canonical call not exact: " + canonical_raw + " -> " + path.canonical);
      return false;
    }
    if (dispatch(path, push_info, push_warn, push_err, append_log)) return true;

    push_err("unsupported canonical iinuji call: " + path.canonical_identity);
    return false;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch(const cuwacunu::camahjucunu::canonical_path_t& path,
                PushInfo&& push_info,
                PushWarn&& push_warn,
                PushErr&& push_err) const {
    auto append_noop = [](const std::string&, const std::string&, const std::string&) {};
    return dispatch(path, push_info, push_warn, push_err, append_noop);
  }

  template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
  bool dispatch(const cuwacunu::camahjucunu::canonical_path_t& path,
                PushInfo&& push_info,
                PushWarn&& push_warn,
                PushErr&& push_err,
                AppendLog&& append_log) const {
    if (path.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Call) {
      push_err("iinuji terminal supports call paths only: " + path.canonical);
      return true;
    }

    if (const auto pattern_id = match_dynamic_pattern_id(path); pattern_id.has_value()) {
      switch (*pattern_id) {
        case canonical_paths::PatternId::DataPlotPattern:
        case canonical_paths::PatternId::DataPlotArgsModePattern:
        case canonical_paths::PatternId::DataPlotArgsViewPattern:
          return dispatch_pattern_data_plot(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::DataXPattern:
          return dispatch_pattern_data_x(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::DataMaskPattern:
          return dispatch_pattern_data_mask(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::DataChIndexPattern:
          return dispatch_pattern_data_ch_index(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::DataSampleIndexPattern:
          return dispatch_pattern_data_sample_index(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::DataDimIndexPattern:
          return dispatch_pattern_data_dim_index(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::DataDimIdPattern:
          return dispatch_pattern_data_dim_id(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::BoardSelectIndexPattern:
          return dispatch_pattern_board_select_index(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::TrainingTabIndexPattern:
          return dispatch_pattern_training_tab_index(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::TrainingTabIdPattern:
          return dispatch_pattern_training_tab_id(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::TrainingHashIndexPattern:
          return dispatch_pattern_training_hash_index(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::TrainingHashIdPattern:
          return dispatch_pattern_training_hash_id(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::TsiTabIndexPattern:
          return dispatch_pattern_tsi_tab_index(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::TsiTabIdPattern:
          return dispatch_pattern_tsi_tab_id(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::TsiDataloaderEditIdPattern:
          return dispatch_pattern_tsi_dataloader_edit(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::TsiDataloaderDeleteIdPattern:
          return dispatch_pattern_tsi_dataloader_delete(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::ConfigTabIndexPattern:
          return dispatch_pattern_config_tab_index(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::ConfigTabIdPattern:
          return dispatch_pattern_config_tab_id(path, push_info, push_warn, push_err);
        case canonical_paths::PatternId::Count:
          break;
      }
    }

    const auto it = call_handlers().find(path.canonical_identity);
    if (it == call_handlers().end()) return false;

    const CallHandlerId call_id = it->second;
    if (dispatch_core_call(call_id, push_info, push_warn, push_err, append_log)) return true;
    if (dispatch_logs_call(call_id, push_info, push_warn, push_err, append_log)) return true;
    if (dispatch_board_call(call_id, push_info, push_warn, push_err, append_log)) return true;
    if (dispatch_tsi_call(call_id, push_info, push_warn, push_err, append_log)) return true;
    if (dispatch_training_call(call_id, push_info, push_warn, push_err, append_log)) return true;
    if (dispatch_config_call(call_id, push_info, push_warn, push_err, append_log)) return true;
    if (dispatch_data_call(call_id, push_info, push_warn, push_err, append_log)) return true;

    push_warn("unhandled iinuji call");
    return true;
  }

#include "iinuji/iinuji_cmd/commands/handlers/iinuji.path.handler.core.h"
#include "iinuji/iinuji_cmd/commands/handlers/iinuji.path.handler.logs.h"
#include "iinuji/iinuji_cmd/commands/handlers/iinuji.path.handler.board.h"
#include "iinuji/iinuji_cmd/commands/handlers/iinuji.path.handler.tsi.h"
#include "iinuji/iinuji_cmd/commands/handlers/iinuji.path.handler.training.h"
#include "iinuji/iinuji_cmd/commands/handlers/iinuji.path.handler.config.h"
#include "iinuji/iinuji_cmd/commands/handlers/iinuji.path.handler.data.h"
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
