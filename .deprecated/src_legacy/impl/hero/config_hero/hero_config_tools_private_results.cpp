[[nodiscard]] std::string bool_json(bool v) { return v ? "true" : "false"; }

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count());
}

[[nodiscard]] std::filesystem::path
resolve_path_near_config(std::string_view raw_path,
                         std::string_view config_path) {
  namespace fs = std::filesystem;
  fs::path raw(trim_ascii(raw_path));
  if (raw.empty())
    return raw;

  const auto canonical_or_normalized = [](const fs::path &p) {
    std::error_code ec{};
    const fs::path canonical = fs::weakly_canonical(p, ec);
    return ec ? p.lexically_normal() : canonical;
  };

  if (raw.is_absolute()) {
    return canonical_or_normalized(raw);
  }

  std::vector<fs::path> candidates{};
  candidates.push_back(raw);

  const fs::path config_parent = fs::path(config_path).parent_path();
  if (!config_parent.empty())
    candidates.push_back(config_parent / raw);

  constexpr std::string_view kRepoRoot = "/cuwacunu";
  candidates.push_back(fs::path(kRepoRoot) / raw);

  for (const auto &candidate : candidates) {
    std::error_code ec{};
    if (fs::exists(candidate, ec) && !ec)
      return canonical_or_normalized(candidate);
  }
  return canonical_or_normalized(candidates.front());
}

[[nodiscard]] std::string
normalized_path_key(const std::filesystem::path &path) {
  if (path.empty())
    return {};
  return path.generic_string();
}

[[nodiscard]] bool sha256_hex_file(const std::filesystem::path &path,
                                   std::string *out_hex,
                                   std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out_hex) {
    if (out_error)
      *out_error = "sha256 output pointer is null";
    return false;
  }
  out_hex->clear();

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (out_error)
      *out_error = "cannot open file for sha256: " + path.string();
    return false;
  }

  std::string content((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());
  if (in.bad()) {
    if (out_error)
      *out_error = "cannot read file for sha256: " + path.string();
    return false;
  }
  return sha256_hex_text(content, out_hex, out_error);
}

[[nodiscard]] bool sha256_hex_text(std::string_view text, std::string *out_hex,
                                   std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out_hex) {
    if (out_error)
      *out_error = "sha256 output pointer is null";
    return false;
  }
  out_hex->clear();

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx) {
    if (out_error)
      *out_error = "cannot allocate EVP context";
    return false;
  }
  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
    EVP_MD_CTX_free(ctx);
    if (out_error)
      *out_error = "EVP_DigestInit_ex failed";
    return false;
  }
  if (!text.empty() &&
      EVP_DigestUpdate(ctx, text.data(),
                       static_cast<std::size_t>(text.size())) != 1) {
    EVP_MD_CTX_free(ctx);
    if (out_error)
      *out_error = "EVP_DigestUpdate failed";
    return false;
  }

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
    EVP_MD_CTX_free(ctx);
    if (out_error)
      *out_error = "EVP_DigestFinal_ex failed";
    return false;
  }
  EVP_MD_CTX_free(ctx);

  static constexpr std::array<char, 16> kHex{'0', '1', '2', '3', '4', '5',
                                             '6', '7', '8', '9', 'a', 'b',
                                             'c', 'd', 'e', 'f'};
  out_hex->resize(static_cast<std::size_t>(digest_len) * 2);
  for (std::size_t i = 0; i < static_cast<std::size_t>(digest_len); ++i) {
    const unsigned char b = digest[i];
    (*out_hex)[2 * i + 0] = kHex[(b >> 4) & 0x0F];
    (*out_hex)[2 * i + 1] = kHex[b & 0x0F];
  }
  return true;
}

[[nodiscard]] bool key_looks_like_component_dsl_path(std::string_view key) {
  const std::string lowered = lowercase_copy(trim_ascii(key));
  if (lowered.find("dsl") == std::string::npos)
    return false;
  if (lowered.find("component") != std::string::npos)
    return true;
  if (lowered.find("hashimyei") != std::string::npos)
    return true;
  if (lowered.find("tsi.") != std::string::npos)
    return true;
  if (lowered.size() >= 9 &&
      lowered.compare(lowered.size() - 9, 9, "_dsl_file") == 0) {
    return true;
  }
  return false;
}

struct touched_component_dsl_diff_t {
  std::string key{};
  std::string action{};
  std::string before_value{};
  std::string after_value{};
  std::string raw_path{};
  std::filesystem::path resolved_path{};
  std::string resolved_norm{};
  std::string dsl_sha256_hex{};
  std::string error{};
};

