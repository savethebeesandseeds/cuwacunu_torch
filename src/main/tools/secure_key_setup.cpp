#include "piaabo/dencryption.h"
#include "piaabo/dsecurity.h"
#include "iitepi/config_space_t.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

namespace {

struct key_target_t {
  const char* id;
  const char* label;
  const char* section;
  const char* key;
};

constexpr key_target_t kTargets[] = {
    {"exchange_real", "Exchange API key (REAL)", "REAL_EXCHANGE",
     "EXCHANGE_api_filename"},
    {"exchange_test", "Exchange API key (TEST)", "TEST_EXCHANGE",
     "EXCHANGE_api_filename"},
    {"openai_real", "OpenAI API key (REAL_HERO)", "REAL_HERO",
     "OPENAI_api_filename"},
};

using parsed_section_t = std::map<std::string, std::string>;
using parsed_config_t = std::map<std::string, parsed_section_t>;

struct umask_guard_t {
  explicit umask_guard_t(mode_t mask) : old_(umask(mask)) {}
  ~umask_guard_t() { umask(old_); }
  mode_t old_;
};

void wipe_string(std::string* s) {
  if (s == nullptr) return;
  if (!s->empty()) {
    volatile char* p = s->data();
    for (std::size_t i = 0; i < s->size(); ++i) p[i] = '\0';
  }
  s->clear();
}

std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

std::string lower_ascii_copy(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

std::vector<std::string> split_csv_lower_trimmed(const std::string& csv) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : csv) {
    if (c == ',') {
      cur = lower_ascii_copy(trim_ascii_ws_copy(cur));
      if (!cur.empty()) out.push_back(cur);
      cur.clear();
      continue;
    }
    cur.push_back(c);
  }
  cur = lower_ascii_copy(trim_ascii_ws_copy(cur));
  if (!cur.empty()) out.push_back(cur);
  return out;
}

bool consume_arg(int argc, char** argv, int* i, const char* flag,
                 std::string* out) {
  if (std::strcmp(argv[*i], flag) != 0) return false;
  if ((*i + 1) >= argc) {
    std::cerr << "error: missing value for " << flag << "\n";
    return false;
  }
  ++(*i);
  *out = argv[*i];
  return true;
}

bool read_hidden_line(const char* prompt, std::string* out) {
  if (out == nullptr) return false;
  out->clear();
  if (!isatty(STDIN_FILENO)) {
    std::cerr << "error: interactive TTY is required\n";
    return false;
  }

  struct termios oldt {};
  if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
    std::cerr << "error: tcgetattr failed\n";
    return false;
  }
  struct termios newt = oldt;
  newt.c_lflag &= static_cast<tcflag_t>(~ECHO);
  if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0) {
    std::cerr << "error: tcsetattr failed\n";
    return false;
  }

  std::cout << prompt << std::flush;
  std::getline(std::cin, *out);
  std::cout << "\n";

  if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) != 0) {
    std::cerr << "error: failed to restore terminal settings\n";
    return false;
  }

  if (!std::cin.good()) {
    std::cerr << "error: failed to read input\n";
    return false;
  }
  return true;
}

bool read_yes_no(const std::string& prompt, bool default_yes = false) {
  std::cout << prompt << (default_yes ? " [Y/n]: " : " [y/N]: ") << std::flush;
  std::string line;
  std::getline(std::cin, line);
  if (!std::cin.good()) return false;
  line = lower_ascii_copy(trim_ascii_ws_copy(line));
  if (line.empty()) return default_yes;
  if (line == "y" || line == "yes") return true;
  if (line == "n" || line == "no") return false;
  return default_yes;
}

void print_usage(const char* argv0) {
  std::cout
      << "Usage:\n"
      << "  " << argv0
      << " [--global-config <path>] [--only <csv>] [--skip <csv>] [--yes] [--skip-existing] [--dry-run]\n"
      << "  " << argv0 << " --list-targets\n"
      << "\n"
      << "Targets:\n"
      << "  exchange_real, exchange_test, openai_real\n"
      << "\n"
      << "Examples:\n"
      << "  " << argv0 << "\n"
      << "  " << argv0 << " --only openai_real\n"
      << "  " << argv0 << " --skip exchange_test --skip-existing\n"
      << "  " << argv0 << " --only exchange_real,openai_real --dry-run\n";
}

void list_targets() {
  std::cout << "Available targets:\n";
  for (const auto& t : kTargets) {
    std::cout << "  - " << t.id << " : " << t.label << " [" << t.section << "."
              << t.key << "]\n";
  }
}

bool is_known_target_id(const std::string& id) {
  for (const auto& t : kTargets) {
    if (id == t.id) return true;
  }
  return false;
}

const key_target_t* find_target_by_id(const std::string& id) {
  for (const auto& t : kTargets) {
    if (id == t.id) return &t;
  }
  return nullptr;
}

