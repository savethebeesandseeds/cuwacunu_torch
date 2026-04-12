#include "hero_config_tools_internal.h"

#include <cctype>
#include <chrono>
#include <sstream>

#include "hero/runtime_hero/runtime_campaign.h"
#include "hero/runtime_hero/runtime_job.h"

namespace cuwacunu::hero::mcp::detail {

struct dev_nuke_backup_snapshot_t {
  bool created{false};
  std::filesystem::path snapshot_root{};
  std::vector<std::filesystem::path> backed_up_store_roots{};
  std::vector<std::filesystem::path> backed_up_catalog_paths{};
};

[[nodiscard]] bool enforce_runtime_reset_targets_write_policy(
    const hero_config_store_t& store,
    const cuwacunu::hero::runtime_dev::runtime_reset_targets_t& targets,
    std::string* out_error) {
  (void)store;
  if (out_error) out_error->clear();
  if (targets.runtime_root.empty()) {
    if (out_error) {
      *out_error =
          make_write_policy_error("runtime reset targets missing runtime_root");
    }
    return false;
  }
  if (!cuwacunu::hero::runtime_dev::detail::looks_safe_to_remove_tree(
          targets.runtime_root)) {
    if (out_error) {
      *out_error = make_write_policy_error("runtime_root is not safe to reset: " +
                                           targets.runtime_root.string());
    }
    return false;
  }

  const auto check_target = [&](const std::filesystem::path& path,
                                std::string_view label) -> bool {
    if (path.empty()) return true;
    if (cuwacunu::hero::runtime_dev::detail::path_within(targets.runtime_root,
                                                         path)) {
      return true;
    }
    if (out_error) {
      *out_error =
          make_write_policy_error(std::string(label) + " escapes runtime_root: " +
                                  path.lexically_normal().string());
    }
    return false;
  };

  if (!check_target(targets.runtime_campaigns_root, "runtime campaigns root")) {
    return false;
  }
  for (const auto& root : targets.store_roots) {
    if (!check_target(root, "runtime store root")) return false;
  }
  for (const auto& catalog : targets.catalog_paths) {
    if (!check_target(catalog, "runtime catalog path")) return false;
  }
  return true;
}

[[nodiscard]] bool ensure_runtime_reset_targets_idle(
    const cuwacunu::hero::runtime_dev::runtime_reset_targets_t& targets,
    std::string* out_error) {
  if (out_error) out_error->clear();
  if (targets.runtime_campaigns_root.empty()) return true;

  std::vector<std::string> active_jobs{};
  std::string active_error{};
  if (!cuwacunu::hero::runtime::list_active_runtime_job_cursors(
          targets.runtime_campaigns_root, &active_jobs, &active_error)) {
    if (out_error) {
      *out_error = "cannot inspect runtime jobs before reset: " + active_error;
    }
    return false;
  }
  if (!active_jobs.empty()) {
    std::ostringstream out;
    out << "refusing runtime reset while active jobs exist under "
        << targets.runtime_campaigns_root.string() << ": ";
    for (std::size_t i = 0; i < active_jobs.size(); ++i) {
      if (i != 0) out << ", ";
      out << active_jobs[i];
    }
    if (out_error) *out_error = out.str();
    return false;
  }

  std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> campaigns{};
  active_error.clear();
  if (!cuwacunu::hero::runtime::scan_runtime_campaign_records(
          targets.runtime_campaigns_root, &campaigns, &active_error)) {
    if (out_error) {
      *out_error = "cannot inspect runtime campaigns before reset: " +
                   active_error;
    }
    return false;
  }

  std::vector<std::string> active_campaigns{};
  for (const auto& campaign : campaigns) {
    if (campaign.state == "launching" || campaign.state == "running" ||
        campaign.state == "stopping") {
      active_campaigns.push_back(campaign.campaign_cursor);
    }
  }
  if (!active_campaigns.empty()) {
    std::ostringstream out;
    out << "refusing runtime reset while active campaigns exist under "
        << targets.runtime_campaigns_root.string() << ": ";
    for (std::size_t i = 0; i < active_campaigns.size(); ++i) {
      if (i != 0) out << ", ";
      out << active_campaigns[i];
    }
    if (out_error) *out_error = out.str();
    return false;
  }

  return true;
}

[[nodiscard]] std::string sanitize_dev_nuke_backup_component(
    std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const unsigned char c : value) {
    if (std::isalnum(c) != 0 || c == '.' || c == '_' || c == '-') {
      out.push_back(static_cast<char>(c));
    } else {
      out.push_back('_');
    }
  }
  if (out.empty()) return "runtime_root";
  return out;
}

