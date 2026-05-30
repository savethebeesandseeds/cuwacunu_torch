/* vicreg_rank4_network_design.h */
#pragma once

#include "camahjucunu/dsl/network_design/network_design.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_rank4 {

struct vicreg_network_design_spec_t {
  std::string network_id{};
  std::string assembly_tag{};

  std::string input_node_id{};
  std::string encoder_node_id{};
  std::string projector_node_id{};

  int C{0};
  int T{0};
  int D{0};

  int encoding_dims{0};
  int channel_expansion_dim{0};
  int fused_feature_dim{0};
  int encoder_hidden_dims{0};
  int encoder_depth{0};

  std::string projector_mlp_spec{};
  std::string projector_norm{};
  std::string projector_activation{};
  bool projector_hidden_bias{false};
  bool projector_last_bias{false};
  bool projector_bn_in_fp32{true};
};

namespace detail {

[[nodiscard]] inline std::string trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string lower_ascii_copy(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

[[nodiscard]] inline bool parse_int_strict(const std::string& s, int* out) {
  if (out == nullptr) return false;
  const std::string t = trim_ascii_copy(s);
  if (t.empty()) return false;
  int v = 0;
  const auto* b = t.data();
  const auto* e = t.data() + t.size();
  const auto r = std::from_chars(b, e, v);
  if (r.ec != std::errc{} || r.ptr != e) return false;
  *out = v;
  return true;
}

[[nodiscard]] inline bool parse_bool_strict(const std::string& s, bool* out) {
  if (out == nullptr) return false;
  const std::string v = lower_ascii_copy(trim_ascii_copy(s));
  if (v == "true" || v == "1" || v == "yes" || v == "on") {
    *out = true;
    return true;
  }
  if (v == "false" || v == "0" || v == "no" || v == "off") {
    *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] inline std::vector<std::string> split_csv_like(
    const std::string& s,
    char sep) {
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string tok;
  while (std::getline(ss, tok, sep)) {
    tok = trim_ascii_copy(std::move(tok));
    if (!tok.empty()) out.push_back(std::move(tok));
  }
  return out;
}

[[nodiscard]] inline bool find_export_target(
    const cuwacunu::camahjucunu::network_design_instruction_t& ir,
    const std::string& export_name,
    std::string* out_node_id) {
  if (out_node_id) out_node_id->clear();
  for (const auto& ex : ir.exports) {
    if (trim_ascii_copy(ex.name) == export_name) {
      if (out_node_id) *out_node_id = trim_ascii_copy(ex.node_id);
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline bool get_required_param(
    const cuwacunu::camahjucunu::network_design_node_t& node,
    const std::string& key,
    std::string* out_value) {
  if (out_value) out_value->clear();
  for (const auto& p : node.params) {
    if (trim_ascii_copy(p.key) == key) {
      if (out_value) *out_value = trim_ascii_copy(p.value);
      return true;
    }
  }
  return false;
}

}  // namespace detail

[[nodiscard]] inline bool resolve_vicreg_network_design(
    const cuwacunu::camahjucunu::network_design_instruction_t& ir,
    vicreg_network_design_spec_t* out,
    std::string* error = nullptr) {
  using detail::find_export_target;
  using detail::get_required_param;
  using detail::lower_ascii_copy;
  using detail::parse_bool_strict;
  using detail::parse_int_strict;
  using detail::split_csv_like;
  using detail::trim_ascii_copy;

  if (error) error->clear();
  if (out == nullptr) {
    if (error) *error = "resolve_vicreg_network_design requires non-null output";
    return false;
  }

  *out = vicreg_network_design_spec_t{};
  out->network_id = trim_ascii_copy(ir.network_id);
  out->assembly_tag = trim_ascii_copy(ir.assembly_tag);
  if (out->network_id.empty()) {
    if (error) *error = "network_design.network_id is empty";
    return false;
  }
  if (!out->assembly_tag.empty() &&
      lower_ascii_copy(out->assembly_tag) != "vicreg_encoder_projector") {
    if (error) *error = "unsupported ASSEMBLY_TAG `" + out->assembly_tag +
                        "` for VICReg";
    return false;
  }

  const auto input_nodes = ir.find_nodes_by_kind("INPUT");
  const auto encoder_nodes = ir.find_nodes_by_kind("VICREG_RANK4_ENCODER");
  const auto projector_nodes = ir.find_nodes_by_kind("MLP");
  if (input_nodes.size() != 1 || encoder_nodes.size() != 1 ||
      projector_nodes.size() != 1) {
    if (error) {
      *error =
          "VICReg expects exactly one INPUT, one VICREG_RANK4_ENCODER, one MLP node";
    }
    return false;
  }

  const auto& input = *input_nodes.front();
  const auto& encoder = *encoder_nodes.front();
  const auto& projector = *projector_nodes.front();
  out->input_node_id = input.id;
  out->encoder_node_id = encoder.id;
  out->projector_node_id = projector.id;

  std::string export_embedding_node{};
  std::string export_projected_node{};
  if (!find_export_target(ir, "embedding", &export_embedding_node) ||
      !find_export_target(ir, "projected", &export_projected_node)) {
    if (error) {
      *error = "VICReg requires exports `embedding` and `projected`";
    }
    return false;
  }
  if (export_embedding_node != out->encoder_node_id) {
    if (error) {
      *error = "`embedding` export must target encoder node `" +
               out->encoder_node_id + "`";
    }
    return false;
  }
  if (export_projected_node != out->projector_node_id) {
    if (error) {
      *error = "`projected` export must target projector node `" +
               out->projector_node_id + "`";
    }
    return false;
  }

  std::string raw{};
  if (!get_required_param(input, "C", &raw) || !parse_int_strict(raw, &out->C) ||
      out->C <= 0) {
    if (error) *error = "INPUT.C must be a positive int";
    return false;
  }
  if (!get_required_param(input, "T", &raw) || !parse_int_strict(raw, &out->T) ||
      out->T <= 0) {
    if (error) *error = "INPUT.T must be a positive int";
    return false;
  }
  if (!get_required_param(input, "D", &raw) || !parse_int_strict(raw, &out->D) ||
      out->D <= 0) {
    if (error) *error = "INPUT.D must be a positive int";
    return false;
  }

  if (!get_required_param(encoder, "encoding_dims", &raw) ||
      !parse_int_strict(raw, &out->encoding_dims) || out->encoding_dims <= 0) {
    if (error) *error = "encoder.encoding_dims must be a positive int";
    return false;
  }
  if (!get_required_param(encoder, "channel_expansion_dim", &raw) ||
      !parse_int_strict(raw, &out->channel_expansion_dim) ||
      out->channel_expansion_dim <= 0) {
    if (error) *error = "encoder.channel_expansion_dim must be a positive int";
    return false;
  }
  if (!get_required_param(encoder, "fused_feature_dim", &raw) ||
      !parse_int_strict(raw, &out->fused_feature_dim) ||
      out->fused_feature_dim <= 0) {
    if (error) *error = "encoder.fused_feature_dim must be a positive int";
    return false;
  }
  if (!get_required_param(encoder, "hidden_dims", &raw) &&
      !get_required_param(encoder, "encoder_hidden_dims", &raw)) {
    if (error) *error = "encoder.hidden_dims (or encoder_hidden_dims) is required";
    return false;
  }
  if (!parse_int_strict(raw, &out->encoder_hidden_dims) ||
      out->encoder_hidden_dims <= 0) {
    if (error) *error = "encoder.hidden_dims must be a positive int";
    return false;
  }
  if (!get_required_param(encoder, "depth", &raw) &&
      !get_required_param(encoder, "encoder_depth", &raw)) {
    if (error) *error = "encoder.depth (or encoder_depth) is required";
    return false;
  }
  if (!parse_int_strict(raw, &out->encoder_depth) || out->encoder_depth <= 0) {
    if (error) *error = "encoder.depth must be a positive int";
    return false;
  }

  if (get_required_param(projector, "dims", &raw)) {
    const auto dims = split_csv_like(raw, ',');
    if (dims.size() < 2) {
      if (error) *error = "projector.dims must contain at least two widths";
      return false;
    }
    std::ostringstream spec;
    for (std::size_t i = 0; i < dims.size(); ++i) {
      int v = 0;
      if (!parse_int_strict(dims[i], &v) || v <= 0) {
        if (error) *error = "projector.dims has non-positive width: " + dims[i];
        return false;
      }
      if (i > 0) spec << "-";
      spec << v;
    }
    out->projector_mlp_spec = spec.str();
  } else if (get_required_param(projector, "projector_mlp_spec", &raw)) {
    out->projector_mlp_spec = raw;
  } else {
    if (error) *error = "projector requires dims or projector_mlp_spec";
    return false;
  }

  if (!get_required_param(projector, "norm", &out->projector_norm) &&
      !get_required_param(projector, "projector_norm", &out->projector_norm)) {
    if (error) *error = "projector.norm (or projector_norm) is required";
    return false;
  }
  if (!get_required_param(projector, "activation", &out->projector_activation) &&
      !get_required_param(projector,
                          "projector_activation",
                          &out->projector_activation)) {
    if (error) *error = "projector.activation (or projector_activation) is required";
    return false;
  }
  if (!get_required_param(projector, "hidden_bias", &raw) &&
      !get_required_param(projector, "projector_hidden_bias", &raw)) {
    if (error) *error = "projector.hidden_bias is required";
    return false;
  }
  if (!parse_bool_strict(raw, &out->projector_hidden_bias)) {
    if (error) *error = "projector.hidden_bias must be bool";
    return false;
  }
  if (!get_required_param(projector, "last_bias", &raw) &&
      !get_required_param(projector, "projector_last_bias", &raw)) {
    if (error) *error = "projector.last_bias is required";
    return false;
  }
  if (!parse_bool_strict(raw, &out->projector_last_bias)) {
    if (error) *error = "projector.last_bias must be bool";
    return false;
  }
  if (!get_required_param(projector, "bn_in_fp32", &raw) &&
      !get_required_param(projector, "projector_bn_in_fp32", &raw)) {
    if (error) *error = "projector.bn_in_fp32 is required";
    return false;
  }
  if (!parse_bool_strict(raw, &out->projector_bn_in_fp32)) {
    if (error) *error = "projector.bn_in_fp32 must be bool";
    return false;
  }

  return true;
}

}  // namespace vicreg_rank4
}  // namespace wikimyei
}  // namespace cuwacunu
