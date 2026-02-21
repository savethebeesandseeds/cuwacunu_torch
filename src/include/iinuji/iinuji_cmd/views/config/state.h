#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct ConfigTabData {
  std::string id{};
  std::string title{};
  std::string path{};
  bool ok{false};
  std::string error{};
  std::string content{};
};

struct ConfigState {
  bool ok{false};
  std::string error{};
  std::vector<ConfigTabData> tabs{};
  std::size_t selected_tab{0};
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
