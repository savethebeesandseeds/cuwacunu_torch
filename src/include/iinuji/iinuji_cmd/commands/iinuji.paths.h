#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "iinuji/iinuji_cmd/commands/iinuji.path.tokens.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
namespace canonical_paths {

enum class CallId {
#define IINUJI_CANONICAL_CALL(ID, TEXT, SUMMARY) ID,
#include "iinuji/iinuji_cmd/commands/iinuji.paths.def"
#undef IINUJI_CANONICAL_CALL
};

#define IINUJI_CANONICAL_CALL(ID, TEXT, SUMMARY) inline constexpr std::string_view k##ID = TEXT;
#include "iinuji/iinuji_cmd/commands/iinuji.paths.def"
#undef IINUJI_CANONICAL_CALL

inline constexpr std::string_view to_text(CallId id) {
  switch (id) {
#define IINUJI_CANONICAL_CALL(ID, TEXT, SUMMARY) \
  case CallId::ID:                           \
    return k##ID;
#include "iinuji/iinuji_cmd/commands/iinuji.paths.def"
#undef IINUJI_CANONICAL_CALL
  }
  return {};
}

inline constexpr std::string_view summary(CallId id) {
  switch (id) {
#define IINUJI_CANONICAL_CALL(ID, TEXT, SUMMARY) \
  case CallId::ID:                                    \
    return SUMMARY;
#include "iinuji/iinuji_cmd/commands/iinuji.paths.def"
#undef IINUJI_CANONICAL_CALL
  }
  return {};
}

inline const std::unordered_map<std::string_view, CallId>& call_map() {
  static const std::unordered_map<std::string_view, CallId> kMap = {
#define IINUJI_CANONICAL_CALL(ID, TEXT, SUMMARY) {k##ID, CallId::ID},
#include "iinuji/iinuji_cmd/commands/iinuji.paths.def"
#undef IINUJI_CANONICAL_CALL
  };
  return kMap;
}

inline const std::unordered_map<std::string_view, CallId>& alias_map() {
  auto add_aliases = [](std::unordered_map<std::string_view, CallId>& map,
                        CallId id,
                        auto... aliases) {
    (map.emplace(std::string_view(aliases), id), ...);
  };
  static const std::unordered_map<std::string_view, CallId> kMap = [&]() {
    std::unordered_map<std::string_view, CallId> map;
#define IINUJI_CANONICAL_CALL(ID, TEXT, SUMMARY)
#define IINUJI_CANONICAL_ALIAS(ID, ...) add_aliases(map, CallId::ID, __VA_ARGS__);
#include "iinuji/iinuji_cmd/commands/iinuji.paths.def"
#undef IINUJI_CANONICAL_ALIAS
#undef IINUJI_CANONICAL_CALL
    return map;
  }();
  return kMap;
}

inline const std::vector<std::pair<std::string_view, std::string_view>>& help_entries() {
  static const std::vector<std::pair<std::string_view, std::string_view>> kEntries = {
#define IINUJI_CANONICAL_CALL(ID, TEXT, SUMMARY) {TEXT, SUMMARY},
#include "iinuji/iinuji_cmd/commands/iinuji.paths.def"
#undef IINUJI_CANONICAL_CALL
  };
  return kEntries;
}

inline const std::vector<std::pair<std::string_view, std::string_view>>& alias_entries() {
  auto append_aliases = [](std::vector<std::pair<std::string_view, std::string_view>>& out,
                           std::string_view canonical,
                           auto... aliases) {
    (out.emplace_back(std::string_view(aliases), canonical), ...);
  };
  static const std::vector<std::pair<std::string_view, std::string_view>> kEntries = [&]() {
    std::vector<std::pair<std::string_view, std::string_view>> out;
#define IINUJI_CANONICAL_CALL(ID, TEXT, SUMMARY)
#define IINUJI_CANONICAL_ALIAS(ID, ...) append_aliases(out, k##ID, __VA_ARGS__);
#include "iinuji/iinuji_cmd/commands/iinuji.paths.def"
#undef IINUJI_CANONICAL_ALIAS
#undef IINUJI_CANONICAL_CALL
    return out;
  }();
  return kEntries;
}

inline std::string build_board_select_index(std::size_t idx1) {
  return "iinuji.board.select.index." + canonical_path_tokens::make_index_atom(idx1) + "()";
}

inline std::string build_tsi_tab_index(std::size_t idx1) {
  return "iinuji.tsi.tab.index." + canonical_path_tokens::make_index_atom(idx1) + "()";
}

inline std::string build_tsi_tab_id(std::string_view value) {
  return "iinuji.tsi.tab.id." + canonical_path_tokens::to_atom(value) + "()";
}

inline std::string build_config_tab_index(std::size_t idx1) {
  return "iinuji.config.tab.index." + canonical_path_tokens::make_index_atom(idx1) + "()";
}

inline std::string build_config_tab_id(std::string_view value) {
  return "iinuji.config.tab.id." + canonical_path_tokens::to_atom(value) + "()";
}

inline std::string build_data_plot_mode(std::string_view mode) {
  if (mode == "seq") return std::string(kDataPlotModeSeq);
  if (mode == "future") return std::string(kDataPlotModeFuture);
  if (mode == "weight") return std::string(kDataPlotModeWeight);
  if (mode == "norm") return std::string(kDataPlotModeNorm);
  if (mode == "bytes") return std::string(kDataPlotModeBytes);
  return "iinuji.view.data.plot(mode=" + std::string(mode) + ")";
}

inline std::string build_data_plot_view(std::string_view view) {
  if (view == "on") return std::string(kDataPlotOn);
  if (view == "off") return std::string(kDataPlotOff);
  if (view == "toggle") return std::string(kDataPlotToggle);
  return "iinuji.view.data.plot(view=" + std::string(view) + ")";
}

inline std::string build_data_x(std::string_view axis) {
  if (axis == "toggle") return std::string(kDataAxisToggle);
  if (axis == "idx" || axis == "index") return std::string(kDataAxisIdx);
  if (axis == "key" || axis == "keyvalue") return std::string(kDataAxisKey);
  return "iinuji.data.x(axis=" + std::string(axis) + ")";
}

inline std::string build_data_mask(std::string_view view) {
  if (view == "on") return std::string(kDataMaskOn);
  if (view == "off") return std::string(kDataMaskOff);
  if (view == "toggle") return std::string(kDataMaskToggle);
  return "iinuji.data.mask(view=" + std::string(view) + ")";
}

inline std::string build_data_ch_index(std::size_t idx1) {
  return "iinuji.data.ch.index." + canonical_path_tokens::make_index_atom(idx1) + "()";
}

inline std::string build_data_sample_index(std::size_t idx1) {
  return "iinuji.data.sample.index." + canonical_path_tokens::make_index_atom(idx1) + "()";
}

inline std::string build_data_dim_index(std::size_t idx1) {
  return "iinuji.data.dim.index." + canonical_path_tokens::make_index_atom(idx1) + "()";
}

inline std::string build_data_dim_id(std::string_view value) {
  return "iinuji.data.dim.id." + canonical_path_tokens::to_atom(value) + "()";
}

}  // namespace canonical_paths
}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
