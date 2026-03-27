#include "hero/config_hero/hero_config_store.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/config_hero/hero.config.h"
#include "hero/hero_runtime_lock.h"

namespace {

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() &&
         std::isspace(static_cast<unsigned char>(in[b])) != 0) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])) != 0) {
    --e;
  }
  return std::string(in.substr(b, e - b));
}

[[nodiscard]] std::string lowercase_copy(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] std::vector<std::string> split_csv(std::string_view in) {
  std::vector<std::string> out;
  std::string current;
  for (const char c : in) {
    if (c == ',') {
      const std::string item = trim_ascii(current);
      if (!item.empty()) out.push_back(item);
      current.clear();
      continue;
    }
    current.push_back(c);
  }
  const std::string tail = trim_ascii(current);
  if (!tail.empty()) out.push_back(tail);
  return out;
}

[[nodiscard]] bool read_text_file(const std::string& path, std::string* out,
                                  std::string* err) {
  std::ifstream in(path);
  if (!in.is_open()) {
    if (err) *err = "failed to open: " + path;
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  if (!in.good() && !in.eof()) {
    if (err) *err = "failed to read: " + path;
    return false;
  }
  if (out) *out = ss.str();
  return true;
}

[[nodiscard]] bool write_text_file_atomic(const std::string& path,
                                          const std::string& content,
                                          std::string* err) {
  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path target(path);
  if (!target.has_parent_path()) {
    if (err) *err = "target path has no parent: " + path;
    return false;
  }

  fs::create_directories(target.parent_path(), ec);
  if (ec) {
    if (err) *err = "failed to create parent directory for: " + path;
    return false;
  }

  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  const fs::path tmp = target.parent_path() /
                       (target.filename().string() + ".tmp." +
                        std::to_string(static_cast<long long>(now)));

  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      if (err) *err = "failed to open temp file: " + tmp.string();
      return false;
    }
    out << content;
    out.flush();
    if (!out.good()) {
      if (err) *err = "failed to write temp file: " + tmp.string();
      return false;
    }
  }

  fs::rename(tmp, target, ec);
  if (ec) {
    std::error_code rm_ec;
    fs::remove(tmp, rm_ec);
    if (err) *err = "failed to replace target file: " + path;
    return false;
  }
  return true;
}

