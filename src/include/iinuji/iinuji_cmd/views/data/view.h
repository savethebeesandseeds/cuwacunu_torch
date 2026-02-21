#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"
#include "camahjucunu/types/types_enums.h"

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline const std::vector<std::string>& data_feature_names_for_record_type(std::string_view record_type) {
  static const std::vector<std::string> kline = {
      "open_price",
      "high_price",
      "low_price",
      "close_price",
      "volume",
      "quote_asset_volume",
      "number_of_trades",
      "taker_buy_base_volume",
      "taker_buy_quote_volume",
  };
  static const std::vector<std::string> trade = {
      "price",
      "qty",
      "quoteQty",
      "isBuyerMaker",
      "isBestMatch",
  };
  static const std::vector<std::string> basic = {"value"};
  static const std::vector<std::string> empty{};

  if (record_type == "kline") return kline;
  if (record_type == "trade") return trade;
  if (record_type == "basic") return basic;
  return empty;
}

inline const std::vector<std::string>& data_feature_types_for_record_type(std::string_view record_type) {
  static const std::vector<std::string> kline = {
      "double",
      "double",
      "double",
      "double",
      "double",
      "double",
      "int32->double",
      "double",
      "double",
  };
  static const std::vector<std::string> trade = {
      "double",
      "double",
      "double",
      "bool->double",
      "bool->double",
  };
  static const std::vector<std::string> basic = {"double"};
  static const std::vector<std::string> empty{};

  if (record_type == "kline") return kline;
  if (record_type == "trade") return trade;
  if (record_type == "basic") return basic;
  return empty;
}

inline std::string data_feature_name_for_channel_dim(const DataChannelView& c, std::size_t dim_index) {
  const auto& names = data_feature_names_for_record_type(c.record_type);
  if (dim_index < names.size()) return names[dim_index];
  return "x" + std::to_string(dim_index);
}

inline std::string data_feature_type_for_channel_dim(const DataChannelView& c, std::size_t dim_index) {
  const auto& dtypes = data_feature_types_for_record_type(c.record_type);
  if (dim_index < dtypes.size()) return dtypes[dim_index];
  return "double";
}

inline std::string data_plot_mode_name(DataPlotMode mode) {
  switch (mode) {
    case DataPlotMode::SeqLength: return "seq_length";
    case DataPlotMode::FutureSeqLength: return "future_seq_length";
    case DataPlotMode::ChannelWeight: return "channel_weight";
    case DataPlotMode::NormWindow: return "norm_window";
    case DataPlotMode::FileBytes: return "file_bytes";
  }
  return "seq_length";
}

inline std::string data_plot_mode_token(DataPlotMode mode) {
  switch (mode) {
    case DataPlotMode::SeqLength: return "seq";
    case DataPlotMode::FutureSeqLength: return "future";
    case DataPlotMode::ChannelWeight: return "weight";
    case DataPlotMode::NormWindow: return "norm";
    case DataPlotMode::FileBytes: return "bytes";
  }
  return "seq";
}

inline std::string data_plot_mode_description(DataPlotMode mode) {
  switch (mode) {
    case DataPlotMode::SeqLength:
      return "past sequence values (features over T)";
    case DataPlotMode::FutureSeqLength:
      return "future sequence values (future features over Hf)";
    case DataPlotMode::ChannelWeight:
      return "configured channel_weight per active channel";
    case DataPlotMode::NormWindow:
      return "configured norm_window per active channel";
    case DataPlotMode::FileBytes:
      return "resolved data footprint (norm.bin > raw.bin > csv)";
  }
  return "past sequence values (features over T)";
}

inline bool data_plot_mode_is_dynamic(DataPlotMode mode) {
  return mode == DataPlotMode::SeqLength || mode == DataPlotMode::FutureSeqLength;
}

inline std::string data_plot_x_axis_name(DataPlotXAxis axis) {
  switch (axis) {
    case DataPlotXAxis::Index: return "index";
    case DataPlotXAxis::KeyValue: return "key_value";
  }
  return "index";
}

