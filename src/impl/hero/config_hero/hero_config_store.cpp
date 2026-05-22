#include "hero/config_hero/hero_config_store.h"

#include "hero/config_hero/hero_config.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include <sys/stat.h>
#include <unistd.h>

namespace cuwacunu::hero::config {
namespace {

namespace fs = std::filesystem;

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  std::size_t begin = 0;
  while (begin < in.size() &&
         std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
    ++begin;
  }
  std::size_t end = in.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
    --end;
  }
  return std::string(in.substr(begin, end - begin));
}

[[nodiscard]] std::string lowercase_ascii(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] std::string strip_inline_comment(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  bool in_single = false;
  bool in_double = false;
  for (const char c : in) {
    if (c == '\'' && !in_double) {
      in_single = !in_single;
      out.push_back(c);
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      out.push_back(c);
      continue;
    }
    if (!in_single && !in_double && c == '#') {
      break;
    }
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] bool parse_bool(std::string_view raw, bool *out) {
  const std::string value = lowercase_ascii(trim_ascii(raw));
  if (value == "true" || value == "1" || value == "yes" || value == "on") {
    if (out) {
      *out = true;
    }
    return true;
  }
  if (value == "false" || value == "0" || value == "no" || value == "off") {
    if (out) {
      *out = false;
    }
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_int(std::string_view raw, int *out) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  int parsed = 0;
  const char *begin = value.data();
  const char *end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end) {
    return false;
  }
  if (out) {
    *out = parsed;
  }
  return true;
}

[[nodiscard]] std::vector<std::string> split_csv(std::string_view raw) {
  std::vector<std::string> out;
  std::string item;
  std::istringstream in{std::string(raw)};
  while (std::getline(in, item, ',')) {
    item = trim_ascii(item);
    if (!item.empty()) {
      out.push_back(item);
    }
  }
  return out;
}

[[nodiscard]] const runtime_key_descriptor_t *
find_descriptor(std::string_view key) {
  for (const auto &descriptor : kRuntimeKeyDescriptors) {
    if (descriptor.key == key) {
      return &descriptor;
    }
  }
  return nullptr;
}

[[nodiscard]] fs::path normalize_path(const fs::path &path) {
  std::error_code ec;
  const fs::path canonical = fs::weakly_canonical(path, ec);
  return ec ? path.lexically_normal() : canonical;
}

[[nodiscard]] fs::path resolve_near_config(std::string_view raw_path,
                                           const fs::path &config_path) {
  fs::path path(trim_ascii(raw_path));
  if (path.empty()) {
    return path;
  }
  if (path.is_absolute()) {
    return normalize_path(path);
  }
  const fs::path parent = config_path.parent_path();
  if (!parent.empty()) {
    return normalize_path(parent / path);
  }
  return normalize_path(path);
}

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count());
}

[[nodiscard]] std::string sanitize_filename_component(std::string in) {
  if (in.empty()) {
    return "config";
  }
  for (char &c : in) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
    if (!ok) {
      c = '_';
    }
  }
  return in;
}

