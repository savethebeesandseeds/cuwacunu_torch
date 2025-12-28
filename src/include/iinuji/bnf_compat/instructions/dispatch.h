#pragma once

#include <string>
#include <vector>
#include <utility>

#include "iinuji/bnf_compat/instructions/diag.h"
#include "iinuji/bnf_compat/instructions/data.h"
#include "iinuji/bnf_compat/instructions/form.h"
#include "iinuji/bnf_compat/instructions/build.h"

namespace cuwacunu {
namespace iinuji {

struct dispatch_payload_t {
  bool has_str = false;
  bool has_vec = false;
  bool has_num = false;

  std::string str;
  std::vector<std::pair<double,double>> vec;
  double num = 0.0;
};

inline bool binding_get_str(const resolved_binding_t& b,
                            const IInstructionsData& data,
                            const dispatch_payload_t* payload,
                            std::string& out,
                            instructions_diag_t& d,
                            const std::string& where)
{
  if (b.ref.kind == data_kind_e::Str) {
    if (!data.get_str(b.ref.index, out)) {
      d.warn(where + ": data.get_str(str" + std::to_string(b.ref.index) + ") returned false");
      return false;
    }
    return true;
  }

  if (b.ref.kind == data_kind_e::System) {
    if (!payload || !payload->has_str) {
      d.err(where + ": system stream binding requires payload.has_str");
      return false;
    }
    out = payload->str;
    return true;
  }

  d.err(where + ": binding_get_str called on non-str kind");
  return false;
}

inline instructions_diag_t dispatch_event(instructions_build_result_t& built,
                                         const std::string& event_name,
                                         IInstructionsData& data,
                                         const dispatch_payload_t* payload = nullptr)
{
  instructions_diag_t d;

  if (!built.root) {
    d.err("dispatch_event: no built.root");
    return d;
  }

  auto itE = built.events_by_name.find(event_name);
  if (itE == built.events_by_name.end()) {
    d.err("dispatch_event: event not found: '" + event_name + "'");
    return d;
  }
  const resolved_event_t& E = itE->second;

  // _action: write payload into data slots
  if (E.kind_raw == "_action") {
    if (!payload) {
      d.err("dispatch_event: _action '" + event_name + "' requires payload");
      return d;
    }
    for (const auto& b : E.bindings) {
      if (b.bind_kind == bind_kind_e::Str) {
        if (!payload->has_str) { d.err("dispatch_event: missing str payload for _action '" + event_name + "'"); continue; }
        if (!data.set_str(b.ref.index, payload->str)) d.err("dispatch_event: failed set_str(" + std::to_string(b.ref.index) + ")");
      } else if (b.bind_kind == bind_kind_e::Vec) {
        if (!payload->has_vec) { d.err("dispatch_event: missing vec payload for _action '" + event_name + "'"); continue; }
        if (!data.set_vec(b.ref.index, payload->vec)) d.err("dispatch_event: failed set_vec(" + std::to_string(b.ref.index) + ")");
      } else if (b.bind_kind == bind_kind_e::Num) {
        if (!payload->has_num) { d.err("dispatch_event: missing num payload for _action '" + event_name + "'"); continue; }
        if (!data.set_num(b.ref.index, payload->num)) d.err("dispatch_event: failed set_num(" + std::to_string(b.ref.index) + ")");
      }
    }
  }

  // Update all figures referencing this event
  auto itF = built.figures_for_event.find(event_name);
  if (itF == built.figures_for_event.end() || itF->second.empty()) {
    d.warn("dispatch_event: event '" + event_name + "' is not referenced by any figure triggers");
    return d;
  }

  for (const auto& fig_id : itF->second) {
    auto itObj  = built.figure_object_by_id.find(fig_id);
    auto itKind = built.figure_kind_by_id.find(fig_id);
    if (itObj == built.figure_object_by_id.end() || itKind == built.figure_kind_by_id.end())
      continue;

    const std::string& kind = itKind->second;
    const bind_kind_e want = required_bind_kind_for_figure(kind);

    const resolved_binding_t* b = first_binding_of_kind(E, want);
    if (!b) continue;

    auto& obj = itObj->second;

    if (kind == "_label" || kind == "_input_box") {
      std::string s;
      (void)binding_get_str(*b, data, payload, s, d, "dispatch_event(" + event_name + "," + fig_id + ")");
      auto tb = std::dynamic_pointer_cast<textBox_data_t>(obj->data);
      if (tb) tb->content = s;
    }
    else if (kind == "_buffer") {
      std::string s;
      if (binding_get_str(*b, data, payload, s, d, "dispatch_event(" + event_name + "," + fig_id + ")")) {
        auto bb = std::dynamic_pointer_cast<bufferBox_data_t>(obj->data);
        if (bb) bb->push_line(std::move(s));
      }
    }
    else if (kind == "_horizontal_plot") {
      if (b->ref.kind != data_kind_e::Vec) continue;
      std::vector<std::pair<double,double>> pts;
      if (data.get_vec(b->ref.index, pts)) {
        auto pb = std::dynamic_pointer_cast<plotBox_data_t>(obj->data);
        if (pb) {
          if (pb->series.empty()) pb->series.resize(1);
          pb->series[0] = std::move(pts);
        }
      }
    }
  }

  return d;
}

} // namespace iinuji
} // namespace cuwacunu