inline std::string data_plot_x_axis_token(DataPlotXAxis axis) {
  switch (axis) {
    case DataPlotXAxis::Index: return "idx";
    case DataPlotXAxis::KeyValue: return "key";
  }
  return "idx";
}

inline DataPlotXAxis next_data_plot_x_axis(DataPlotXAxis axis) {
  const std::size_t n = data_plot_x_axis_count();
  const auto idx = static_cast<std::size_t>(axis);
  return static_cast<DataPlotXAxis>((idx + 1) % std::max<std::size_t>(n, 1));
}

inline std::optional<DataPlotXAxis> parse_data_plot_x_axis_token(std::string token) {
  token = to_lower_copy(token);
  if (token == "idx" || token == "index" || token == "i") return DataPlotXAxis::Index;
  if (token == "key" || token == "k" || token == "key_value" || token == "keyvalue") {
    return DataPlotXAxis::KeyValue;
  }
  return std::nullopt;
}

inline std::string data_nav_focus_name(DataNavFocus focus) {
  switch (focus) {
    case DataNavFocus::Channel: return "channel";
    case DataNavFocus::Sample: return "sample";
    case DataNavFocus::Dim: return "dim";
    case DataNavFocus::PlotMode: return "plot";
    case DataNavFocus::XAxis: return "x-axis";
    case DataNavFocus::Mask: return "mask";
  }
  return "channel";
}

inline DataPlotMode next_data_plot_mode(DataPlotMode mode) {
  const std::size_t n = data_plot_mode_count();
  const auto idx = static_cast<std::size_t>(mode);
  return static_cast<DataPlotMode>((idx + 1) % n);
}

inline DataPlotMode prev_data_plot_mode(DataPlotMode mode) {
  const std::size_t n = data_plot_mode_count();
  const auto idx = static_cast<std::size_t>(mode);
  return static_cast<DataPlotMode>((idx + n - 1) % n);
}

inline std::optional<DataPlotMode> parse_data_plot_mode_token(std::string token) {
  token = to_lower_copy(token);
  if (token == "seq" || token == "seqlen" || token == "seq_length") {
    return DataPlotMode::SeqLength;
  }
  if (token == "future" || token == "future_seq" || token == "future_seq_length") {
    return DataPlotMode::FutureSeqLength;
  }
  if (token == "weight" || token == "channel_weight") {
    return DataPlotMode::ChannelWeight;
  }
  if (token == "norm" || token == "norm_window") {
    return DataPlotMode::NormWindow;
  }
  if (token == "bytes" || token == "file" || token == "size") {
    return DataPlotMode::FileBytes;
  }
  return std::nullopt;
}

inline bool parse_size_t_value(std::string_view text, std::size_t* out) {
  if (!out) return false;
  try {
    const auto v = std::stoull(std::string(text));
    *out = static_cast<std::size_t>(v);
    return true;
  } catch (...) {
    return false;
  }
}

inline double parse_double_value(std::string_view text, double fallback = 0.0) {
  try {
    return std::stod(std::string(text));
  } catch (...) {
    return fallback;
  }
}

inline std::string raw_bin_for_source(const std::string& source_csv) {
  std::filesystem::path p(source_csv);
  p.replace_extension(".bin");
  return p.string();
}

inline std::string norm_bin_for_source(const std::string& source_csv, std::size_t norm_window) {
  if (norm_window == 0) return {};
  std::filesystem::path p(source_csv);
  const std::string stem = p.stem().string();
  const std::string name = stem + ".normW" + std::to_string(norm_window) + ".bin";
  return (p.parent_path() / name).string();
}

inline void probe_file(const std::string& path, bool* exists, std::uintmax_t* bytes) {
  if (exists) *exists = false;
  if (bytes) *bytes = 0;
  if (path.empty()) return;

  std::error_code ec;
  if (!std::filesystem::exists(path, ec) || ec) return;
  if (!std::filesystem::is_regular_file(path, ec) || ec) return;

  if (exists) *exists = true;
  if (bytes) {
    const auto sz = std::filesystem::file_size(path, ec);
    if (!ec) *bytes = sz;
  }
}