[[nodiscard]] bool read_text_file(const fs::path &path, std::string *out,
                                  std::string *err) {
  if (!out) {
    if (err) {
      *err = "read output pointer is null";
    }
    return false;
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (err) {
      *err = "cannot open file: " + path.string();
    }
    return false;
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  if (in.bad()) {
    if (err) {
      *err = "cannot read file: " + path.string();
    }
    return false;
  }
  *out = buffer.str();
  return true;
}

[[nodiscard]] bool write_text_file_atomic(const fs::path &path,
                                          std::string_view text,
                                          std::string *err) {
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  if (ec) {
    if (err) {
      *err = "cannot create parent directory for " + path.string() + ": " +
             ec.message();
    }
    return false;
  }

  const fs::path tmp = path.string() + ".tmp." + std::to_string(::getpid());
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (err) {
        *err = "cannot open temp file for write: " + tmp.string();
      }
      return false;
    }
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    out.flush();
    if (!out) {
      if (err) {
        *err = "cannot write temp file: " + tmp.string();
      }
      std::error_code ignored;
      fs::remove(tmp, ignored);
      return false;
    }
  }
  (void)::chmod(tmp.c_str(), S_IRUSR | S_IWUSR);
  fs::rename(tmp, path, ec);
  if (ec) {
    std::error_code ignored;
    fs::remove(tmp, ignored);
    if (err) {
      *err =
          "cannot publish temp file to " + path.string() + ": " + ec.message();
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool parse_entry_line(std::string_view line,
                                    hero_config_store_t::view_entry_t *out,
                                    std::string *err) {
  const std::string stripped = trim_ascii(strip_inline_comment(line));
  if (stripped.empty()) {
    return false;
  }
  const std::size_t eq = stripped.find('=');
  if (eq == std::string::npos) {
    if (err) {
      *err = "missing '=' in config line: " + stripped;
    }
    return false;
  }

  const std::string lhs = trim_ascii(stripped.substr(0, eq));
  const std::string value = trim_ascii(stripped.substr(eq + 1));
  if (lhs.empty()) {
    if (err) {
      *err = "empty config key in line: " + stripped;
    }
    return false;
  }

  std::string key = lhs;
  std::string domain;
  std::string type = "str";
  const std::size_t colon = lhs.rfind(':');
  if (colon != std::string::npos) {
    key = trim_ascii(lhs.substr(0, colon));
    type = trim_ascii(lhs.substr(colon + 1));
  }
  const std::size_t bracket = key.find('[');
  if (bracket != std::string::npos) {
    const std::size_t close = key.find(']', bracket + 1);
    if (close == std::string::npos) {
      if (err) {
        *err = "missing ']' in config key declaration: " + lhs;
      }
      return false;
    }
    domain = trim_ascii(key.substr(bracket + 1, close - bracket - 1));
    key = trim_ascii(key.substr(0, bracket));
  }
  if (key.empty() || type.empty()) {
    if (err) {
      *err = "malformed config declaration: " + lhs;
    }
    return false;
  }
  if (out) {
    out->key = std::move(key);
    out->declared_domain = std::move(domain);
    out->declared_type = std::move(type);
    out->value = std::move(value);
  }
  return true;
}

[[nodiscard]] bool
parse_config_text(const std::string &text,
                  std::vector<hero_config_store_t::view_entry_t> *out_entries,
                  std::string *err) {
  if (!out_entries) {
    if (err) {
      *err = "entry output pointer is null";
    }
    return false;
  }
  out_entries->clear();
  std::istringstream in(text);
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(in, line)) {
    ++line_number;
    hero_config_store_t::view_entry_t entry;
    std::string line_error;
    if (!parse_entry_line(line, &entry, &line_error)) {
      if (!line_error.empty()) {
        if (err) {
          *err = "line " + std::to_string(line_number) + ": " + line_error;
        }
        return false;
      }
      continue;
    }
    out_entries->push_back(std::move(entry));
  }
  return true;
}

[[nodiscard]] bool backup_file(const fs::path &source,
                               const fs::path &backup_dir, int max_entries,
                               std::string *err) {
  std::error_code ec;
  if (!fs::exists(source, ec)) {
    return true;
  }
  if (ec) {
    if (err) {
      *err = "cannot inspect file for backup: " + source.string() + ": " +
             ec.message();
    }
    return false;
  }
  fs::create_directories(backup_dir, ec);
  if (ec) {
    if (err) {
      *err = "cannot create backup directory " + backup_dir.string() + ": " +
             ec.message();
    }
    return false;
  }

  const std::string filename =
      std::to_string(now_ms_utc()) + "--" +
      sanitize_filename_component(source.filename().string()) + ".bak";
  const fs::path backup_path = backup_dir / filename;
  fs::copy_file(source, backup_path, fs::copy_options::overwrite_existing, ec);
  if (ec) {
    if (err) {
      *err =
          "cannot write backup " + backup_path.string() + ": " + ec.message();
    }
    return false;
  }
  (void)::chmod(backup_path.c_str(), S_IRUSR | S_IWUSR);

  if (max_entries < 1) {
    return true;
  }
  std::vector<fs::directory_entry> backups;
  for (fs::directory_iterator it(backup_dir, ec), end; !ec && it != end;
       it.increment(ec)) {
    if (it->is_regular_file(ec) && it->path().extension() == ".bak") {
      backups.push_back(*it);
    }
  }
  if (ec) {
    return true;
  }
  std::sort(backups.begin(), backups.end(),
            [](const fs::directory_entry &a, const fs::directory_entry &b) {
              return a.path().filename().string() >
                     b.path().filename().string();
            });
  for (std::size_t i = static_cast<std::size_t>(max_entries);
       i < backups.size(); ++i) {
    std::error_code ignored;
    fs::remove(backups[i].path(), ignored);
  }
  return true;
}

} // namespace

