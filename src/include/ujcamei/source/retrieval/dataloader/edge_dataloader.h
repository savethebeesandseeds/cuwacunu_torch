// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <torch/torch.h>

#include "kikijyeba/topology/graph/graph.h"
#include "ujcamei/source/contract/contract.h"
#include "ujcamei/source/retrieval/dataloader/edge_sample.h"
#include "ujcamei/source/retrieval/storage/memory_mapped/memory_mapped_dataloader.h"
#include "ujcamei/source/retrieval/storage/memory_mapped/memory_mapped_dataset.h"

namespace cuwacunu::ujcamei::source::retrieval::dataloader {

enum class edge_dataloader_backend_t {
  memory_mapped,
};

struct edge_dataloader_options_t {
  edge_dataloader_backend_t backend{edge_dataloader_backend_t::memory_mapped};
  bool force_rebuild_cache{false};
  std::size_t batch_size{64};
  std::size_t workers{0};
};

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
class edge_dataloader_t {
  static_assert(
      std::is_same_v<Sampler_t, torch::data::samplers::SequentialSampler> ||
          std::is_same_v<Sampler_t, torch::data::samplers::RandomSampler>,
      "edge_dataloader_t v1 supports SequentialSampler or RandomSampler");

public:
  using dataset_t = cuwacunu::ujcamei::source::retrieval::storage::
      memory_mapped::MemoryMappedEdgeDataset<Datatype_t>;
  using source_materialization_request_t = cuwacunu::ujcamei::source::
      retrieval::storage::memory_mapped::source_materialization_request_t;
  using loader_t = cuwacunu::ujcamei::source::retrieval::storage::
      memory_mapped::MemoryMappedDataLoader<dataset_t, edge_sample_t,
                                            Datatype_t, Sampler_t>;

  edge_dataloader_t(
      const cuwacunu::ujcamei::source::registry::instrument_signature_t
          &edge_signature,
      source_materialization_request_t materialization_request,
      edge_dataloader_options_t options = {})
      : options_(validate_options_(options)),
        dataset_(
            make_dataset_(edge_signature, materialization_request, options_)),
        sampler_(make_sampler_(dataset_)),
        data_loader_options_(torch::data::DataLoaderOptions()
                                 .batch_size(options_.batch_size)
                                 .workers(options_.workers)),
        loader_(dataset_, sampler_, data_loader_options_) {}

  edge_dataloader_t(
      const cuwacunu::ujcamei::source::registry::instrument_signature_t
          &edge_signature,
      cuwacunu::ujcamei::source::contract::source_spec_t source_spec,
      edge_dataloader_options_t options = {})
      : edge_dataloader_t(
            edge_signature,
            cuwacunu::ujcamei::source::retrieval::storage::memory_mapped::
                make_source_materialization_request(source_spec),
            options) {}

  explicit edge_dataloader_t(dataset_t dataset,
                             edge_dataloader_options_t options = {})
      : options_(validate_options_(options)), dataset_(std::move(dataset)),
        sampler_(make_sampler_(dataset_)),
        data_loader_options_(torch::data::DataLoaderOptions()
                                 .batch_size(options_.batch_size)
                                 .workers(options_.workers)),
        loader_(dataset_, sampler_, data_loader_options_) {}

  auto begin() { return loader_.begin(); }
  auto end() { return loader_.end(); }
  void reset() { loader_.reset(); }

  [[nodiscard]] const dataset_t &dataset() const { return dataset_; }
  [[nodiscard]] torch::optional<std::size_t> size() const {
    return dataset_.size();
  }

  [[nodiscard]] int64_t channel_count() const { return loader_.C_; }
  [[nodiscard]] int64_t input_horizon() const { return loader_.Hx_; }
  [[nodiscard]] int64_t feature_width() const { return loader_.D_; }

  [[nodiscard]] const edge_dataloader_options_t &options() const {
    return options_;
  }

  loader_t &backend_loader() { return loader_; }
  const loader_t &backend_loader() const { return loader_; }

private:
  static edge_dataloader_options_t
  validate_options_(edge_dataloader_options_t options) {
    TORCH_CHECK(options.batch_size > 0,
                "[edge_dataloader_t] batch_size must be positive");
    return options;
  }