inline std::size_t feature_dims_for_record_type(std::string_view record_type) {
  if (record_type == "kline") return 9;
  if (record_type == "trade") return 5;
  if (record_type == "basic") return 1;
  return 0;
}

inline std::string data_focus_instrument(const BoardState* board_view,
                                         const cuwacunu::camahjucunu::observation_instruction_t& obs) {
  if (board_view && board_view->ok && !board_view->board.circuits.empty()) {
    const std::string from_board =
        cuwacunu::camahjucunu::circuit_invoke_symbol(board_view->board.circuits.front());
    if (!from_board.empty()) return from_board;
  }
  if (!obs.instrument_forms.empty()) return obs.instrument_forms.front().instrument;
  return {};
}

inline std::string format_bytes_approx(std::uintmax_t bytes) {
  static constexpr const char* kUnits[] = {"B", "KB", "MB", "GB", "TB"};
  double val = static_cast<double>(bytes);
  std::size_t ui = 0;
  while (val >= 1024.0 && ui + 1 < (sizeof(kUnits) / sizeof(kUnits[0]))) {
    val /= 1024.0;
    ++ui;
  }
  std::ostringstream oss;
  if (ui == 0) oss << static_cast<std::uintmax_t>(val) << " " << kUnits[ui];
  else oss << std::fixed << std::setprecision(2) << val << " " << kUnits[ui];
  return oss.str();
}

