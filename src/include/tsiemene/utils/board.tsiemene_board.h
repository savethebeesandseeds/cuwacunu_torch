// ./include/tsiemene/utils/board.tsiemene_board.h
// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <torch/torch.h>

#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board_runtime.h"

#include "tsiemene/utils/board.h"
#include "tsiemene/tsi.wikimyei.wave.generator.h"
#include "tsiemene/tsi.wikimyei.source.dataloader.h"
#include "tsiemene/tsi.wikimyei.representation.vicreg.h"
#include "tsiemene/tsi.sink.null.h"
#include "tsiemene/tsi.sink.log.sys.h"
#include "tsiemene/tsi.sink.tensor.h"

namespace tsiemene {
namespace board_builder {

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
using DataloaderT = TsiDataloaderInstrument<Datatype_t, Sampler_t>;

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
std::unique_ptr<Tsi> make_tsi_for_decl(
    TsiId id,
    const cuwacunu::camahjucunu::tsiemene_instance_decl_t& decl,
    const std::string& instrument,
    torch::Device device,
    const DataloaderT<Datatype_t, Sampler_t>* first_dataloader,
    bool* made_dataloader) {
  if (made_dataloader) *made_dataloader = false;

  if (decl.tsi_type == "tsi.wikimyei.wave.generator") {
    return std::make_unique<TsiWaveGenerator>(id, decl.alias);
  }
  if (decl.tsi_type == "tsi.wikimyei.source.dataloader") {
    if (made_dataloader) *made_dataloader = true;
    return std::make_unique<DataloaderT<Datatype_t, Sampler_t>>(id, instrument, device);
  }
  if (decl.tsi_type == "tsi.wikimyei.representation.vicreg") {
    if (!first_dataloader) return nullptr;
    return std::make_unique<TsiVicreg4D>(
        id,
        decl.alias,
        static_cast<int>(first_dataloader->C()),
        static_cast<int>(first_dataloader->T()),
        static_cast<int>(first_dataloader->D()),
        /*train=*/true,
        /*use_swa=*/true,
        /*detach_to_cpu=*/true);
  }
  if (decl.tsi_type == "tsi.sink.null") {
    return std::make_unique<TsiSinkNull>(id, decl.alias);
  }
  if (decl.tsi_type == "tsi.sink.log.sys") {
    return std::make_unique<TsiSinkLogSys>(id, decl.alias);
  }
  if (decl.tsi_type == "tsi.sink.tensor") {
    return std::make_unique<TsiSinkTensor>(id, decl.alias);
  }
  return nullptr;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_circuit_from_decl(
    const cuwacunu::camahjucunu::tsiemene_circuit_decl_t& parsed,
    torch::Device device,
    BoardCircuit* out,
    std::string* error = nullptr) {
  if (!out) return false;

  {
    std::string semantic_error;
    if (!cuwacunu::camahjucunu::validate_circuit_decl(parsed, &semantic_error)) {
      if (error) *error = semantic_error;
      return false;
    }
  }

  out->name = parsed.name;
  out->invoke_name = parsed.invoke_name;
  out->invoke_payload = parsed.invoke_payload;
  out->nodes.clear();
  out->hops.clear();

  const std::string instrument = cuwacunu::camahjucunu::circuit_invoke_symbol(parsed);
  if (instrument.empty()) {
    if (error) *error = "empty instrument in invoke payload: " + parsed.invoke_payload;
    return false;
  }

  std::unordered_map<std::string, Tsi*> alias_to_tsi;
  std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t> resolved_hops;
  DataloaderT<Datatype_t, Sampler_t>* first_dataloader = nullptr;
  std::uint64_t next_id = 1;

  for (const auto& decl : parsed.instances) {
    bool made_dataloader = false;
    std::unique_ptr<Tsi> node = make_tsi_for_decl<Datatype_t, Sampler_t>(
        next_id++,
        decl,
        instrument,
        device,
        first_dataloader,
        &made_dataloader);

    if (!node) {
      if (error) {
        if (decl.tsi_type == "tsi.wikimyei.representation.vicreg" && !first_dataloader) {
          *error = "vicreg requires a dataloader declared earlier in the same circuit";
        } else {
          *error = "unsupported tsi_type: " + decl.tsi_type;
        }
      }
      return false;
    }

    auto [it, inserted] = alias_to_tsi.emplace(decl.alias, node.get());
    if (!inserted) {
      if (error) *error = "duplicated instance alias: " + decl.alias;
      return false;
    }
    (void)it;

    if (made_dataloader && !first_dataloader) {
      first_dataloader = dynamic_cast<DataloaderT<Datatype_t, Sampler_t>*>(node.get());
    }

    out->nodes.push_back(std::move(node));
  }

  if (!cuwacunu::camahjucunu::resolve_hops(parsed, &resolved_hops, error)) return false;

  out->hops.reserve(resolved_hops.size());
  for (const auto& h : resolved_hops) {
    const auto it_from = alias_to_tsi.find(h.from.instance);
    const auto it_to = alias_to_tsi.find(h.to.instance);
    if (it_from == alias_to_tsi.end() || it_to == alias_to_tsi.end()) {
      if (error) *error = "hop references unknown instance alias: " + h.from.instance + " -> " + h.to.instance;
      return false;
    }

    const auto* out_spec = find_directive(*it_from->second, h.from.directive, DirectiveDir::Out);
    const auto* in_spec = find_directive(*it_to->second, h.to.directive, DirectiveDir::In);
    if (!out_spec || !in_spec) {
      if (error) {
        *error = "hop directive not found on tsi declarations: " +
                 h.from.instance + "@" + std::string(h.from.directive) +
                 " -> " +
                 h.to.instance + "@" + std::string(h.to.directive);
      }
      return false;
    }

    if (out_spec->kind.kind != h.from.kind || in_spec->kind.kind != h.to.kind) {
      if (error) {
        *error = "hop kind mismatch against tsi declarations: " +
                 h.from.instance + "@" + std::string(h.from.directive) +
                 " -> " +
                 h.to.instance + "@" + std::string(h.to.directive);
      }
      return false;
    }

    out->hops.push_back(hop(
        ep(*it_from->second, h.from.directive),
        ep(*it_to->second, h.to.directive),
        query("")));
  }

  out->wave0 = Wave{.id = 0, .i = 0};
  out->ingress0 = Ingress{
    .directive = pick_start_directive(out->view()),
    .signal = string_signal(parsed.invoke_payload)
  };
  return true;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_board_from_instruction(
    const cuwacunu::camahjucunu::tsiemene_board_instruction_t& inst,
    torch::Device device,
    Board* out,
    std::string* error = nullptr) {
  if (!out) return false;

  {
    std::string semantic_error;
    if (!cuwacunu::camahjucunu::validate_board_instruction(inst, &semantic_error)) {
      if (error) *error = semantic_error;
      return false;
    }
  }

  out->circuits.clear();
  out->circuits.reserve(inst.circuits.size());

  for (std::size_t i = 0; i < inst.circuits.size(); ++i) {
    BoardCircuit c{};
    std::string local_error;
    if (!build_runtime_circuit_from_decl<Datatype_t, Sampler_t>(
            inst.circuits[i], device, &c, &local_error)) {
      if (error) {
        *error = "circuit[" + std::to_string(i) + "] " + local_error;
      }
      return false;
    }
    c.wave0.id = static_cast<WaveId>(i);
    out->circuits.push_back(std::move(c));
  }
  return true;
}

} // namespace board_builder
} // namespace tsiemene
