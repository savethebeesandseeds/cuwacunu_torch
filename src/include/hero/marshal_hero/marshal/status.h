// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "hero/marshal_hero/marshal/dispatch_receipt.h"

namespace cuwacunu::hero::marshal {

struct marshal_status_t {
  std::string schema_version{"kikijyeba.marshal.status.v1"};
  std::filesystem::path receipt_root{};
  std::size_t recent_receipt_count{0};
  std::string last_accepted_dry_run_receipt_id{};
  std::string last_execution_handoff_receipt_id{};
  std::string last_refusal_reason{};
  bool runtime_default_dry_run{true};
  bool runtime_allow_execute{false};
  bool runtime_allow_train_execute{false};
  bool lattice_advice_surface_available{false};
  std::string non_authority_statement{
      k_marshal_dispatch_non_authority_statement};
};

[[nodiscard]] inline marshal_status_t make_marshal_status(
    const std::filesystem::path &receipt_root,
    const std::vector<marshal_dispatch_receipt_t> &recent_receipts,
    const marshal_runtime_policy_snapshot_t &policy,
    bool lattice_advice_surface_available) {
  marshal_status_t status{};
  status.receipt_root = receipt_root;
  status.recent_receipt_count = recent_receipts.size();
  status.runtime_default_dry_run = policy.default_dry_run;
  status.runtime_allow_execute = policy.allow_execute;
  status.runtime_allow_train_execute = policy.allow_train_execute;
  status.lattice_advice_surface_available = lattice_advice_surface_available;
  for (const auto &receipt : recent_receipts) {
    if (receipt.policy_decision == "accepted" &&
        receipt.receipt_kind == "dry_run") {
      status.last_accepted_dry_run_receipt_id = receipt.receipt_id;
    }
    if (receipt.policy_decision == "accepted" &&
        receipt.receipt_kind == "execute") {
      status.last_execution_handoff_receipt_id = receipt.receipt_id;
    }
    if (receipt.policy_decision != "accepted" &&
        !receipt.refusal_reasons.empty()) {
      status.last_refusal_reason = receipt.refusal_reasons.back();
    }
  }
  return status;
}

[[nodiscard]] inline std::string
canonical_marshal_status_text(const marshal_status_t &status) {
  std::ostringstream out;
  detail::append_kv(out, "schema_version", status.schema_version);
  detail::append_kv(out, "receipt_root",
                    detail::normalize_path_text(status.receipt_root.string()));
  detail::append_kv(out, "recent_receipt_count",
                    std::to_string(status.recent_receipt_count));
  detail::append_kv(out, "last_accepted_dry_run_receipt_id",
                    status.last_accepted_dry_run_receipt_id);
  detail::append_kv(out, "last_execution_handoff_receipt_id",
                    status.last_execution_handoff_receipt_id);
  detail::append_kv(out, "last_refusal_reason", status.last_refusal_reason);
  detail::append_kv(out, "runtime_default_dry_run",
                    detail::bool_text(status.runtime_default_dry_run));
  detail::append_kv(out, "runtime_allow_execute",
                    detail::bool_text(status.runtime_allow_execute));
  detail::append_kv(out, "runtime_allow_train_execute",
                    detail::bool_text(status.runtime_allow_train_execute));
  detail::append_kv(out, "lattice_advice_surface_available",
                    detail::bool_text(status.lattice_advice_surface_available));
  detail::append_kv(out, "non_authority_statement",
                    status.non_authority_statement);
  return out.str();
}

} // namespace cuwacunu::hero::marshal