inline DataState load_data_view_from_config(const BoardState* board_view = nullptr) {
  DataState out{};
  out.batch_size = static_cast<std::size_t>(cuwacunu::piaabo::dconfig::config_space_t::get<int>(
      "DATA_LOADER", "dataloader_batch_size", std::optional<int>(64)));
  out.raw_instruction = cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction();

  cuwacunu::camahjucunu::observation_instruction_t obs{};
  try {
    auto parser = cuwacunu::camahjucunu::BNF::observationPipeline();
    obs = parser.decode(out.raw_instruction);
  } catch (const std::exception& e) {
    out.ok = false;
    out.error = std::string("decode failed: ") + e.what();
    return out;
  } catch (...) {
    out.ok = false;
    out.error = "decode failed";
    return out;
  }

  out.focus_instrument = data_focus_instrument(board_view, obs);

  std::set<std::size_t> dims_set;
  for (const auto& in_form : obs.input_forms) {
    if (to_lower_copy(in_form.active) != "true") continue;

    std::size_t seq_length = 0;
    std::size_t future_seq_length = 0;
    parse_size_t_value(in_form.seq_length, &seq_length);
    parse_size_t_value(in_form.future_seq_length, &future_seq_length);
    const double channel_weight = parse_double_value(in_form.channel_weight, 0.0);

    bool matched_focus = false;
    for (const auto& instr_form : obs.instrument_forms) {
      if (instr_form.record_type != in_form.record_type) continue;
      if (instr_form.interval != in_form.interval) continue;
      if (!out.focus_instrument.empty() && instr_form.instrument != out.focus_instrument) continue;

      DataChannelView v{};
      v.instrument = instr_form.instrument;
      v.interval = cuwacunu::camahjucunu::exchange::enum_to_string(instr_form.interval);
      v.record_type = in_form.record_type;
      v.seq_length = seq_length;
      v.future_seq_length = future_seq_length;
      v.channel_weight = channel_weight;
      parse_size_t_value(instr_form.norm_window, &v.norm_window);
      v.feature_dims = feature_dims_for_record_type(v.record_type);
      v.from_focus_instrument = true;

      v.csv_path = instr_form.source;
      v.raw_bin_path = raw_bin_for_source(v.csv_path);
      v.norm_bin_path = norm_bin_for_source(v.csv_path, v.norm_window);

      probe_file(v.csv_path, &v.csv_exists, &v.csv_bytes);
      probe_file(v.raw_bin_path, &v.raw_bin_exists, &v.raw_bin_bytes);
      probe_file(v.norm_bin_path, &v.norm_bin_exists, &v.norm_bin_bytes);

      if (v.csv_exists) ++out.csv_present;
      if (v.raw_bin_exists) ++out.raw_bin_present;
      if (!v.norm_bin_path.empty() && v.norm_bin_exists) ++out.norm_bin_present;

      if (v.feature_dims > 0) dims_set.insert(v.feature_dims);
      out.max_seq_length = std::max(out.max_seq_length, v.seq_length);
      out.max_future_seq_length = std::max(out.max_future_seq_length, v.future_seq_length);

      out.channels.push_back(std::move(v));
      matched_focus = true;
    }

    if (matched_focus || out.focus_instrument.empty()) continue;

    // Fallback only when no board-focused source was found for this active channel.
    for (const auto& instr_form : obs.instrument_forms) {
      if (instr_form.record_type != in_form.record_type) continue;
      if (instr_form.interval != in_form.interval) continue;

      DataChannelView v{};
      v.instrument = instr_form.instrument;
      v.interval = cuwacunu::camahjucunu::exchange::enum_to_string(instr_form.interval);
      v.record_type = in_form.record_type;
      v.seq_length = seq_length;
      v.future_seq_length = future_seq_length;
      v.channel_weight = channel_weight;
      parse_size_t_value(instr_form.norm_window, &v.norm_window);
      v.feature_dims = feature_dims_for_record_type(v.record_type);
      v.from_focus_instrument = false;

      v.csv_path = instr_form.source;
      v.raw_bin_path = raw_bin_for_source(v.csv_path);
      v.norm_bin_path = norm_bin_for_source(v.csv_path, v.norm_window);

      probe_file(v.csv_path, &v.csv_exists, &v.csv_bytes);
      probe_file(v.raw_bin_path, &v.raw_bin_exists, &v.raw_bin_bytes);
      probe_file(v.norm_bin_path, &v.norm_bin_exists, &v.norm_bin_bytes);

      if (v.csv_exists) ++out.csv_present;
      if (v.raw_bin_exists) ++out.raw_bin_present;
      if (!v.norm_bin_path.empty() && v.norm_bin_exists) ++out.norm_bin_present;

      if (v.feature_dims > 0) dims_set.insert(v.feature_dims);
      out.max_seq_length = std::max(out.max_seq_length, v.seq_length);
      out.max_future_seq_length = std::max(out.max_future_seq_length, v.future_seq_length);

      out.channels.push_back(std::move(v));
    }
  }

  out.active_channels = out.channels.size();
  out.mixed_feature_dims = (dims_set.size() > 1);
  if (!dims_set.empty()) out.feature_dims = *dims_set.begin();
  out.ok = true;
  if (out.channels.empty()) {
    out.error = "no active channels resolved from observation pipeline";
  }
  return out;
}

inline std::string bar_for_value(double v, double vmax, int width = 28) {
  if (width <= 0) return {};
  if (!(vmax > 0.0)) return std::string(static_cast<std::size_t>(width), '.');
  double ratio = v / vmax;
  if (!std::isfinite(ratio)) ratio = 0.0;
  ratio = std::max(0.0, std::min(1.0, ratio));
  const int filled = static_cast<int>(std::llround(ratio * static_cast<double>(width)));
  return std::string(static_cast<std::size_t>(std::max(0, filled)), '#') +
         std::string(static_cast<std::size_t>(std::max(0, width - filled)), '.');
}

inline double plot_value_for_channel(const DataChannelView& c, DataPlotMode mode) {
  switch (mode) {
    case DataPlotMode::SeqLength: return static_cast<double>(c.seq_length);
    case DataPlotMode::FutureSeqLength: return static_cast<double>(c.future_seq_length);
    case DataPlotMode::ChannelWeight: return c.channel_weight;
    case DataPlotMode::NormWindow: return static_cast<double>(c.norm_window);
    case DataPlotMode::FileBytes: {
      if (c.norm_window > 0 && c.norm_bin_exists) return static_cast<double>(c.norm_bin_bytes);
      if (c.raw_bin_exists) return static_cast<double>(c.raw_bin_bytes);
      return static_cast<double>(c.csv_bytes);
    }
  }
  return 0.0;
}