hero_config_store_t::hero_config_store_t(std::string config_path,
                                         std::string global_config_path)
    : config_path_(std::move(config_path)),
      global_config_path_(std::move(global_config_path)) {}

bool hero_config_store_t::load(std::string *err) {
  std::string text;
  std::error_code ec;
  from_template_ = false;
  if (fs::exists(config_path_, ec) && !ec) {
    if (!read_text_file(config_path_, &text, err)) {
      return false;
    }
  } else {
    text = std::string(kRuntimeConfigDslTemplateText);
    from_template_ = true;
  }

  std::vector<view_entry_t> parsed;
  if (!parse_config_text(text, &parsed, err)) {
    return false;
  }

  entries_.clear();
  entries_.reserve(parsed.size() + kRuntimeKeyDescriptors.size());
  for (auto &entry : parsed) {
    entries_.push_back(
        entry_t{std::move(entry.key), std::move(entry.declared_domain),
                std::move(entry.declared_type), std::move(entry.value)});
  }
  rebuild_index();

  for (const auto &descriptor : kRuntimeKeyDescriptors) {
    if (index_by_key_.find(descriptor.key) != index_by_key_.end()) {
      continue;
    }
    entry_t entry;
    entry.key = std::string(descriptor.key);
    entry.declared_type = std::string(descriptor.type);
    entry.value = std::string(descriptor.default_value);
    entries_.push_back(std::move(entry));
  }
  rebuild_index();
  loaded_ = true;
  dirty_ = from_template_;
  return true;
}

const std::string &hero_config_store_t::config_path() const {
  return config_path_;
}

const std::string &hero_config_store_t::global_config_path() const {
  return global_config_path_;
}

bool hero_config_store_t::loaded() const { return loaded_; }

bool hero_config_store_t::dirty() const { return dirty_; }

bool hero_config_store_t::from_template() const { return from_template_; }

std::optional<std::string>
hero_config_store_t::get_value(std::string_view key) const {
  const auto it = index_by_key_.find(key);
  if (it == index_by_key_.end()) {
    return std::nullopt;
  }
  return entries_[it->second].value;
}

std::string hero_config_store_t::get_or_default(std::string_view key) const {
  const auto found = get_value(key);
  if (found.has_value()) {
    return *found;
  }
  if (const auto *descriptor = find_descriptor(key)) {
    return std::string(descriptor->default_value);
  }
  return {};
}

bool hero_config_store_t::set_value(std::string_view key,
                                    std::string_view value, std::string *err) {
  const auto *descriptor = find_descriptor(key);
  if (!descriptor) {
    if (err) {
      *err = "unknown Config Hero key: " + std::string(key);
    }
    return false;
  }
  const auto it = index_by_key_.find(key);
  if (it == index_by_key_.end()) {
    entry_t entry;
    entry.key = std::string(descriptor->key);
    entry.declared_type = std::string(descriptor->type);
    entry.value = std::string(value);
    entries_.push_back(std::move(entry));
    rebuild_index();
  } else {
    entries_[it->second].declared_type = std::string(descriptor->type);
    entries_[it->second].value = std::string(value);
  }
  dirty_ = true;
  return true;
}

bool hero_config_store_t::save(std::string *err) {
  const std::string proposed = render();
  bool backup_enabled = true;
  (void)parse_bool(get_or_default("backup_enabled"), &backup_enabled);
  int max_entries = 20;
  (void)parse_int(get_or_default("backup_max_entries"), &max_entries);
  const fs::path backup_dir =
      resolve_near_config(get_or_default("backup_dir"), config_path_) /
      "policy";

  if (backup_enabled &&
      !backup_file(config_path_, backup_dir, max_entries, err)) {
    return false;
  }
  if (!write_text_file_atomic(config_path_, proposed, err)) {
    return false;
  }
  dirty_ = false;
  from_template_ = false;
  return true;
}