struct component_identity_receipt_entry_t {
  std::string component_dsl_key{};
  std::string action{};
  std::string dsl_path{};
  std::string dsl_sha256_hex{};
  std::string status{};
  std::string message{};
};

[[nodiscard]] std::string build_component_identity_receipt_json_impl(
    const cuwacunu::hero::mcp::hero_config_store_t::save_preview_t &preview,
    std::string_view config_path, std::string_view global_config_path) {
  (void)global_config_path;
  std::vector<touched_component_dsl_diff_t> touched{};
  touched.reserve(preview.diffs.size());
  for (const auto &diff : preview.diffs) {
    if (!key_looks_like_component_dsl_path(diff.key))
      continue;
    touched_component_dsl_diff_t t{};
    t.key = diff.key;
    t.action = diff.action;
    t.before_value = diff.before_value;
    t.after_value = diff.after_value;
    t.raw_path = trim_ascii(!diff.after_value.empty() ? diff.after_value
                                                      : diff.before_value);
    if (trim_ascii(t.action) == "removed") {
      t.error =
          "component DSL key was removed; no component identity update applied";
      touched.push_back(std::move(t));
      continue;
    }
    if (t.raw_path.empty()) {
      t.error = "component DSL path is empty";
      touched.push_back(std::move(t));
      continue;
    }
    t.resolved_path = resolve_path_near_config(t.raw_path, config_path);
    t.resolved_norm = normalized_path_key(t.resolved_path);
    if (t.resolved_norm.empty()) {
      t.error = "cannot resolve component DSL path";
      touched.push_back(std::move(t));
      continue;
    }
    std::string sha_error;
    if (!sha256_hex_file(t.resolved_path, &t.dsl_sha256_hex, &sha_error)) {
      t.error = sha_error;
    }
    touched.push_back(std::move(t));
  }

  std::sort(touched.begin(), touched.end(),
            [](const touched_component_dsl_diff_t &a,
               const touched_component_dsl_diff_t &b) {
              if (a.key != b.key)
                return a.key < b.key;
              if (a.resolved_norm != b.resolved_norm) {
                return a.resolved_norm < b.resolved_norm;
              }
              return a.raw_path < b.raw_path;
            });
  touched.erase(std::unique(touched.begin(), touched.end(),
                            [](const touched_component_dsl_diff_t &a,
                               const touched_component_dsl_diff_t &b) {
                              return a.key == b.key &&
                                     a.resolved_norm == b.resolved_norm &&
                                     a.action == b.action &&
                                     a.raw_path == b.raw_path;
                            }),
                touched.end());

  const std::uint64_t generated_at_ms = now_ms_utc();

  std::vector<component_identity_receipt_entry_t> receipts{};
  receipts.reserve(touched.size());

  std::size_t applied_count = 0;
  std::size_t skipped_count = 0;
  std::size_t error_count = 0;

  for (const auto &t : touched) {
    if (!t.error.empty()) {
      receipts.push_back(component_identity_receipt_entry_t{
          .component_dsl_key = t.key,
          .action = t.action,
          .dsl_path = t.resolved_norm.empty() ? t.raw_path : t.resolved_norm,
          .dsl_sha256_hex = t.dsl_sha256_hex,
          .status = "error",
          .message = t.error,
      });
      ++error_count;
      continue;
    }

    receipts.push_back(component_identity_receipt_entry_t{
        .component_dsl_key = t.key,
        .action = t.action,
        .dsl_path = t.resolved_norm,
        .dsl_sha256_hex = t.dsl_sha256_hex,
        .status = "deferred",
        .message = "component storage identity is derived during contract-wave "
                   "staging; config save does not mint Hashimyei revisions",
    });
    ++skipped_count;
  }

  std::ostringstream out;
  out << "{"
      << "\"generated_at_ms\":" << generated_at_ms << ","
      << "\"component_identity_reviewed\":" << bool_json(!touched.empty())
      << ","
      << "\"path\":" << json_quote(config_path) << ","
      << "\"touched_component_dsl_keys\":[";
  for (std::size_t i = 0; i < touched.size(); ++i) {
    if (i != 0)
      out << ",";
    out << json_quote(touched[i].key);
  }
  out << "],\"applied_count\":" << applied_count
      << ",\"deferred_count\":" << skipped_count
      << ",\"error_count\":" << error_count << ",\"receipts\":[";
  for (std::size_t i = 0; i < receipts.size(); ++i) {
    if (i != 0)
      out << ",";
    out << "{"
        << "\"component_dsl_key\":" << json_quote(receipts[i].component_dsl_key)
        << ",\"action\":" << json_quote(receipts[i].action)
        << ",\"dsl_path\":" << json_quote(receipts[i].dsl_path)
        << ",\"dsl_sha256_hex\":" << json_quote(receipts[i].dsl_sha256_hex)
        << ",\"status\":" << json_quote(receipts[i].status)
        << ",\"message\":" << json_quote(receipts[i].message) << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string detect_error_tag(std::string_view message) {
  const std::size_t colon = message.find(':');
  if (colon == std::string_view::npos || colon == 0)
    return {};
  const std::string_view candidate = message.substr(0, colon);
  for (const char c : candidate) {
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
      continue;
    }
    return {};
  }
  if (candidate.find("E_") != 0)
    return {};
  return std::string(candidate);
}

[[nodiscard]] std::string
make_text_content_result_json(std::string_view text, bool is_error,
                              int error_code, std::string_view error_tag) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(text)
      << "}],\"isError\":" << bool_json(is_error);
  if (is_error) {
    out << ",\"structuredContent\":{\"ok\":false"
        << ",\"error_code\":" << error_code
        << ",\"error_tag\":" << json_quote(error_tag)
        << ",\"message\":" << json_quote(text) << "}";
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string
make_tool_success_result_json(std::string_view tool_name,
                              std::string_view structured_content_json) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":"
      << json_quote(std::string("tool ") + std::string(tool_name) + " executed")
      << "}],\"structuredContent\":" << structured_content_json
      << ",\"isError\":false}";
  return out.str();
}

[[nodiscard]] std::string
build_status_result(const cuwacunu::hero::mcp::hero_config_store_t &store) {
  const auto errors = store.validate();
  std::ostringstream out;
  out << "{";
  out << "\"valid\":" << bool_json(errors.empty()) << ",";
  out << "\"config_path\":" << json_quote(store.config_path()) << ",";
  out << "\"dirty\":" << bool_json(store.dirty()) << ",";
  out << "\"from_template\":" << bool_json(store.from_template()) << ",";
  out << "\"protocol_layer\":"
      << json_quote(store.get_or_default("protocol_layer")) << ",";
  out << "\"backup_enabled\":"
      << json_quote(store.get_or_default("backup_enabled")) << ",";
  out << "\"backup_dir\":" << json_quote(store.get_or_default("backup_dir"))
      << ",";
  out << "\"backup_max_entries\":"
      << json_quote(store.get_or_default("backup_max_entries")) << ",";
  out << "\"optim_backup_enabled\":"
      << json_quote(store.get_or_default("optim_backup_enabled")) << ",";
  out << "\"optim_backup_dir\":"
      << json_quote(store.get_or_default("optim_backup_dir")) << ",";
  out << "\"optim_backup_max_entries\":"
      << json_quote(store.get_or_default("optim_backup_max_entries")) << ",";
  out << "\"errors\":[";
  for (std::size_t i = 0; i < errors.size(); ++i) {
    if (i != 0)
      out << ",";
    out << json_quote(errors[i]);
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string build_save_preview_result(
    const cuwacunu::hero::mcp::hero_config_store_t::save_preview_t &preview,
    bool include_text) {
  const bool format_only = preview.text_changed && preview.diffs.empty();
  std::ostringstream out;
  out << "{";
  out << "\"file_exists\":" << bool_json(preview.file_exists) << ",";
  out << "\"text_changed\":" << bool_json(preview.text_changed) << ",";
  out << "\"format_only\":" << bool_json(format_only) << ",";
  out << "\"has_changes\":" << bool_json(preview.has_changes) << ",";
  out << "\"diff_count\":" << preview.diff_count << ",";
  out << "\"diffs\":[";
  for (std::size_t i = 0; i < preview.diffs.size(); ++i) {
    const auto &d = preview.diffs[i];
    if (i != 0)
      out << ",";
    out << "{"
        << "\"key\":" << json_quote(d.key) << ","
        << "\"action\":" << json_quote(d.action) << ","
        << "\"before_domain\":" << json_quote(d.before_declared_domain) << ","
        << "\"before_type\":" << json_quote(d.before_declared_type) << ","
        << "\"before_value\":" << json_quote(d.before_value) << ","
        << "\"after_domain\":" << json_quote(d.after_declared_domain) << ","
        << "\"after_type\":" << json_quote(d.after_declared_type) << ","
        << "\"after_value\":" << json_quote(d.after_value) << "}";
  }
  out << "]";
  if (include_text) {
    out << ",\"current_text\":" << json_quote(preview.current_text);
    out << ",\"proposed_text\":" << json_quote(preview.proposed_text);
  }
  out << "}";
  return out.str();
}

constexpr hero_config_tool_descriptor_t kHeroConfigTools[] = {
#define HERO_CONFIG_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER)        \
  hero_config_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
#include "hero/config_hero/hero_config_tools.def"
#undef HERO_CONFIG_TOOL
};