inline std::string format_plot_value(double value, DataPlotMode mode) {
  if (mode == DataPlotMode::ChannelWeight) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << value;
    return oss.str();
  }
  if (mode == DataPlotMode::FileBytes) {
    return format_bytes_approx(static_cast<std::uintmax_t>(std::max(0.0, value)));
  }
  std::ostringstream oss;
  oss << static_cast<std::uintmax_t>(std::max(0.0, value));
  return oss.str();
}

inline std::string make_data_plot(const DataState& dv, int width = 22, bool focus_view = false) {
  std::ostringstream oss;
  if (dv.channels.empty()) {
    oss << "(no channels)\n";
    return oss.str();
  }

  const DataPlotMode mode = dv.plot_mode;
  const std::size_t sel = std::min(dv.selected_channel, dv.channels.size() - 1);
  const bool dynamic_mode = data_plot_mode_is_dynamic(mode);

  if (!dynamic_mode) {
    if (focus_view) {
      oss << "STATIC VIEW : " << data_plot_mode_name(mode)
          << " (token='" << data_plot_mode_token(mode) << "')\n";
    } else {
      oss << "Static values: " << data_plot_mode_name(mode)
          << " (token='" << data_plot_mode_token(mode) << "')\n";
    }
    for (std::size_t i = 0; i < dv.channels.size(); ++i) {
      const auto& c = dv.channels[i];
      const double v = plot_value_for_channel(c, mode);
      oss << (i == sel ? " >" : "  ")
          << "[" << (i + 1) << "] "
          << (c.from_focus_instrument ? "*" : " ")
          << c.interval << "/" << c.record_type
          << " value=" << format_plot_value(v, mode)
          << "\n";
    }
    return oss.str();
  }

  double vmax = 0.0;
  for (const auto& c : dv.channels) vmax = std::max(vmax, plot_value_for_channel(c, mode));

  if (focus_view) {
    oss << "PLOT VIEW : " << data_plot_mode_name(mode)
        << " (token='" << data_plot_mode_token(mode) << "')\n";
    oss << "x-axis=" << data_plot_x_axis_name(dv.plot_x_axis)
        << " (" << data_plot_x_axis_token(dv.plot_x_axis) << ")\n";
    oss << "selected channel=" << (sel + 1) << "/" << dv.channels.size() << "\n";
  } else {
    oss << "Plot: " << data_plot_mode_name(mode)
        << " (token='" << data_plot_mode_token(mode) << "')"
        << " x-axis=" << data_plot_x_axis_token(dv.plot_x_axis) << "\n";
  }
  for (std::size_t i = 0; i < dv.channels.size(); ++i) {
    const auto& c = dv.channels[i];
    const double v = plot_value_for_channel(c, mode);
    oss << (i == sel ? " >" : "  ")
        << "[" << (i + 1) << "] "
        << (c.from_focus_instrument ? "*" : " ")
        << c.interval << "/" << c.record_type
        << " |" << bar_for_value(v, vmax, width) << "| "
        << format_plot_value(v, mode)
        << "\n";
  }
  return oss.str();
}