[[nodiscard]] bool ensure_dev_nuke_backup_snapshot_root(
    const cuwacunu::hero::runtime_dev::runtime_reset_targets_t& targets,
    std::filesystem::path* snapshot_root, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!snapshot_root) {
    if (out_error) *out_error = "missing destination for dev_nuke backup root";
    return false;
  }
  if (!snapshot_root->empty()) return true;

  const std::string runtime_component = sanitize_dev_nuke_backup_component(
      targets.runtime_root.filename().string());
  const std::filesystem::path family_root =
      (targets.runtime_root.parent_path() / ".backups" / "hero.runtime_reset" /
       runtime_component)
          .lexically_normal();

  const auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  const std::string stamp_text = std::to_string(static_cast<long long>(stamp));

  std::filesystem::path candidate = family_root / stamp_text;
  std::error_code ec{};
  int disambiguator = 1;
  while (std::filesystem::exists(candidate, ec)) {
    if (ec) {
      if (out_error) {
        *out_error = "failed to probe dev_nuke backup path: " +
                     candidate.string() + ": " + ec.message();
      }
      return false;
    }
    candidate = family_root / (stamp_text + "." +
                                std::to_string(disambiguator++));
  }

  std::filesystem::create_directories(candidate, ec);
  if (ec) {
    if (out_error) {
      *out_error =
          "failed to create dev_nuke backup directory: " + candidate.string() +
          ": " + ec.message();
    }
    return false;
  }

  *snapshot_root = candidate.lexically_normal();
  return true;
}

[[nodiscard]] bool move_runtime_reset_target_to_backup_snapshot(
    const cuwacunu::hero::runtime_dev::runtime_reset_targets_t& targets,
    const std::filesystem::path& source, std::filesystem::path* snapshot_root,
    std::filesystem::path* out_backup_path, std::string* out_error) {
  if (out_error) out_error->clear();
  if (out_backup_path) out_backup_path->clear();
  if (source.empty()) return true;

  std::error_code ec{};
  if (!std::filesystem::exists(source, ec)) {
    if (ec) {
      if (out_error) {
        *out_error = "failed to inspect runtime reset target for backup: " +
                     source.string() + ": " + ec.message();
      }
      return false;
    }
    return true;
  }
  if (!cuwacunu::hero::runtime_dev::detail::path_within(targets.runtime_root,
                                                        source)) {
    if (out_error) {
      *out_error = "runtime reset backup target escapes runtime_root: " +
                   source.lexically_normal().string();
    }
    return false;
  }

  std::filesystem::path relative = source.lexically_relative(targets.runtime_root);
  if (relative.empty() || relative == ".") {
    if (out_error) {
      *out_error = "failed to derive runtime reset backup path for: " +
                   source.string();
    }
    return false;
  }
  if (!ensure_dev_nuke_backup_snapshot_root(targets, snapshot_root, out_error)) {
    return false;
  }

  const std::filesystem::path backup_path =
      (*snapshot_root / relative).lexically_normal();
  std::filesystem::create_directories(backup_path.parent_path(), ec);
  if (ec) {
    if (out_error) {
      *out_error = "failed to prepare dev_nuke backup path: " +
                   backup_path.parent_path().string() + ": " + ec.message();
    }
    return false;
  }

  std::filesystem::rename(source, backup_path, ec);
  if (ec) {
    if (out_error) {
      *out_error = "failed to move runtime reset target into backup snapshot: " +
                   source.string() + " -> " + backup_path.string() + ": " +
                   ec.message();
    }
    return false;
  }

  if (out_backup_path) *out_backup_path = backup_path;
  return true;
}

[[nodiscard]] bool backup_runtime_reset_targets(
    const cuwacunu::hero::runtime_dev::runtime_reset_targets_t& targets,
    dev_nuke_backup_snapshot_t* out_snapshot, std::string* out_error) {
  if (out_error) out_error->clear();
  dev_nuke_backup_snapshot_t snapshot{};
  std::filesystem::path snapshot_root{};
  std::vector<std::filesystem::path> moved_store_roots{};

  for (const auto& root : targets.store_roots) {
    std::filesystem::path backup_path{};
    if (!move_runtime_reset_target_to_backup_snapshot(
            targets, root, &snapshot_root, &backup_path, out_error)) {
      return false;
    }
    if (!backup_path.empty()) {
      snapshot.backed_up_store_roots.push_back(backup_path);
      moved_store_roots.push_back(root.lexically_normal());
    }
  }

  for (const auto& catalog : targets.catalog_paths) {
    bool already_covered = false;
    for (const auto& moved_root : moved_store_roots) {
      if (cuwacunu::hero::runtime_dev::detail::path_within(moved_root, catalog)) {
        already_covered = true;
        break;
      }
    }
    if (already_covered) continue;

    std::filesystem::path backup_path{};
    if (!move_runtime_reset_target_to_backup_snapshot(
            targets, catalog, &snapshot_root, &backup_path, out_error)) {
      return false;
    }
    if (!backup_path.empty()) {
      snapshot.backed_up_catalog_paths.push_back(backup_path);
    }
  }

  snapshot.created = !snapshot_root.empty();
  snapshot.snapshot_root = snapshot_root;
  if (out_snapshot) *out_snapshot = std::move(snapshot);
  return true;
}

