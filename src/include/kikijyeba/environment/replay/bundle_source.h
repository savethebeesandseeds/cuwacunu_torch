// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "kikijyeba/environment/replay/artifact_source.h"

namespace cuwacunu::kikijyeba::environment {

class replay_bundle_source_iface_t {
public:
  virtual ~replay_bundle_source_iface_t() = default;

  [[nodiscard]] virtual std::string source_id() const = 0;
  [[nodiscard]] virtual std::optional<replay::replay_episode_bundle_t>
  next_bundle() = 0;
  virtual void reset() = 0;
};

class preloaded_replay_bundle_source_t final
    : public replay_bundle_source_iface_t {
public:
  preloaded_replay_bundle_source_t(
      std::string source_id,
      std::vector<replay::replay_episode_bundle_t> bundles)
      : source_id_(std::move(source_id)), bundles_(std::move(bundles)) {
    if (detail::blank(source_id_)) {
      throw std::runtime_error("[replay_bundle_source] source_id is required");
    }
    if (bundles_.empty()) {
      throw std::runtime_error("[replay_bundle_source] bundles are required");
    }
    for (const auto &bundle : bundles_) {
      replay::validate_replay_episode_bundle(bundle);
    }
  }

  [[nodiscard]] std::string source_id() const override { return source_id_; }

  [[nodiscard]] std::optional<replay::replay_episode_bundle_t>
  next_bundle() override {
    if (cursor_ >= bundles_.size()) {
      return std::nullopt;
    }
    return bundles_[cursor_++];
  }

  void reset() override { cursor_ = 0; }

  [[nodiscard]] std::size_t size() const { return bundles_.size(); }
  [[nodiscard]] std::size_t cursor() const { return cursor_; }

private:
  std::string source_id_{};
  std::vector<replay::replay_episode_bundle_t> bundles_{};
  std::size_t cursor_{0};
};

template <typename KeyT> struct graph_anchor_replay_bundle_record_t {
  episode_spec_t base_spec{};
  protocol::component_stream_wave_t wave{};
  cuwacunu::ujcamei::source::retrieval::dataloader::graph_anchor_cursor_t<KeyT>
      cursor{};
  cuwacunu::ujcamei::source::retrieval::dataloader::graph_anchor_edge_batch_t<
      KeyT>
      batch{};
  replay::replay_frame_build_options_t frame_options{};
  replay::replay_world_options_t world_options{};
  std::string artifact_source_id{};
  std::vector<replay::replay_observation_artifacts_t> observation_artifacts{};
  replay::replay_observation_artifact_options_t artifact_options{};
  bool require_observation_artifacts{false};
};

template <typename KeyT>
class graph_anchor_replay_bundle_source_t final
    : public replay_bundle_source_iface_t {
public:
  graph_anchor_replay_bundle_source_t(
      std::string source_id,
      cuwacunu::kikijyeba::topology::graph::market_graph_t graph,
      std::vector<graph_anchor_replay_bundle_record_t<KeyT>> records)
      : source_id_(std::move(source_id)), graph_(std::move(graph)),
        records_(std::move(records)) {
    if (detail::blank(source_id_)) {
      throw std::runtime_error(
          "[graph_anchor_replay_bundle_source] source_id is required");
    }
    graph_.validate();
    if (records_.empty()) {
      throw std::runtime_error(
          "[graph_anchor_replay_bundle_source] records are required");
    }
  }

  [[nodiscard]] std::string source_id() const override { return source_id_; }

  [[nodiscard]] std::optional<replay::replay_episode_bundle_t>
  next_bundle() override {
    if (cursor_ >= records_.size()) {
      return std::nullopt;
    }
    const auto &record = records_[cursor_++];
    auto base_spec = record.base_spec;
    auto bundle =
        replay::make_replay_episode_bundle_from_graph_anchor_edge_batch(
            std::move(base_spec), record.wave, record.cursor, record.batch,
            graph_, record.frame_options, record.world_options);
    if (!record.observation_artifacts.empty() ||
        record.require_observation_artifacts) {
      auto artifact_options = record.artifact_options;
      artifact_options.require_artifacts_per_frame =
          artifact_options.require_artifacts_per_frame ||
          record.require_observation_artifacts;
      auto artifact_source_id = record.artifact_source_id.empty()
                                    ? source_id_ + ".observation_artifacts"
                                    : record.artifact_source_id;
      replay::keyed_replay_observation_artifact_source_t artifact_source(
          std::move(artifact_source_id), record.observation_artifacts);
      replay::enrich_replay_episode_bundle_with_artifacts(
          bundle, artifact_source, artifact_options);
    }
    replay::validate_replay_episode_bundle(bundle);
    return bundle;
  }

  void reset() override { cursor_ = 0; }

  [[nodiscard]] std::size_t size() const { return records_.size(); }
  [[nodiscard]] std::size_t cursor() const { return cursor_; }

private:
  std::string source_id_{};
  cuwacunu::kikijyeba::topology::graph::market_graph_t graph_{};
  std::vector<graph_anchor_replay_bundle_record_t<KeyT>> records_{};
  std::size_t cursor_{0};
};

[[nodiscard]] inline std::vector<replay::replay_episode_bundle_t>
collect_replay_bundles(replay_bundle_source_iface_t &source,
                       std::size_t max_count = 0) {
  std::vector<replay::replay_episode_bundle_t> out;
  while (max_count == 0 || out.size() < max_count) {
    auto bundle = source.next_bundle();
    if (!bundle.has_value()) {
      break;
    }
    replay::validate_replay_episode_bundle(*bundle);
    out.push_back(std::move(*bundle));
  }
  return out;
}

} // namespace cuwacunu::kikijyeba::environment