inline std::string make_data_left(const CmdState& st) {
  if (!st.data.ok) {
    std::ostringstream oss;
    oss << "Observation data view invalid.\n\n";
    oss << "error: " << st.data.error << "\n\n";
    oss << "raw instruction:\n" << st.data.raw_instruction << "\n";
    return oss.str();
  }

  const bool has_channels = !st.data.channels.empty();
  const std::size_t idx = has_channels
                              ? std::min(st.data.selected_channel, st.data.channels.size() - 1)
                              : 0;

  std::ostringstream oss;
  oss << "Observation Data View\n";

  oss << "\nSummary\n";
  oss << "  focus instrument: " << (st.data.focus_instrument.empty() ? "<none>" : st.data.focus_instrument) << "\n";
  oss << "  active channels : " << st.data.active_channels << "\n";
  oss << "  batch size (B)  : " << st.data.batch_size << "\n";
  oss << "  max seq (T)     : " << st.data.max_seq_length << "\n";
  oss << "  max future (Hf) : " << st.data.max_future_seq_length << "\n";
  if (st.data.feature_dims == 0) {
    oss << "  feature dims (D): unknown\n";
  } else if (st.data.mixed_feature_dims) {
    oss << "  feature dims (D): mixed across record types\n";
  } else {
    oss << "  feature dims (D): " << st.data.feature_dims << "\n";
  }
  oss << "  tensor state    : " << (st.data.plot_tensor_ready ? "ready" : "pending") << "\n";
  if (!st.data.plot_tensor_error.empty()) {
    oss << "  tensor error    : " << st.data.plot_tensor_error << "\n";
  }
  oss << "  presence csv/raw/norm: "
      << st.data.csv_present << "/" << st.data.raw_bin_present << "/";
  std::size_t norm_expected = 0;
  for (const auto& c : st.data.channels) if (!c.norm_bin_path.empty()) ++norm_expected;
  oss << st.data.norm_bin_present << " (norm expected=" << norm_expected << ")\n";

  oss << "\nSelection Snapshot\n";
  oss << "  plot view   : " << (st.data.plot_view ? "on" : "off") << "\n";
  oss << "  plot mode   : " << data_plot_mode_token(st.data.plot_mode)
      << " (" << data_plot_mode_description(st.data.plot_mode) << ")\n";
  oss << "  x-axis      : " << data_plot_x_axis_name(st.data.plot_x_axis)
      << " (" << data_plot_x_axis_token(st.data.plot_x_axis) << ")\n";
  oss << "  nav focus   : " << data_nav_focus_name(st.data.nav_focus) << "\n";
  oss << "  plot sample : ";
  if (st.data.plot_sample_count == 0) oss << "n/a\n";
  else oss << (st.data.plot_sample_index + 1) << "/" << st.data.plot_sample_count << "\n";
  oss << "  plot dim (D): ";
  if (st.data.plot_D == 0) oss << "n/a\n";
  else oss << (st.data.plot_feature_dim + 1) << "/" << st.data.plot_D << "\n";
  oss << "  mask overlay: " << (st.data.plot_mask_overlay ? "on" : "off") << "\n";

  if (!st.data.mixed_feature_dims && st.data.feature_dims > 0 && st.data.active_channels > 0) {
    oss << "\nExpected tensor shapes\n";
    oss << "  features: [B,C,T,D] = ["
        << st.data.batch_size << ","
        << st.data.active_channels << ","
        << st.data.max_seq_length << ","
        << st.data.feature_dims << "]\n";
    oss << "  packed  : [B,C,T,D+1] (mask in last dim)\n";
    oss << "  future  : [B,C,Hf,D] = ["
        << st.data.batch_size << ","
        << st.data.active_channels << ","
        << st.data.max_future_seq_length << ","
        << st.data.feature_dims << "]\n";
  }

  if (!has_channels) {
    oss << "\n(no channels resolved from active rows)\n";
    return oss.str();
  }

  const auto& c = st.data.channels[idx];

  oss << "\nSelected channel [" << (idx + 1) << "/" << st.data.channels.size() << "]\n";
  oss << "  key       : " << c.instrument << " " << c.interval << " " << c.record_type << "\n";
  oss << "  seq/future: " << c.seq_length << "/" << c.future_seq_length << "\n";
  oss << "  weight    : " << c.channel_weight << "\n";
  oss << "  normW     : " << c.norm_window << "\n";
  oss << "  dims      : " << c.feature_dims << "\n";
  if (st.data.plot_D > 0) {
    const std::size_t dim = std::min(st.data.plot_feature_dim, st.data.plot_D - 1);
    oss << "  selected d: [" << (dim + 1) << "] "
        << data_feature_name_for_channel_dim(c, dim)
        << " (" << data_feature_type_for_channel_dim(c, dim) << ")\n";
  }
  oss << "  source    : " << (c.from_focus_instrument ? "focus" : "fallback") << "\n";

  oss << "\nSelected channel files\n";
  oss << "  csv : " << (c.csv_exists ? "ok " : "missing ") << format_bytes_approx(c.csv_bytes) << "\n";
  oss << "  raw : " << (c.raw_bin_exists ? "ok " : "missing ") << format_bytes_approx(c.raw_bin_bytes) << "\n";
  if (!c.norm_bin_path.empty()) {
    oss << "  norm: " << (c.norm_bin_exists ? "ok " : "missing ") << format_bytes_approx(c.norm_bin_bytes) << "\n";
  } else {
    oss << "  norm: n/a (norm_window=0)\n";
  }

  oss << "\nOption details\n";
  oss << "  plot mode options\n";
  for (const auto mode : {DataPlotMode::SeqLength,
                          DataPlotMode::FutureSeqLength,
                          DataPlotMode::ChannelWeight,
                          DataPlotMode::NormWindow,
                          DataPlotMode::FileBytes}) {
    const bool active = (mode == st.data.plot_mode);
    oss << "  " << (active ? ">" : " ")
        << " " << data_plot_mode_token(mode) << " : "
        << data_plot_mode_description(mode)
        << (data_plot_mode_is_dynamic(mode) ? " [dynamic]" : " [static]")
        << "\n";
  }

  oss << "  x-axis options\n";
  for (const auto axis : {DataPlotXAxis::Index, DataPlotXAxis::KeyValue}) {
    const bool active = (axis == st.data.plot_x_axis);
    oss << "  " << (active ? ">" : " ")
        << " " << data_plot_x_axis_token(axis)
        << " : " << data_plot_x_axis_name(axis) << "\n";
  }

  oss << "  mask options\n";
  oss << "  " << (st.data.plot_mask_overlay ? ">" : " ") << " on\n";
  oss << "  " << (st.data.plot_mask_overlay ? " " : ">") << " off\n";

  oss << "\n  channel options\n";
  for (std::size_t i = 0; i < st.data.channels.size(); ++i) {
    const auto& ch = st.data.channels[i];
    oss << "  " << (i == idx ? ">" : " ")
        << "[" << (i + 1) << "] "
        << ch.interval << "/" << ch.record_type
        << " seq=" << ch.seq_length
        << " fut=" << ch.future_seq_length
        << " w=" << std::fixed << std::setprecision(3) << ch.channel_weight
        << " normW=" << ch.norm_window
        << "\n";
  }

  oss << "\n  sample option\n";
  if (st.data.plot_sample_count == 0) {
    oss << "    n/a\n";
  } else {
    const std::size_t sidx = std::min(st.data.plot_sample_index, st.data.plot_sample_count - 1);
    oss << "    current: [" << (sidx + 1) << "/" << st.data.plot_sample_count << "]\n";
    const std::size_t from = (sidx > 2) ? (sidx - 2) : 0;
    const std::size_t to = std::min(st.data.plot_sample_count - 1, from + 4);
    for (std::size_t i = from; i <= to; ++i) {
      oss << "    " << (i == sidx ? ">" : " ")
          << "[" << (i + 1) << "]\n";
    }
    if (from > 0) oss << "    ...\n";
    if (to + 1 < st.data.plot_sample_count) oss << "    ...\n";
  }

  {
    const auto& names = data_feature_names_for_record_type(c.record_type);
    const auto& dtypes = data_feature_types_for_record_type(c.record_type);
    const std::size_t dim_count = std::max<std::size_t>(st.data.plot_D, std::max(c.feature_dims, names.size()));
    if (dim_count > 0) {
      oss << "\nFeature mapping (" << c.record_type << ")\n";
      for (std::size_t i = 0; i < dim_count; ++i) {
        const bool active = (i == st.data.plot_feature_dim);
        std::string n = (i < names.size()) ? names[i] : ("x" + std::to_string(i));
        std::string t = (i < dtypes.size()) ? dtypes[i] : "double";
        oss << "  " << (active ? ">" : " ")
            << "[" << (i + 1) << "] "
            << n << " : " << t << "\n";
      }
    }
  }

  if (st.data.plot_view) {
    oss << "\nPlot overlay active (Esc to close)\n";
    if (!data_plot_mode_is_dynamic(st.data.plot_mode)) {
      oss << "  current mode is static; overlay plots only seq/future.\n";
    }
  } else if (data_plot_mode_is_dynamic(st.data.plot_mode)) {
    oss << "\nQuick plot (" << data_plot_mode_token(st.data.plot_mode) << ")\n";
    oss << make_data_plot(st.data);
  } else {
    oss << "\nStatic values (" << data_plot_mode_token(st.data.plot_mode) << ")\n";
    oss << make_data_plot(st.data, 22, false);
  }
  return oss.str();
}

