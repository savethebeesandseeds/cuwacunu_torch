#pragma once

#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "iinuji/iinuji_types.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct ConfigAsyncState;

enum class ConfigFileFamily : std::uint8_t {
  Defaults = 0,
  Objectives = 1,
  Optim = 2,
};

enum class ConfigBrowseFocus : std::uint8_t {
  Families = 0,
  Files = 1,
};

struct ConfigFileEntry {
  std::string id{};
  std::string title{};
  std::string path{};
  std::string root_path{};
  std::string relative_path{};
  std::string request_root_path{};
  std::string request_path{};
  std::string family_id{};
  std::string family_title{};
  std::string extension{};
  std::string sha256{};
  bool ok{false};
  std::string error{};
  std::string saved_content{};
  std::string content{};
  bool replace_supported{false};
  bool write_allowed{false};
  bool editable{false};
  ConfigFileFamily family{ConfigFileFamily::Defaults};
};

struct ConfigState {
  bool ok{false};
  std::string error{};
  std::vector<ConfigFileEntry> files{};
  std::size_t selected_file{0};
  ConfigFileFamily selected_family{ConfigFileFamily::Defaults};
  ConfigBrowseFocus browse_focus{ConfigBrowseFocus::Files};
  std::shared_ptr<cuwacunu::iinuji::editorBox_data_t> editor{};
  std::string status{};
  bool status_is_error{false};
  bool editor_focus{false};
  std::string policy_path{};
  std::string write_policy_path{};
  bool using_session_policy{false};
  std::string allowed_extensions{};
  bool policy_write_allowed{false};
  std::vector<std::string> default_roots{};
  std::vector<std::string> objective_roots{};
  std::vector<std::string> write_roots{};
  std::string optim_root{};
  std::string optim_archive_path{};
  std::string optim_public_keep_path{};
  std::shared_ptr<ConfigAsyncState> async{};
};

struct ConfigAsyncState {
  bool refresh_pending{false};
  bool refresh_requeue{false};
  std::future<std::shared_ptr<ConfigState>> refresh_future{};
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
