#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class DataPlotMode : std::uint8_t {
  SeqLength = 0,
  FutureSeqLength = 1,
  ChannelWeight = 2,
  NormWindow = 3,
  FileBytes = 4,
};

enum class DataPlotXAxis : std::uint8_t {
  Index = 0,
  KeyValue = 1,
};

enum class DataNavFocus : std::uint8_t {
  Channel = 0,
  Sample = 1,
  Dim = 2,
  PlotMode = 3,
  XAxis = 4,
  Mask = 5,
};

struct DataChannelView {
  std::string instrument{};
  std::string interval{};
  std::string record_type{};
  std::size_t seq_length{0};
  std::size_t future_seq_length{0};
  double channel_weight{0.0};
  std::size_t norm_window{0};
  std::size_t feature_dims{0};
  bool from_focus_instrument{false};

  std::string csv_path{};
  std::string raw_bin_path{};
  std::string norm_bin_path{};

  bool csv_exists{false};
  bool raw_bin_exists{false};
  bool norm_bin_exists{false};
  std::uintmax_t csv_bytes{0};
  std::uintmax_t raw_bin_bytes{0};
  std::uintmax_t norm_bin_bytes{0};
};

struct DataState {
  bool ok{false};
  std::string error{};
  std::string raw_instruction{};
  std::string focus_instrument{};

  std::size_t batch_size{0};
  std::size_t active_channels{0};
  std::size_t max_seq_length{0};
  std::size_t max_future_seq_length{0};
  std::size_t feature_dims{0};
  bool mixed_feature_dims{false};

  std::size_t csv_present{0};
  std::size_t raw_bin_present{0};
  std::size_t norm_bin_present{0};

  std::vector<DataChannelView> channels{};
  std::size_t selected_channel{0};
  DataPlotMode plot_mode{DataPlotMode::SeqLength};
  DataPlotXAxis plot_x_axis{DataPlotXAxis::Index};
  DataNavFocus nav_focus{DataNavFocus::Channel};
  bool plot_view{false};

  // Runtime sample navigation (populated by iinuji_cmd app data runtime).
  bool plot_tensor_ready{false};
  std::string plot_tensor_error{};
  std::size_t plot_sample_index{0};   // 0-based
  std::size_t plot_sample_count{0};
  std::size_t plot_C{0};
  std::size_t plot_T{0};
  std::size_t plot_D{0};
  std::size_t plot_feature_dim{0};    // selected D index
  bool plot_mask_overlay{true};
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