inline std::string make_data_right(const CmdState& st) {
  std::ostringstream oss;
  if (!st.data.ok) {
    oss << "Data view error\n";
    oss << "  " << st.data.error << "\n";
    oss << "\ncommands\n";
    oss << "  reload data\n";
    return oss.str();
  }

  const bool has_channels = !st.data.channels.empty();
  const std::size_t idx = has_channels
                              ? std::min(st.data.selected_channel, st.data.channels.size() - 1)
                              : 0;

  auto focus_mark = [&](DataNavFocus f) -> const char* {
    return st.data.nav_focus == f ? ">" : " ";
  };
  auto push_focus_row = [&](DataNavFocus f, std::string row) {
    if (st.data.nav_focus == f) oss << mark_selected_line(std::move(row)) << "\n";
    else oss << row << "\n";
  };

  oss << "Arrow selections\n";
  oss << "  Up/Down: focus selection\n";
  oss << "  Left/Right: change selected value\n";
  push_focus_row(
      DataNavFocus::Channel,
      std::string(" ") + focus_mark(DataNavFocus::Channel) + " channel : " +
          (has_channels
               ? (std::to_string(idx + 1) + "/" + std::to_string(st.data.channels.size()))
               : std::string("n/a")));
  push_focus_row(
      DataNavFocus::Sample,
      std::string(" ") + focus_mark(DataNavFocus::Sample) + " sample  : " +
          (st.data.plot_sample_count == 0
               ? std::string("n/a")
               : (std::to_string(st.data.plot_sample_index + 1) + "/" +
                  std::to_string(st.data.plot_sample_count))));
  push_focus_row(
      DataNavFocus::Dim,
      std::string(" ") + focus_mark(DataNavFocus::Dim) + " dim     : " +
          (st.data.plot_D == 0
               ? std::string("n/a")
               : (std::to_string(st.data.plot_feature_dim + 1) + "/" +
                  std::to_string(st.data.plot_D))));
  push_focus_row(
      DataNavFocus::PlotMode,
      std::string(" ") + focus_mark(DataNavFocus::PlotMode) + " plot    : " +
          data_plot_mode_token(st.data.plot_mode));
  push_focus_row(
      DataNavFocus::XAxis,
      std::string(" ") + focus_mark(DataNavFocus::XAxis) + " x-axis  : " +
          data_plot_x_axis_token(st.data.plot_x_axis));
  push_focus_row(
      DataNavFocus::Mask,
      std::string(" ") + focus_mark(DataNavFocus::Mask) + " mask    : " +
          (st.data.plot_mask_overlay ? "on" : "off"));
  oss << "\nStatus\n";
  oss << "  plot view : " << (st.data.plot_view ? "on" : "off") << "\n";
  if (st.data.plot_view) oss << "  Esc closes plot overlay\n";
  oss << "  details moved to view panel\n";

  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