bool write_blob_atomic(const std::filesystem::path& out_path,
                       const unsigned char* data,
                       std::size_t data_len,
                       std::string* error) {
  if (error) error->clear();
  if (data == nullptr || data_len == 0) {
    if (error) *error = "empty blob";
    return false;
  }

  std::error_code ec;
  const std::filesystem::path dir = out_path.parent_path();
  if (!dir.empty()) {
    std::filesystem::create_directories(dir, ec);
    if (ec) {
      if (error) *error = "create_directories failed: " + ec.message();
      return false;
    }
  }

  std::filesystem::path tmp = out_path;
  tmp += ".tmp." + std::to_string(static_cast<long long>(::getpid()));

  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (error) *error = "cannot open temp file for write";
      return false;
    }
    out.write(reinterpret_cast<const char*>(data),
              static_cast<std::streamsize>(data_len));
    if (!out) {
      if (error) *error = "failed writing temp file";
      out.close();
      std::filesystem::remove(tmp, ec);
      return false;
    }
  }

  if (chmod(tmp.c_str(), S_IRUSR | S_IWUSR) != 0) {
    if (error) *error = "chmod 0600 failed on temp file";
    std::filesystem::remove(tmp, ec);
    return false;
  }

  std::filesystem::rename(tmp, out_path, ec);
  if (ec) {
    if (error) *error = "rename failed: " + ec.message();
    std::filesystem::remove(tmp, ec);
    return false;
  }

  if (chmod(out_path.c_str(), S_IRUSR | S_IWUSR) != 0) {
    if (error) *error = "chmod 0600 failed on output file";
    return false;
  }

  return true;
}

std::string strip_comment(std::string line) {
  bool in_single = false;
  bool in_double = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (c == '\'' && !in_double) {
      in_single = !in_single;
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      continue;
    }
    if (!in_single && !in_double && (c == '#' || c == ';')) {
      line.resize(i);
      break;
    }
  }
  return line;
}

std::filesystem::path resolve_global_config_path(const std::string& global_config_path) {
  return std::filesystem::path(global_config_path);
}

bool parse_global_config(const std::filesystem::path& path,
                         parsed_config_t* out,
                         std::string* error) {
  if (out == nullptr) return false;
  out->clear();
  if (error) error->clear();

  std::ifstream in(path);
  if (!in) {
    if (error) *error = "cannot open config file";
    return false;
  }

  std::string current;
  std::string raw;
  while (std::getline(in, raw)) {
    std::string line = trim_ascii_ws_copy(strip_comment(raw));
    if (line.empty()) continue;
    if (line.front() == '[' && line.back() == ']') {
      current = trim_ascii_ws_copy(line.substr(1, line.size() - 2));
      (*out)[current];
      continue;
    }
    const std::size_t pos = line.find('=');
    if (pos == std::string::npos) continue;
    const std::string key = trim_ascii_ws_copy(line.substr(0, pos));
    const std::string value = trim_ascii_ws_copy(line.substr(pos + 1));
    if (!current.empty() && !key.empty()) {
      (*out)[current][key] = value;
    }
  }
  return true;
}

