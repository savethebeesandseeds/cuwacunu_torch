// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "hero/marshal_hero/marshal/dispatch_operation.h"

namespace cuwacunu::hero::marshal {

struct marshal_batch_preview_item_t {
  std::string item_id{};
  marshal_dispatch_advice_t advice{};
  marshal_dispatch_request_t request{};
  marshal_runtime_wave_snapshot_t active_wave{};
};

struct marshal_batch_preview_response_item_t {
  std::string item_id{};
  std::string advice_digest{};
  std::string request_digest{};
  std::string validation_context_digest{};
  std::string runtime_policy_digest{};
  std::string runtime_wave_digest{};
  marshal_dry_run_dispatch_response_t response{};
};

struct marshal_batch_preview_result_t {
  bool dry_run_only{true};
  std::vector<marshal_batch_preview_response_item_t> items{};
  std::vector<marshal_dry_run_dispatch_response_t> responses{};
  std::string non_authority_statement{
      k_marshal_dispatch_non_authority_statement};
};

[[nodiscard]] inline marshal_batch_preview_result_t run_batch_preview(
    const std::vector<marshal_batch_preview_item_t> &items,
    const marshal_dispatch_validation_context_t &context,
    const marshal_runtime_policy_snapshot_t &policy,
    const marshal_runtime_hero_handoff_options_t &handoff_options = {}) {
  marshal_batch_preview_result_t out{};
  out.responses.reserve(items.size());
  out.items.reserve(items.size());
  for (const auto &item : items) {
    auto response =
        run_marshal_dry_run_dispatch(item.advice, item.request, context, policy,
                                     item.active_wave, handoff_options);
    out.items.push_back(marshal_batch_preview_response_item_t{
        item.item_id, dispatch_advice_digest(item.advice),
        dispatch_request_digest(item.request),
        dispatch_validation_context_digest(context),
        runtime_policy_snapshot_digest(policy),
        runtime_wave_snapshot_digest(item.active_wave), response});
    out.responses.push_back(std::move(response));
  }
  return out;
}

[[nodiscard]] inline bool batch_preview_has_independent_results(
    const marshal_batch_preview_result_t &preview,
    std::size_t expected_item_count) {
  return preview.dry_run_only && preview.items.size() == expected_item_count &&
         preview.responses.size() == expected_item_count;
}

} // namespace cuwacunu::hero::marshal