[[nodiscard]] bool parse_bool_value(std::string_view in, bool* out) {
  const std::string v = lowercase_copy(trim_ascii(in));
  if (v == "true" || v == "1" || v == "yes" || v == "y" || v == "on") {
    if (out) *out = true;
    return true;
  }
  if (v == "false" || v == "0" || v == "no" || v == "n" || v == "off") {
    if (out) *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_int64_value(std::string_view in, int64_t* out) {
  const std::string v = trim_ascii(in);
  if (v.empty()) return false;
  int64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(v.data(), v.data() + v.size(), parsed);
  if (ec != std::errc() || ptr != v.data() + v.size()) return false;
  if (out) *out = parsed;
  return true;
}

[[nodiscard]] bool parse_double_value(std::string_view in, double* out) {
  const std::string v = trim_ascii(in);
  if (v.empty()) return false;
  char* end = nullptr;
  errno = 0;
  const double parsed = std::strtod(v.c_str(), &end);
  if (errno != 0 || end == nullptr || *end != '\0') return false;
  if (out) *out = parsed;
  return true;
}

[[nodiscard]] std::string normalized_protocol_layer(std::string_view in) {
  return lowercase_copy(trim_ascii(in));
}

[[nodiscard]] std::filesystem::path resolve_path_near_config(
    std::string_view raw_path, std::string_view config_path) {
  namespace fs = std::filesystem;
  fs::path resolved(trim_ascii(raw_path));
  if (resolved.empty()) return resolved;
  if (resolved.is_relative()) {
    const fs::path config_parent = fs::path(config_path).parent_path();
    if (!config_parent.empty()) resolved = config_parent / resolved;
  }
  return resolved.lexically_normal();
}

[[nodiscard]] bool file_name_has_prefix(std::string_view file_name,
                                        std::string_view prefix) {
  return file_name.size() >= prefix.size() &&
         file_name.compare(0, prefix.size(), prefix) == 0;
}

struct backup_file_info_t {
  std::filesystem::path path;
  std::filesystem::file_time_type mtime;
};

[[nodiscard]] bool collect_backup_files_sorted_desc(
    std::string_view source_path, const std::filesystem::path& backup_dir,
    std::vector<backup_file_info_t>* out, std::string* err) {
  namespace fs = std::filesystem;
  if (out == nullptr) {
    if (err) *err = "backup collector output is null";
    return false;
  }
  out->clear();

  std::error_code ec;
  if (!fs::exists(backup_dir, ec)) {
    if (ec) {
      if (err) *err = "failed to inspect backup directory: " + backup_dir.string();
      return false;
    }
    return true;
  }
  if (!fs::is_directory(backup_dir, ec)) {
    if (ec) {
      if (err) *err = "failed to inspect backup directory: " + backup_dir.string();
    } else if (err) {
      *err = "backup_dir is not a directory: " + backup_dir.string();
    }
    return false;
  }

  const std::string base_name = fs::path(source_path).filename().string();
  const std::string prefix = base_name + ".bak.";

  const fs::directory_iterator begin(backup_dir, ec);
  if (ec) {
    if (err) *err = "failed to enumerate backup directory: " + backup_dir.string();
    return false;
  }

  for (const auto& entry : begin) {
    if (!entry.is_regular_file(ec) || ec) continue;
    const std::string name = entry.path().filename().string();
    if (!file_name_has_prefix(name, prefix)) continue;
    const auto mtime = entry.last_write_time(ec);
    if (ec) {
      if (err) *err = "failed to stat backup file: " + entry.path().string();
      return false;
    }
    out->push_back({entry.path(), mtime});
  }

  std::sort(out->begin(), out->end(), [](const auto& a, const auto& b) {
    if (a.mtime == b.mtime) return a.path.string() > b.path.string();
    return a.mtime > b.mtime;
  });
  return true;
}

[[nodiscard]] bool path_is_within(std::filesystem::path base,
                                  std::filesystem::path candidate) {
  std::error_code ec{};
  if (!base.empty()) {
    const auto c = std::filesystem::weakly_canonical(base, ec);
    if (!ec) base = c;
  }
  ec.clear();
  if (!candidate.empty()) {
    const auto c = std::filesystem::weakly_canonical(candidate, ec);
    if (!ec) candidate = c;
  }
  base = base.lexically_normal();
  candidate = candidate.lexically_normal();
  auto base_it = base.begin();
  auto cand_it = candidate.begin();
  for (; base_it != base.end() && cand_it != candidate.end();
       ++base_it, ++cand_it) {
    if (*base_it != *cand_it) return false;
  }
  return base_it == base.end();
}

[[nodiscard]] bool is_deprecated_runtime_key(std::string_view key) {
  return key == "mode" || key == "transport" || key == "endpoint" ||
         key == "auth_token_env" || key == "model" ||
         key == "reasoning_effort" || key == "temperature" ||
         key == "top_p" || key == "max_output_tokens" ||
         key == "timeout_ms" || key == "connect_timeout_ms" ||
         key == "retry_max_attempts" || key == "retry_backoff_ms" ||
         key == "verify_tls";
}

[[nodiscard]] bool backup_previous_file_with_cap(
    std::string_view source_path, const std::filesystem::path& backup_dir,
    int64_t max_entries, std::string* err) {
  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path source(source_path);
  if (!fs::exists(source, ec)) return true;
  if (ec) {
    if (err) *err = "failed to inspect source config for backup: " + source.string();
    return false;
  }

  fs::create_directories(backup_dir, ec);
  if (ec) {
    if (err) {
      *err = "failed to create backup directory: " + backup_dir.string();
    }
    return false;
  }

  const std::string base_name = source.filename().string();
  const std::string prefix = base_name + ".bak.";
  const auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  fs::path backup_path =
      backup_dir / (prefix + std::to_string(static_cast<long long>(stamp)) + ".dsl");
  int disambiguator = 1;
  while (fs::exists(backup_path, ec)) {
    if (ec) {
      if (err) *err = "failed to probe backup path: " + backup_path.string();
      return false;
    }
    backup_path = backup_dir / (prefix + std::to_string(static_cast<long long>(stamp)) +
                                "." + std::to_string(disambiguator++) + ".dsl");
  }

  fs::copy_file(source, backup_path, fs::copy_options::none, ec);
  if (ec) {
    if (err) {
      *err = "failed to write backup file: " + backup_path.string();
    }
    return false;
  }

  std::vector<std::pair<std::filesystem::file_time_type, fs::path>> backups;
  const fs::directory_iterator begin(backup_dir, ec);
  if (ec) {
    if (err) {
      *err = "failed to enumerate backup directory: " + backup_dir.string();
    }
    return false;
  }
  for (const auto& entry : begin) {
    if (!entry.is_regular_file(ec) || ec) continue;
    const std::string name = entry.path().filename().string();
    if (!file_name_has_prefix(name, prefix)) continue;
    const auto mtime = entry.last_write_time(ec);
    if (ec) {
      if (err) {
        *err = "failed to stat backup file: " + entry.path().string();
      }
      return false;
    }
    backups.push_back({mtime, entry.path()});
  }

  std::sort(backups.begin(), backups.end(),
            [](const auto& a, const auto& b) {
              if (a.first == b.first) return a.second.string() < b.second.string();
              return a.first < b.first;
            });

  const std::size_t keep =
      max_entries > 0 ? static_cast<std::size_t>(max_entries) : std::size_t{0};
  while (backups.size() > keep) {
    const fs::path doomed = backups.front().second;
    backups.erase(backups.begin());
    fs::remove(doomed, ec);
    if (ec) {
      if (err) *err = "failed to prune backup file: " + doomed.string();
      return false;
    }
  }

  return true;
}

[[nodiscard]] const cuwacunu::hero::config::runtime_key_descriptor_t*
find_runtime_descriptor(std::string_view key) {
  for (const auto& d : cuwacunu::hero::config::kRuntimeKeyDescriptors) {
    if (d.key == key) return &d;
  }
  return nullptr;
}

[[nodiscard]] std::string descriptor_dsl_type(
    const cuwacunu::hero::config::runtime_key_descriptor_t& d) {
  if (d.type == "int") return "int";
  if (d.type == "float") return "float";
  if (d.type == "bool") return "bool";
  return "str";
}

[[nodiscard]] std::string descriptor_dsl_domain(
    const cuwacunu::hero::config::runtime_key_descriptor_t& d,
    std::string_view declared_type) {
  if (!d.allowed.empty()) {
    const auto options = split_csv(d.allowed);
    if (!options.empty()) {
      std::ostringstream out;
      out << "[";
      for (std::size_t i = 0; i < options.size(); ++i) {
        if (i != 0) out << "|";
        out << options[i];
      }
      out << "]";
      return out.str();
    }
  }
  return cuwacunu::camahjucunu::dsl::default_domain_for_declared_type(
      declared_type);
}

[[nodiscard]] std::string strip_comment_line(std::string_view line,
                                             bool* in_block_comment) {
  bool block = (in_block_comment != nullptr) ? *in_block_comment : false;
  bool in_single = false;
  bool in_double = false;
  std::string out;
  out.reserve(line.size());

  for (std::size_t i = 0; i < line.size();) {
    const char c = line[i];
    const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';

    if (block) {
      if (c == '*' && next == '/') {
        block = false;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }

    if (!in_single && !in_double && c == '/' && next == '*') {
      block = true;
      i += 2;
      continue;
    }

    if (c == '\'' && !in_double) {
      in_single = !in_single;
      out.push_back(c);
      ++i;
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      out.push_back(c);
      ++i;
      continue;
    }
    if ((c == '#' || c == ';') && !in_single && !in_double) {
      break;
    }

    out.push_back(c);
    ++i;
  }

  if (in_block_comment != nullptr) *in_block_comment = block;
  return out;
}

[[nodiscard]] bool parse_runtime_kv_text(
    const std::string& text,
    std::vector<std::array<std::string, 4>>* out_entries,
    std::string* err) {
  std::istringstream input(text);
  std::string raw;
  std::size_t line_no = 0;
  bool in_block_comment = false;
  if (out_entries) out_entries->clear();

  while (std::getline(input, raw)) {
    ++line_no;
    const std::string stripped = trim_ascii(strip_comment_line(raw, &in_block_comment));
    if (stripped.empty()) continue;

    const std::size_t eq = stripped.find('=');
    if (eq == std::string::npos) {
      if (err) {
        std::ostringstream ss;
        ss << "invalid line " << line_no << ": missing '='";
        *err = ss.str();
      }
      return false;
    }

    const std::string lhs = trim_ascii(stripped.substr(0, eq));
    const std::string rhs = trim_ascii(stripped.substr(eq + 1));
    if (lhs.empty()) {
      if (err) {
        std::ostringstream ss;
        ss << "invalid line " << line_no << ": empty key";
        *err = ss.str();
      }
      return false;
    }

    const auto parsed_lhs = cuwacunu::camahjucunu::dsl::parse_latent_lineage_state_lhs(lhs);
    const std::string key = parsed_lhs.key;
    const std::string declared_domain = parsed_lhs.declared_domain;
    const std::string declared_type = parsed_lhs.declared_type;
    if (key.empty()) {
      if (err) {
        std::ostringstream ss;
        ss << "invalid line " << line_no << ": empty key";
        *err = ss.str();
      }
      return false;
    }

    if (out_entries) {
      out_entries->push_back({key, declared_domain, declared_type, rhs});
    }
  }

  return true;
}

using parsed_entry_map_t =
    std::map<std::string, std::array<std::string, 3>, std::less<>>;

[[nodiscard]] parsed_entry_map_t build_entry_map_from_parsed_entries(
    const std::vector<std::array<std::string, 4>>& parsed_entries) {
  parsed_entry_map_t out;
  for (const auto& e : parsed_entries) {
    out[e[0]] = {trim_ascii(e[1]), trim_ascii(e[2]), trim_ascii(e[3])};
  }
  return out;
}

}  // namespace

namespace cuwacunu {
namespace hero {
namespace mcp {

hero_config_store_t::hero_config_store_t(std::string config_path,
                                         std::string global_config_path)
    : config_path_(std::move(config_path)),
      global_config_path_(std::move(global_config_path)) {}

bool hero_config_store_t::load(std::string* err) {
  std::string source_text;
  from_template_ = false;
  dirty_ = false;
  if (!std::filesystem::exists(config_path_)) {
    source_text = std::string(cuwacunu::hero::config::kRuntimeConfigDslTemplateText);
    from_template_ = true;
    dirty_ = true;
  } else if (!read_text_file(config_path_, &source_text, err)) {
    return false;
  }

  std::vector<std::array<std::string, 4>> parsed_entries;
  if (!parse_runtime_kv_text(source_text, &parsed_entries, err)) return false;

  entries_.clear();
  entries_.reserve(parsed_entries.size());
  for (const auto& e : parsed_entries) {
    entries_.push_back(
        {e[0], trim_ascii(e[1]), trim_ascii(e[2]), trim_ascii(e[3])});
  }
  rebuild_index();
  loaded_ = true;
  return true;
}

const std::string& hero_config_store_t::config_path() const { return config_path_; }
const std::string& hero_config_store_t::global_config_path() const {
  return global_config_path_;
}

bool hero_config_store_t::loaded() const { return loaded_; }
bool hero_config_store_t::dirty() const { return dirty_; }
bool hero_config_store_t::from_template() const { return from_template_; }

std::optional<std::string> hero_config_store_t::get_value(std::string_view key) const {
  const auto it = index_by_key_.find(std::string(key));
  if (it == index_by_key_.end()) return std::nullopt;
  return entries_[it->second].value;
}

std::string hero_config_store_t::get_or_default(std::string_view key) const {
  if (const auto value = get_value(key)) return *value;
  if (const auto* d = find_runtime_descriptor(key)) return std::string(d->default_value);
  return {};
}

bool hero_config_store_t::set_value(std::string_view key, std::string_view value,
                                    std::string* err) {
  const auto* d = find_runtime_descriptor(key);
  if (d == nullptr) {
    if (err) *err = "unknown key: " + std::string(key);
    return false;
  }

  const std::string k(key);
  const std::string v = trim_ascii(value);
  const auto it = index_by_key_.find(k);
  if (it == index_by_key_.end()) {
    const std::string declared_type = descriptor_dsl_type(*d);
    entries_.push_back(
        {k, descriptor_dsl_domain(*d, declared_type), declared_type, v});
    rebuild_index();
  } else {
    entries_[it->second].value = v;
    if (entries_[it->second].declared_type.empty()) {
      entries_[it->second].declared_type = descriptor_dsl_type(*d);
    }
    if (entries_[it->second].declared_domain.empty()) {
      entries_[it->second].declared_domain = descriptor_dsl_domain(
          *d, entries_[it->second].declared_type);
    }
  }
  dirty_ = true;
  return true;
}

bool hero_config_store_t::save(std::string* err) {
  bool backup_enabled = true;
  int64_t backup_max_entries = 20;
  const std::string backup_enabled_raw = get_or_default("backup_enabled");
  const std::string backup_max_entries_raw = get_or_default("backup_max_entries");
  const std::string backup_dir_raw = get_or_default("backup_dir");

  if (!backup_enabled_raw.empty() &&
      !parse_bool_value(backup_enabled_raw, &backup_enabled)) {
    if (err) *err = "invalid bool for key backup_enabled: " + backup_enabled_raw;
    return false;
  }
  if (!backup_max_entries_raw.empty() &&
      !parse_int64_value(backup_max_entries_raw, &backup_max_entries)) {
    if (err) {
      *err = "invalid int for key backup_max_entries: " + backup_max_entries_raw;
    }
    return false;
  }

  cuwacunu::hero::runtime_lock::scoped_lock_t global_runtime_lock{};
  if (!cuwacunu::hero::runtime_lock::acquire(&global_runtime_lock, err)) {
    return false;
  }

  if (backup_enabled) {
    if (backup_max_entries < 1) {
      if (err) *err = "backup_max_entries must be >= 1 when backup_enabled=true";
      return false;
    }
    const auto backup_dir = resolve_path_near_config(backup_dir_raw, config_path_);
    if (backup_dir.empty()) {
      if (err) *err = "backup_dir must be non-empty when backup_enabled=true";
      return false;
    }
    if (!backup_previous_file_with_cap(config_path_, backup_dir, backup_max_entries,
                                       err)) {
      return false;
    }
  }

  const std::string rendered = render();
  if (!write_text_file_atomic(config_path_, rendered, err)) return false;
  dirty_ = false;
  from_template_ = false;
  return true;
}

bool hero_config_store_t::preview_save(save_preview_t* out, std::string* err) const {
  if (out == nullptr) {
    if (err) *err = "preview output pointer is null";
    return false;
  }

  out->file_exists = false;
  out->text_changed = false;
  out->has_changes = false;
  out->diff_count = 0;
  out->current_text.clear();
  out->proposed_text.clear();
  out->diffs.clear();

  out->proposed_text = render();

  std::error_code ec;
  const bool exists = std::filesystem::exists(config_path_, ec);
  if (ec) {
    if (err) *err = "failed to inspect config path: " + config_path_;
    return false;
  }
  out->file_exists = exists;
  if (exists && !read_text_file(config_path_, &out->current_text, err)) {
    return false;
  }

  out->text_changed = (out->current_text != out->proposed_text);

  std::vector<std::array<std::string, 4>> parsed_current;
  std::vector<std::array<std::string, 4>> parsed_proposed;
  if (!out->current_text.empty() &&
      !parse_runtime_kv_text(out->current_text, &parsed_current, err)) {
    return false;
  }
  if (!parse_runtime_kv_text(out->proposed_text, &parsed_proposed, err)) {
    return false;
  }

  const parsed_entry_map_t current_map =
      build_entry_map_from_parsed_entries(parsed_current);
  const parsed_entry_map_t proposed_map =
      build_entry_map_from_parsed_entries(parsed_proposed);

  for (const auto& [key, before] : current_map) {
    const auto it_after = proposed_map.find(key);
    if (it_after == proposed_map.end()) {
      out->diffs.push_back(
          {key, "removed", before[0], before[1], before[2], "", "", ""});
      continue;
    }
    const auto& after = it_after->second;
    if (before[0] != after[0] || before[1] != after[1] ||
        before[2] != after[2]) {
      out->diffs.push_back({key,
                            "updated",
                            before[0],
                            before[1],
                            before[2],
                            after[0],
                            after[1],
                            after[2]});
    }
  }

  for (const auto& [key, after] : proposed_map) {
    if (current_map.find(key) != current_map.end()) continue;
    out->diffs.push_back(
        {key, "added", "", "", "", after[0], after[1], after[2]});
  }

  out->diff_count = out->diffs.size();
  out->has_changes = out->text_changed || !out->diffs.empty();
  return true;
}

bool hero_config_store_t::list_backups(std::vector<backup_entry_t>* out,
                                       std::string* err) const {
  if (out == nullptr) {
    if (err) *err = "backup list output pointer is null";
    return false;
  }
  out->clear();

  const std::string backup_dir_raw = get_or_default("backup_dir");
  const auto backup_dir = resolve_path_near_config(backup_dir_raw, config_path_);
  if (backup_dir.empty()) return true;

  std::vector<backup_file_info_t> backups;
  if (!collect_backup_files_sorted_desc(config_path_, backup_dir, &backups, err)) {
    return false;
  }

  out->reserve(backups.size());
  for (const auto& b : backups) {
    out->push_back({b.path.filename().string(), b.path.string()});
  }
  return true;
}

bool hero_config_store_t::rollback_to_backup(std::string_view backup_name_or_path,
                                             std::string* out_selected_path,
                                             std::string* err) {
  const std::string backup_enabled_raw = get_or_default("backup_enabled");
  const std::string backup_max_entries_raw = get_or_default("backup_max_entries");
  const std::string backup_dir_raw = get_or_default("backup_dir");

  bool backup_enabled = true;
  int64_t backup_max_entries = 20;
  if (!backup_enabled_raw.empty() &&
      !parse_bool_value(backup_enabled_raw, &backup_enabled)) {
    if (err) *err = "invalid bool for key backup_enabled: " + backup_enabled_raw;
    return false;
  }
  if (!backup_max_entries_raw.empty() &&
      !parse_int64_value(backup_max_entries_raw, &backup_max_entries)) {
    if (err) {
      *err = "invalid int for key backup_max_entries: " + backup_max_entries_raw;
    }
    return false;
  }

  const auto backup_dir = resolve_path_near_config(backup_dir_raw, config_path_);
  if (backup_dir.empty()) {
    if (err) *err = "backup_dir must be configured for rollback";
    return false;
  }

  std::vector<backup_file_info_t> backups;
  if (!collect_backup_files_sorted_desc(config_path_, backup_dir, &backups, err)) {
    return false;
  }
  if (backups.empty()) {
    if (err) *err = "no backup files found in: " + backup_dir.string();
    return false;
  }

  std::filesystem::path selected_backup;
  const std::string selector = trim_ascii(backup_name_or_path);
  if (selector.empty()) {
    selected_backup = backups.front().path;
  } else {
    std::filesystem::path candidate(selector);
    if (!candidate.is_absolute()) candidate = backup_dir / candidate;
    candidate = candidate.lexically_normal();
    if (!path_is_within(backup_dir, candidate)) {
      if (err) {
        *err = "rollback target must stay inside backup_dir: " +
               backup_dir.string();
      }
      return false;
    }

    std::error_code ec;
    if (std::filesystem::exists(candidate, ec) &&
        std::filesystem::is_regular_file(candidate, ec)) {
      selected_backup = candidate;
    } else {
      bool matched = false;
      for (const auto& b : backups) {
        if (b.path.filename() == selector || b.path.string() == selector) {
          selected_backup = b.path;
          matched = true;
          break;
        }
      }
      if (!matched) {
        if (err) *err = "backup not found: " + selector;
        return false;
      }
    }
  }

  std::string backup_text;
  if (!read_text_file(selected_backup.string(), &backup_text, err)) {
    return false;
  }
  std::vector<std::array<std::string, 4>> parsed_entries;
  if (!parse_runtime_kv_text(backup_text, &parsed_entries, err)) {
    if (err) {
      *err = "backup parse validation failed for " + selected_backup.string() +
             ": " + *err;
    }
    return false;
  }

  if (backup_enabled) {
    if (backup_max_entries < 1) {
      if (err) *err = "backup_max_entries must be >= 1 when backup_enabled=true";
      return false;
    }
    if (!backup_previous_file_with_cap(config_path_, backup_dir, backup_max_entries,
                                       err)) {
      return false;
    }
  }

  if (!write_text_file_atomic(config_path_, backup_text, err)) return false;

  std::string reload_err;
  if (!load(&reload_err)) {
    if (err) {
      *err = "rollback wrote target but reload failed: " + reload_err;
    }
    return false;
  }

  if (out_selected_path) *out_selected_path = selected_backup.string();
  return true;
}

std::vector<std::string> hero_config_store_t::validate() const {
  std::vector<std::string> errors;

  for (const auto& d : cuwacunu::hero::config::kRuntimeKeyDescriptors) {
    const auto val = get_value(d.key);
    if (d.required && (!val.has_value() || trim_ascii(*val).empty())) {
      errors.emplace_back("missing required key: " + std::string(d.key));
      continue;
    }
    if (!val.has_value()) continue;
    const std::string value = trim_ascii(*val);
    if (value.empty()) continue;

    if (d.type == "int") {
      int64_t parsed = 0;
      if (!parse_int64_value(value, &parsed)) {
        errors.emplace_back("invalid int for key " + std::string(d.key) + ": " +
                            value);
      }
    } else if (d.type == "float") {
      double parsed = 0.0;
      if (!parse_double_value(value, &parsed)) {
        errors.emplace_back("invalid float for key " + std::string(d.key) + ": " +
                            value);
      }
    } else if (d.type == "bool") {
      bool parsed = false;
      if (!parse_bool_value(value, &parsed)) {
        errors.emplace_back("invalid bool for key " + std::string(d.key) + ": " +
                            value);
      }
    } else if (d.type == "enum") {
      const std::vector<std::string> allowed = split_csv(d.allowed);
      const std::string lowered = lowercase_copy(value);
      bool matched = false;
      for (const std::string& option : allowed) {
        if (lowered == lowercase_copy(option)) {
          matched = true;
          break;
        }
      }
      if (!matched) {
        errors.emplace_back("invalid enum for key " + std::string(d.key) + ": " +
                            value + " (allowed: " + std::string(d.allowed) + ")");
      }
    }
  }

  {
    const std::string protocol_layer =
        normalized_protocol_layer(get_or_default("protocol_layer"));
    if (protocol_layer != "stdio" && protocol_layer != "https/sse") {
      errors.emplace_back("protocol_layer must be STDIO or HTTPS/SSE");
    }
    if (protocol_layer == "https/sse") {
      errors.emplace_back(
          std::string(cuwacunu::hero::config::kProtocolLayerHttpsSseFailFastMessage));
    }
  }

  {
    const std::string scope = get_or_default("config_scope_root");
    std::error_code ec;
    if (!scope.empty() && !std::filesystem::exists(scope, ec)) {
      errors.emplace_back("config_scope_root does not exist: " + scope);
    }
    if (!scope.empty() && std::filesystem::exists(scope, ec) &&
        !std::filesystem::is_directory(scope, ec)) {
      errors.emplace_back("config_scope_root is not a directory: " + scope);
    }
  }

  {
    const auto validate_csv_paths = [&](std::string_view key) {
      const std::string raw = trim_ascii(get_or_default(std::string(key)));
      if (raw.empty()) {
        errors.emplace_back(std::string(key) + " must be non-empty");
        return;
      }
      std::size_t start = 0;
      bool any = false;
      while (start <= raw.size()) {
        const std::size_t comma = raw.find(',', start);
        const std::string token = trim_ascii(raw.substr(start, comma - start));
        if (!token.empty()) {
          any = true;
          const auto resolved = resolve_path_near_config(token, config_path_);
          std::error_code ec;
          if (resolved.empty() || !std::filesystem::exists(resolved, ec)) {
            errors.emplace_back(std::string(key) +
                                " contains a path that does not exist: " + token);
          } else if (!std::filesystem::is_directory(resolved, ec)) {
            errors.emplace_back(std::string(key) +
                                " contains a path that is not a directory: " +
                                resolved.string());
          }
        }
        if (comma == std::string::npos) break;
        start = comma + 1;
      }
      if (!any) errors.emplace_back(std::string(key) + " must list at least one path");
    };

    validate_csv_paths("default_roots");
    validate_csv_paths("objective_roots");
  }

  {
    const std::string raw = trim_ascii(get_or_default("allowed_extensions"));
    if (raw.empty()) {
      errors.emplace_back("allowed_extensions must be non-empty");
    } else {
      std::size_t start = 0;
      bool any = false;
      while (start <= raw.size()) {
        const std::size_t comma = raw.find(',', start);
        const std::string token = trim_ascii(raw.substr(start, comma - start));
        if (!token.empty()) {
          any = true;
          if (token.size() < 2 || token[0] != '.') {
            errors.emplace_back("allowed_extensions entries must be dot-prefixed: " +
                                token);
          }
        }
        if (comma == std::string::npos) break;
        start = comma + 1;
      }
      if (!any) errors.emplace_back("allowed_extensions must list at least one extension");
    }
  }

  {
    bool allow_local_write = false;
    if (parse_bool_value(get_or_default("allow_local_write"), &allow_local_write) &&
        allow_local_write) {
      const std::string roots = trim_ascii(get_or_default("write_roots"));
      if (roots.empty()) {
        errors.emplace_back(
            "write_roots must be non-empty when allow_local_write=true");
      }
    }
  }

  {
    bool backup_enabled = true;
    int64_t backup_max_entries = 20;
    const std::string enabled_raw = get_or_default("backup_enabled");
    const std::string max_entries_raw = get_or_default("backup_max_entries");
    const std::string backup_dir_raw = trim_ascii(get_or_default("backup_dir"));

    if (!enabled_raw.empty() && !parse_bool_value(enabled_raw, &backup_enabled)) {
      errors.emplace_back("invalid bool for key backup_enabled: " + enabled_raw);
    }
    if (!max_entries_raw.empty() &&
        !parse_int64_value(max_entries_raw, &backup_max_entries)) {
      errors.emplace_back("invalid int for key backup_max_entries: " +
                          max_entries_raw);
    }
    if (backup_enabled && backup_max_entries < 1) {
      errors.emplace_back("backup_max_entries must be >= 1 when backup_enabled=true");
    }
    if (backup_enabled && backup_dir_raw.empty()) {
      errors.emplace_back("backup_dir must be non-empty when backup_enabled=true");
    }
    if (!backup_dir_raw.empty()) {
      const auto backup_dir = resolve_path_near_config(backup_dir_raw, config_path_);
      std::error_code ec;
      if (std::filesystem::exists(backup_dir, ec) &&
          !std::filesystem::is_directory(backup_dir, ec)) {
        errors.emplace_back("backup_dir is not a directory: " + backup_dir.string());
      }
    }
  }

  return errors;
}

std::vector<hero_config_store_t::view_entry_t> hero_config_store_t::entries_snapshot()
    const {
  std::vector<view_entry_t> out;
  out.reserve(entries_.size());
  for (const auto& e : entries_) {
    out.push_back({e.key, e.declared_domain, e.declared_type, e.value});
  }
  return out;
}

std::string hero_config_store_t::render() const {
  std::ostringstream out;
  out << "/* defaults/default.hero.config.dsl (managed by hero_config_mcp) */\n";
  for (const auto& d : cuwacunu::hero::config::kRuntimeKeyDescriptors) {
    std::string declared = descriptor_dsl_type(d);
    std::string declared_domain = descriptor_dsl_domain(d, declared);
    std::string value = std::string(d.default_value);
    const auto it = index_by_key_.find(std::string(d.key));
    if (it != index_by_key_.end()) {
      const auto& e = entries_[it->second];
      if (!e.declared_type.empty()) declared = e.declared_type;
      if (!e.declared_domain.empty()) {
        declared_domain = e.declared_domain;
      } else {
        declared_domain = descriptor_dsl_domain(d, declared);
      }
      value = e.value;
    }
    out << d.key << declared_domain << ":" << declared << " = " << value;
    if (!d.allowed.empty()) out << " # allowed: " << d.allowed;
    out << "\n";
  }

  for (const auto& e : entries_) {
    if (find_runtime_descriptor(e.key) != nullptr) continue;
    if (is_deprecated_runtime_key(e.key)) continue;
    const std::string declared = e.declared_type.empty() ? "str" : e.declared_type;
    const std::string declared_domain = e.declared_domain.empty()
                                            ? cuwacunu::camahjucunu::dsl::
                                                  default_domain_for_declared_type(
                                                      declared)
                                            : e.declared_domain;
    out << e.key << declared_domain << ":" << declared << " = " << e.value
        << "\n";
  }
  return out.str();
}

void hero_config_store_t::rebuild_index() {
  index_by_key_.clear();
  for (std::size_t i = 0; i < entries_.size(); ++i) {
    index_by_key_[entries_[i].key] = i;
  }
}

}  // namespace mcp
}  // namespace hero
}  // namespace cuwacunu