bool lookup_value(const parsed_config_t& cfg,
                  const std::string& section,
                  const std::string& key,
                  std::string* out) {
  if (out == nullptr) return false;
  const auto sec_it = cfg.find(section);
  if (sec_it == cfg.end()) return false;
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end()) return false;
  *out = key_it->second;
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  std::string global_config_path = DEFAULT_GLOBAL_CONFIG_PATH;
  std::string only_csv;
  std::string skip_csv;
  bool assume_yes = false;
  bool skip_existing = false;
  bool dry_run = false;
  bool list_only = false;

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
      print_usage(argv[0]);
      return 0;
    }
    if (std::strcmp(argv[i], "--list-targets") == 0) {
      list_only = true;
      continue;
    }
    if (std::strcmp(argv[i], "--yes") == 0) {
      assume_yes = true;
      continue;
    }
    if (std::strcmp(argv[i], "--skip-existing") == 0) {
      skip_existing = true;
      continue;
    }
    if (std::strcmp(argv[i], "--dry-run") == 0) {
      dry_run = true;
      continue;
    }
    if (consume_arg(argc, argv, &i, "--global-config", &global_config_path)) continue;
    if (consume_arg(argc, argv, &i, "--only", &only_csv)) continue;
    if (consume_arg(argc, argv, &i, "--skip", &skip_csv)) continue;
    std::cerr << "error: unknown argument: " << argv[i] << "\n";
    print_usage(argv[0]);
    return 2;
  }

  if (list_only) {
    list_targets();
    return 0;
  }

  std::vector<std::string> only_ids = split_csv_lower_trimmed(only_csv);
  std::vector<std::string> skip_ids = split_csv_lower_trimmed(skip_csv);

  for (const auto& id : only_ids) {
    if (!is_known_target_id(id)) {
      std::cerr << "error: unknown id in --only: " << id << "\n";
      return 2;
    }
  }
  for (const auto& id : skip_ids) {
    if (!is_known_target_id(id)) {
      std::cerr << "error: unknown id in --skip: " << id << "\n";
      return 2;
    }
  }

  const std::unordered_set<std::string> only_set(only_ids.begin(), only_ids.end());
  const std::unordered_set<std::string> skip_set(skip_ids.begin(), skip_ids.end());

  std::vector<const key_target_t*> selected;
  for (const auto& t : kTargets) {
    if (!only_set.empty() && only_set.find(t.id) == only_set.end()) continue;
    if (skip_set.find(t.id) != skip_set.end()) continue;
    selected.push_back(&t);
  }

  if (selected.empty()) {
    std::cout << "No targets selected. Nothing to do.\n";
    return 0;
  }

  const std::filesystem::path config_file =
      resolve_global_config_path(global_config_path);
  const std::filesystem::path config_root = config_file.parent_path();

  parsed_config_t parsed_config;
  std::string parse_error;
  if (!parse_global_config(config_file, &parsed_config, &parse_error)) {
    std::cerr << "error: failed to parse " << config_file << " (" << parse_error
              << ")\n";
    return 2;
  }

  umask_guard_t umask_guard(0077);

  std::string passphrase_1;
  std::string passphrase_2;
  if (!read_hidden_line("Passphrase: ", &passphrase_1)) return 3;
  if (!read_hidden_line("Passphrase (confirm): ", &passphrase_2)) {
    wipe_string(&passphrase_1);
    return 3;
  }
  if (passphrase_1 != passphrase_2) {
    std::cerr << "error: passphrases do not match\n";
    wipe_string(&passphrase_1);
    wipe_string(&passphrase_2);
    return 3;
  }

  int updated = 0;
  int skipped = 0;
  for (const key_target_t* t : selected) {
    std::string raw_path;
    if (!lookup_value(parsed_config, t->section, t->key, &raw_path)) {
      wipe_string(&passphrase_1);
      wipe_string(&passphrase_2);
      std::cerr << "error: missing [" << t->section << "]." << t->key
                << " in " << config_file << "\n";
      return 2;
    }

    std::filesystem::path out_path(raw_path);
    if (!out_path.is_absolute()) out_path = config_root / out_path;

    std::error_code ec;
    const bool exists_and_non_empty =
        std::filesystem::exists(out_path, ec) &&
        std::filesystem::is_regular_file(out_path, ec) &&
        std::filesystem::file_size(out_path, ec) > 0;

    if (skip_existing && exists_and_non_empty) {
      std::cout << "[skip-existing] " << t->id << " -> " << out_path << "\n";
      ++skipped;
      continue;
    }

    if (!assume_yes) {
      const bool proceed = read_yes_no(
          "Update " + std::string(t->id) + " at " + out_path.string() + "?",
          false);
      if (!proceed) {
        ++skipped;
        continue;
      }
    }

    std::string key_value_1;
    std::string key_value_2;
    for (;;) {
      if (!read_hidden_line(("Key value (" + std::string(t->id) + "): ").c_str(),
                            &key_value_1)) {
        wipe_string(&passphrase_1);
        wipe_string(&passphrase_2);
        return 3;
      }
      if (!read_hidden_line(
              ("Key value (confirm " + std::string(t->id) + "): ").c_str(),
              &key_value_2)) {
        wipe_string(&key_value_1);
        wipe_string(&passphrase_1);
        wipe_string(&passphrase_2);
        return 3;
      }
      if (key_value_1.empty()) {
        std::cout << "[skip-empty] " << t->id << "\n";
        ++skipped;
        break;
      }
      if (key_value_1 != key_value_2) {
        std::cerr << "warning: key values do not match, retrying " << t->id
                  << "\n";
        wipe_string(&key_value_1);
        wipe_string(&key_value_2);
        continue;
      }

      if (dry_run) {
        std::cout << "[dry-run] would update " << t->id << " -> " << out_path
                  << "\n";
        ++updated;
        wipe_string(&key_value_1);
        wipe_string(&key_value_2);
        break;
      }

      std::size_t encrypted_len = 0;
      unsigned char* encrypted =
          cuwacunu::piaabo::dencryption::aead_encrypt_blob(
              reinterpret_cast<const unsigned char*>(key_value_1.data()),
              key_value_1.size(), passphrase_1.c_str(), encrypted_len);
      wipe_string(&key_value_1);
      wipe_string(&key_value_2);

      if (encrypted == nullptr || encrypted_len == 0) {
        wipe_string(&passphrase_1);
        wipe_string(&passphrase_2);
        std::cerr << "error: failed to encrypt key material for " << t->id
                  << "\n";
        return 4;
      }

      std::string write_error;
      const bool ok =
          write_blob_atomic(out_path, encrypted, encrypted_len, &write_error);
      cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(encrypted,
                                                                encrypted_len);
      if (!ok) {
        wipe_string(&passphrase_1);
        wipe_string(&passphrase_2);
        std::cerr << "error: failed to write " << out_path << " (" << write_error
                  << ")\n";
        return 4;
      }

      std::cout << "[updated] " << t->id << " -> " << out_path << "\n";
      ++updated;
      break;
    }
  }

  wipe_string(&passphrase_1);
  wipe_string(&passphrase_2);

  std::cout << "Done. updated=" << updated << " skipped=" << skipped
            << " total=" << selected.size() << "\n";
  return 0;
}
