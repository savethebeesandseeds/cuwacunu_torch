#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cuwacunu {
namespace hero {
namespace mcp {

class hero_config_store_t {
 public:
  struct view_entry_t {
    std::string key;
    std::string declared_type;
    std::string value;
  };

  struct diff_entry_t {
    std::string key;
    std::string action;
    std::string before_declared_type;
    std::string before_value;
    std::string after_declared_type;
    std::string after_value;
  };

  struct save_preview_t {
    bool file_exists{false};
    bool text_changed{false};
    bool has_changes{false};
    std::size_t diff_count{0};
    std::string current_text;
    std::string proposed_text;
    std::vector<diff_entry_t> diffs;
  };

  struct backup_entry_t {
    std::string filename;
    std::string path;
  };

  explicit hero_config_store_t(std::string config_path);

  [[nodiscard]] bool load(std::string* err);
  [[nodiscard]] const std::string& config_path() const;
  [[nodiscard]] bool loaded() const;
  [[nodiscard]] bool dirty() const;
  [[nodiscard]] bool from_template() const;

  [[nodiscard]] std::optional<std::string> get_value(std::string_view key) const;
  [[nodiscard]] std::string get_or_default(std::string_view key) const;
  [[nodiscard]] bool set_value(std::string_view key, std::string_view value,
                               std::string* err);
  [[nodiscard]] bool save(std::string* err);
  [[nodiscard]] bool preview_save(save_preview_t* out, std::string* err) const;
  [[nodiscard]] bool list_backups(std::vector<backup_entry_t>* out,
                                  std::string* err) const;
  [[nodiscard]] bool rollback_to_backup(std::string_view backup_name_or_path,
                                        std::string* out_selected_path,
                                        std::string* err);

  [[nodiscard]] std::vector<std::string> validate() const;
  [[nodiscard]] std::vector<view_entry_t> entries_snapshot() const;

 private:
  struct entry_t {
    std::string key;
    std::string declared_type;
    std::string value;
  };

  [[nodiscard]] std::string render() const;
  void rebuild_index();

  std::string config_path_;
  bool loaded_{false};
  bool dirty_{false};
  bool from_template_{false};
  std::vector<entry_t> entries_;
  std::map<std::string, std::size_t, std::less<>> index_by_key_;
};

}  // namespace mcp
}  // namespace hero
}  // namespace cuwacunu