bool hero_config_store_t::preview_save(save_preview_t *out,
                                       std::string *err) const {
  if (!out) {
    if (err) {
      *err = "preview output pointer is null";
    }
    return false;
  }
  *out = save_preview_t{};
  out->proposed_text = render();
  std::error_code ec;
  out->file_exists = fs::exists(config_path_, ec) && !ec;
  if (out->file_exists) {
    if (!read_text_file(config_path_, &out->current_text, err)) {
      return false;
    }
  }
  out->text_changed = out->current_text != out->proposed_text;

  std::vector<view_entry_t> current;
  if (out->file_exists &&
      !parse_config_text(out->current_text, &current, err)) {
    return false;
  }
  std::map<std::string, view_entry_t, std::less<>> before;
  for (auto &entry : current) {
    before.emplace(entry.key, std::move(entry));
  }
  std::map<std::string, view_entry_t, std::less<>> after;
  for (const auto &entry : entries_snapshot()) {
    after.emplace(entry.key, entry);
  }

  for (const auto &[key, next] : after) {
    const auto prev = before.find(key);
    if (prev == before.end()) {
      out->diffs.push_back(diff_entry_t{key, "added", "", "", "",
                                        next.declared_domain,
                                        next.declared_type, next.value});
      continue;
    }
    if (prev->second.value != next.value ||
        prev->second.declared_domain != next.declared_domain ||
        prev->second.declared_type != next.declared_type) {
      out->diffs.push_back(diff_entry_t{
          key,
          "changed",
          prev->second.declared_domain,
          prev->second.declared_type,
          prev->second.value,
          next.declared_domain,
          next.declared_type,
          next.value,
      });
    }
  }
  for (const auto &[key, prev] : before) {
    if (after.find(key) != after.end()) {
      continue;
    }
    out->diffs.push_back(diff_entry_t{key, "removed", prev.declared_domain,
                                      prev.declared_type, prev.value, "", "",
                                      ""});
  }
  out->diff_count = out->diffs.size();
  out->has_changes = out->text_changed || !out->diffs.empty();
  return true;
}

bool hero_config_store_t::list_backups(std::vector<backup_entry_t> *out,
                                       std::string *err) const {
  if (!out) {
    if (err) {
      *err = "backup output pointer is null";
    }
    return false;
  }
  out->clear();
  const fs::path backup_dir =
      resolve_near_config(get_or_default("backup_dir"), config_path_) /
      "policy";
  std::error_code ec;
  if (!fs::exists(backup_dir, ec)) {
    return true;
  }
  if (ec) {
    if (err) {
      *err = "cannot inspect backup directory " + backup_dir.string() + ": " +
             ec.message();
    }
    return false;
  }
  for (fs::directory_iterator it(backup_dir, ec), end; !ec && it != end;
       it.increment(ec)) {
    if (!it->is_regular_file(ec) || it->path().extension() != ".bak") {
      continue;
    }
    out->push_back(
        backup_entry_t{it->path().filename().string(), it->path().string()});
  }
  if (ec) {
    if (err) {
      *err = "cannot list backup directory " + backup_dir.string() + ": " +
             ec.message();
    }
    return false;
  }
  std::sort(out->begin(), out->end(), [](const auto &a, const auto &b) {
    return a.filename > b.filename;
  });
  return true;
}

bool hero_config_store_t::rollback_to_backup(
    std::string_view backup_name_or_path, std::string *out_selected_path,
    std::string *err) {
  std::vector<backup_entry_t> backups;
  if (!list_backups(&backups, err)) {
    return false;
  }
  if (backups.empty()) {
    if (err) {
      *err = "no Config Hero policy backups found";
    }
    return false;
  }

  std::string selector = trim_ascii(backup_name_or_path);
  fs::path selected;
  if (selector.empty()) {
    selected = backups.front().path;
  } else {
    int index = -1;
    if (parse_int(selector, &index) && index >= 0 &&
        static_cast<std::size_t>(index) < backups.size()) {
      selected = backups[static_cast<std::size_t>(index)].path;
    } else {
      for (const auto &backup : backups) {
        if (backup.filename == selector || backup.path == selector) {
          selected = backup.path;
          break;
        }
      }
    }
  }
  if (selected.empty()) {
    if (err) {
      *err = "backup not found: " + selector;
    }
    return false;
  }

  std::string text;
  if (!read_text_file(selected, &text, err)) {
    return false;
  }
  std::vector<view_entry_t> parsed;
  if (!parse_config_text(text, &parsed, err)) {
    return false;
  }
  if (!write_text_file_atomic(config_path_, text, err)) {
    return false;
  }
  if (out_selected_path) {
    *out_selected_path = selected.string();
  }
  return load(err);
}