  static dataset_t make_dataset_(
      const cuwacunu::ujcamei::source::registry::instrument_signature_t
          &edge_signature,
      const source_materialization_request_t &materialization_request,
      const edge_dataloader_options_t &options) {
    switch (options.backend) {
    case edge_dataloader_backend_t::memory_mapped:
      return cuwacunu::ujcamei::source::retrieval::storage::memory_mapped::
          create_memory_mapped_edge_dataset<Datatype_t>(
              edge_signature, materialization_request,
              options.force_rebuild_cache);
    }
    TORCH_CHECK(false, "[edge_dataloader_t] unsupported backend");
    return dataset_t{};
  }

  static Sampler_t make_sampler_(const dataset_t &dataset) {
    if constexpr (std::is_same_v<Sampler_t,
                                 torch::data::samplers::SequentialSampler>) {
      return dataset.SequentialSampler();
    } else if constexpr (std::is_same_v<Sampler_t,
                                        torch::data::samplers::RandomSampler>) {
      return dataset.RandomSampler();
    }
  }

  edge_dataloader_options_t options_{};
  dataset_t dataset_{};
  Sampler_t sampler_;
  torch::data::DataLoaderOptions data_loader_options_{};
  loader_t loader_;
};

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline auto make_edge_dataloader(
    const cuwacunu::ujcamei::source::registry::instrument_signature_t
        &edge_signature,
    typename edge_dataloader_t<Datatype_t,
                               Sampler_t>::source_materialization_request_t
        materialization_request,
    edge_dataloader_options_t options = {}) {
  return edge_dataloader_t<Datatype_t, Sampler_t>(
      edge_signature, std::move(materialization_request), options);
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline auto make_edge_dataloader(
    const cuwacunu::ujcamei::source::registry::instrument_signature_t
        &edge_signature,
    cuwacunu::ujcamei::source::contract::source_spec_t source_spec,
    edge_dataloader_options_t options = {}) {
  return make_edge_dataloader<Datatype_t, Sampler_t>(
      edge_signature,
      cuwacunu::ujcamei::source::retrieval::storage::memory_mapped::
          make_source_materialization_request(source_spec),
      options);
}

template <typename Datatype_t>
[[nodiscard]] inline auto make_sequential_edge_dataloader(
    const cuwacunu::ujcamei::source::registry::instrument_signature_t
        &edge_signature,
    typename edge_dataloader_t<Datatype_t,
                               torch::data::samplers::SequentialSampler>::
        source_materialization_request_t materialization_request,
    edge_dataloader_options_t options = {}) {
  return make_edge_dataloader<Datatype_t,
                              torch::data::samplers::SequentialSampler>(
      edge_signature, std::move(materialization_request), options);
}

template <typename Datatype_t>
[[nodiscard]] inline auto make_sequential_edge_dataloader(
    const cuwacunu::ujcamei::source::registry::instrument_signature_t
        &edge_signature,
    cuwacunu::ujcamei::source::contract::source_spec_t source_spec,
    edge_dataloader_options_t options = {}) {
  return make_edge_dataloader<Datatype_t,
                              torch::data::samplers::SequentialSampler>(
      edge_signature,
      cuwacunu::ujcamei::source::retrieval::storage::memory_mapped::
          make_source_materialization_request(source_spec),
      options);
}

template <typename Datatype_t>
[[nodiscard]] inline auto make_random_edge_dataloader(
    const cuwacunu::ujcamei::source::registry::instrument_signature_t
        &edge_signature,
    typename edge_dataloader_t<Datatype_t,
                               torch::data::samplers::RandomSampler>::
        source_materialization_request_t materialization_request,
    edge_dataloader_options_t options = {}) {
  return make_edge_dataloader<Datatype_t, torch::data::samplers::RandomSampler>(
      edge_signature, std::move(materialization_request), options);
}

template <typename Datatype_t>
[[nodiscard]] inline auto make_random_edge_dataloader(
    const cuwacunu::ujcamei::source::registry::instrument_signature_t
        &edge_signature,
    cuwacunu::ujcamei::source::contract::source_spec_t source_spec,
    edge_dataloader_options_t options = {}) {
  return make_edge_dataloader<Datatype_t, torch::data::samplers::RandomSampler>(
      edge_signature,
      cuwacunu::ujcamei::source::retrieval::storage::memory_mapped::
          make_source_materialization_request(source_spec),
      options);
}

} // namespace cuwacunu::ujcamei::source::retrieval::dataloader