[[nodiscard]] bool handle_tool_dev_nuke_reset(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  (void)tool_name;
  (void)request_json;
  std::string err;
  cuwacunu::hero::runtime_dev::runtime_reset_targets_t targets{};
  if (!cuwacunu::hero::runtime_dev::resolve_runtime_reset_targets_from_global_config(
          store->global_config_path(), &targets, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (!enforce_runtime_reset_targets_write_policy(*store, targets, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  bool dev_nuke_reset_backup_enabled = true;
  if (!read_bool_config_key_or_default(*store, "dev_nuke_reset_backup_enabled",
                                       true,
                                       &dev_nuke_reset_backup_enabled, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (dev_nuke_reset_backup_enabled &&
      !ensure_runtime_reset_targets_idle(targets, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  const bool dirty_before_reset = store->dirty();
  dev_nuke_backup_snapshot_t backup_snapshot{};
  if (dev_nuke_reset_backup_enabled &&
      !backup_runtime_reset_targets(targets, &backup_snapshot, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  cuwacunu::hero::runtime_dev::runtime_reset_result_t reset_result{};
  if (!cuwacunu::hero::runtime_dev::reset_runtime_state(targets, &reset_result,
                                                        &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (!store->load(&err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message =
          "runtime state reset succeeded but config reload failed: " + err;
    }
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{"
        << "\"reset\":true,"
        << "\"config_path\":" << json_quote(store->config_path()) << ","
        << "\"global_config_path\":"
        << json_quote(store->global_config_path()) << ","
        << "\"dirty_before_reset\":" << bool_json(dirty_before_reset) << ","
        << "\"resolved_from_saved_global_config\":true,"
        << "\"hashimyei_hero_dsl_path\":"
        << json_quote(targets.hashimyei_hero_dsl_path.string()) << ","
        << "\"lattice_hero_dsl_path\":"
        << json_quote(targets.lattice_hero_dsl_path.string()) << ","
        << "\"runtime_hero_dsl_path\":"
        << json_quote(targets.runtime_hero_dsl_path.string()) << ","
        << "\"runtime_root\":" << json_quote(targets.runtime_root.string())
        << ","
        << "\"runtime_campaigns_root\":"
        << json_quote(targets.runtime_campaigns_root.string()) << ","
        << "\"marshal_root\":" << json_quote(targets.marshal_root.string()) << ","
        << "\"dev_nuke_reset_backup_enabled\":"
        << bool_json(dev_nuke_reset_backup_enabled) << ","
        << "\"dev_nuke_reset_backup_created\":"
        << bool_json(backup_snapshot.created) << ","
        << "\"dev_nuke_reset_backup_snapshot_path\":"
        << json_quote(backup_snapshot.snapshot_root.string()) << ","
        << "\"dev_nuke_reset_backed_up_store_roots\":"
        << path_vector_json(backup_snapshot.backed_up_store_roots) << ","
        << "\"dev_nuke_reset_backed_up_catalog_paths\":"
        << path_vector_json(backup_snapshot.backed_up_catalog_paths) << ","
        << "\"target_store_roots\":" << path_vector_json(targets.store_roots)
        << ","
        << "\"target_catalog_paths\":"
        << path_vector_json(targets.catalog_paths) << ","
        << "\"removed_store_roots\":"
        << path_vector_json(reset_result.removed_store_roots) << ","
        << "\"removed_catalog_paths\":"
        << path_vector_json(reset_result.removed_catalog_paths) << ","
        << "\"removed_store_entries\":" << reset_result.removed_store_entries
        << ",\"reloaded\":true"
        << "}";
    *out_result_json = out.str();
  }
  return true;
}

}  // namespace cuwacunu::hero::mcp::detail