std::vector<std::string> hero_config_store_t::validate() const {
  std::vector<std::string> errors;
  if (!loaded_) {
    errors.emplace_back("store is not loaded");
    return errors;
  }

  std::map<std::string, std::size_t, std::less<>> seen;
  for (const auto &entry : entries_) {
    const auto [_, inserted] = seen.emplace(entry.key, 1);
    if (!inserted) {
      errors.emplace_back("duplicate key: " + entry.key);
    }
    if (!find_descriptor(entry.key)) {
      errors.emplace_back("unknown key: " + entry.key);
    }
  }

  for (const auto &descriptor : kRuntimeKeyDescriptors) {
    const auto value = get_or_default(descriptor.key);
    if (descriptor.required && trim_ascii(value).empty()) {
      errors.emplace_back("missing required key: " +
                          std::string(descriptor.key));
      continue;
    }
    const std::string type = std::string(descriptor.type);
    if (type == "bool") {
      bool ignored = false;
      if (!parse_bool(value, &ignored)) {
        errors.emplace_back(std::string(descriptor.key) + " must be boolean");
      }
    } else if (type == "int") {
      int parsed = 0;
      if (!parse_int(value, &parsed) || parsed < 1) {
        errors.emplace_back(std::string(descriptor.key) +
                            " must be an integer >= 1");
      }
    } else if (std::string(descriptor.key) == "protocol_layer") {
      const std::string protocol = lowercase_ascii(value);
      if (protocol != "stdio" && protocol != "https/sse") {
        errors.emplace_back("protocol_layer must be STDIO or HTTPS/SSE");
      } else if (protocol == "https/sse") {
        errors.emplace_back(std::string(kProtocolLayerHttpsSseFailFastMessage));
      }
    } else if (type == "path") {
      if (trim_ascii(value).empty()) {
        errors.emplace_back(std::string(descriptor.key) + " must not be empty");
      }
    } else if (type == "path_list" || type == "string_list") {
      if (split_csv(value).empty()) {
        errors.emplace_back(std::string(descriptor.key) +
                            " must contain at least one value");
      }
    }
  }

  const fs::path config_root =
      resolve_near_config(get_or_default("config_root"), config_path_);
  std::error_code ec;
  if (!fs::is_directory(config_root, ec)) {
    errors.emplace_back("config_root is not an existing directory: " +
                        config_root.string());
  }

  for (const std::string &raw : split_csv(get_or_default("managed_roots"))) {
    const fs::path root = resolve_near_config(raw, config_path_);
    if (!fs::is_directory(root, ec)) {
      errors.emplace_back("managed_roots contains non-directory: " +
                          root.string());
    }
  }

  bool allow_write = false;
  (void)parse_bool(get_or_default("allow_write"), &allow_write);
  if (allow_write) {
    for (const std::string &raw : split_csv(get_or_default("write_roots"))) {
      const fs::path root = resolve_near_config(raw, config_path_);
      if (!fs::is_directory(root, ec)) {
        errors.emplace_back("write_roots contains non-directory: " +
                            root.string());
      }
    }
  }

  for (const std::string &ext :
       split_csv(get_or_default("allowed_extensions"))) {
    if (ext.empty() || ext.front() != '.') {
      errors.emplace_back("allowed extension must start with '.': " + ext);
    }
  }
  return errors;
}

std::vector<hero_config_store_t::view_entry_t>
hero_config_store_t::entries_snapshot() const {
  std::vector<view_entry_t> out;
  out.reserve(entries_.size());
  for (const auto &entry : entries_) {
    out.push_back(view_entry_t{entry.key, entry.declared_domain,
                               entry.declared_type, entry.value});
  }
  return out;
}

std::string hero_config_store_t::render() const {
  std::ostringstream out;
  out << "# Config Hero v2 policy\n";
  for (const auto &descriptor : kRuntimeKeyDescriptors) {
    const std::string key(descriptor.key);
    const auto value = get_or_default(key);
    out << key;
    if (!descriptor.allowed.empty() && std::string(descriptor.type) == "enum") {
      out << "[" << descriptor.allowed << "]";
    }
    out << ":" << descriptor.type << " = " << value << "\n";
  }
  for (const auto &entry : entries_) {
    if (find_descriptor(entry.key)) {
      continue;
    }
    out << entry.key;
    if (!entry.declared_domain.empty()) {
      out << "[" << entry.declared_domain << "]";
    }
    out << ":" << (entry.declared_type.empty() ? "str" : entry.declared_type)
        << " = " << entry.value << "\n";
  }
  return out.str();
}

void hero_config_store_t::rebuild_index() {
  index_by_key_.clear();
  for (std::size_t i = 0; i < entries_.size(); ++i) {
    index_by_key_[entries_[i].key] = i;
  }
}

} // namespace cuwacunu::hero::config
