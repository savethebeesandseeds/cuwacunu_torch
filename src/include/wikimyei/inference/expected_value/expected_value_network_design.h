/* expected_value_network_design.h */
#pragma once

#include "camahjucunu/dsl/network_design/network_design.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace cuwacunu {
namespace wikimyei {

struct expected_value_network_design_spec_t {
  std::string network_id{};
  std::string assembly_tag{};

  std::string encoding_node_id{};
  std::string target_node_id{};
  std::string mdn_node_id{};

  int encoding_dims{0};
  std::string encoding_temporal_reducer{"last_valid"};
  std::vector<int> target_dims{};
  int mixture_comps{0};
  int features_hidden{0};
  int residual_depth{0};
};

namespace expected_value_network_design_detail {

[[nodiscard]] inline std::string trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string lower_ascii_copy(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

[[nodiscard]] inline bool parse_int_strict(const std::string &s, int *out) {
  if (out == nullptr)
    return false;
  const std::string t = trim_ascii_copy(s);
  if (t.empty())
    return false;
  int v = 0;
  const auto *b = t.data();
  const auto *e = t.data() + t.size();
  const auto r = std::from_chars(b, e, v);
  if (r.ec != std::errc{} || r.ptr != e)
    return false;
  *out = v;
  return true;
}

[[nodiscard]] inline std::vector<std::string>
split_csv_like(const std::string &s) {
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string tok;
  while (std::getline(ss, tok, ',')) {
    tok = trim_ascii_copy(std::move(tok));
    if (!tok.empty())
      out.push_back(std::move(tok));
  }
  return out;
}

[[nodiscard]] inline bool parse_int_list_strict(const std::string &s,
                                                std::vector<int> *out) {
  if (out == nullptr)
    return false;
  std::vector<int> parsed{};
  for (const auto &tok : split_csv_like(s)) {
    int v = 0;
    if (!parse_int_strict(tok, &v))
      return false;
    parsed.push_back(v);
  }
  if (parsed.empty())
    return false;
  *out = std::move(parsed);
  return true;
}

[[nodiscard]] inline bool
get_required_param(const cuwacunu::camahjucunu::network_design_node_t &node,
                   const std::string &key, std::string *out_value) {
  if (out_value)
    out_value->clear();
  for (const auto &p : node.params) {
    if (trim_ascii_copy(p.key) == key) {
      if (out_value)
        *out_value = trim_ascii_copy(p.value);
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline bool find_export_target(
    const cuwacunu::camahjucunu::network_design_instruction_t &ir,
    const std::string &export_name, std::string *out_node_id) {
  if (out_node_id)
    out_node_id->clear();
  for (const auto &ex : ir.exports) {
    if (trim_ascii_copy(ex.name) == export_name) {
      if (out_node_id)
        *out_node_id = trim_ascii_copy(ex.node_id);
      return true;
    }
  }
  return false;
}

} // namespace expected_value_network_design_detail

[[nodiscard]] inline bool resolve_expected_value_network_design(
    const cuwacunu::camahjucunu::network_design_instruction_t &ir,
    expected_value_network_design_spec_t *out, std::string *error = nullptr) {
  using expected_value_network_design_detail::find_export_target;
  using expected_value_network_design_detail::get_required_param;
  using expected_value_network_design_detail::lower_ascii_copy;
  using expected_value_network_design_detail::parse_int_list_strict;
  using expected_value_network_design_detail::parse_int_strict;
  using expected_value_network_design_detail::trim_ascii_copy;

  if (error)
    error->clear();
  if (out == nullptr) {
    if (error)
      *error = "resolve_expected_value_network_design requires non-null output";
    return false;
  }

  *out = expected_value_network_design_spec_t{};
  out->network_id = trim_ascii_copy(ir.network_id);
  out->assembly_tag = trim_ascii_copy(ir.assembly_tag);
  if (out->network_id.empty()) {
    if (error)
      *error = "network_design.network_id is empty";
    return false;
  }
  if (!out->assembly_tag.empty() &&
      lower_ascii_copy(out->assembly_tag) != "mdn_expected_value") {
    if (error) {
      *error = "unsupported ASSEMBLY_TAG `" + out->assembly_tag +
               "` for ExpectedValue";
    }
    return false;
  }

  const auto encoding_nodes = ir.find_nodes_by_kind("ENCODING");
  const auto target_nodes = ir.find_nodes_by_kind("FUTURE_TARGET");
  const auto mdn_nodes = ir.find_nodes_by_kind("MDN");
  if (encoding_nodes.size() != 1 || target_nodes.size() != 1 ||
      mdn_nodes.size() != 1) {
    if (error) {
      *error = "ExpectedValue expects exactly one ENCODING, one FUTURE_TARGET, "
               "and one MDN node";
    }
    return false;
  }

  const auto &encoding = *encoding_nodes.front();
  const auto &target = *target_nodes.front();
  const auto &mdn = *mdn_nodes.front();
  out->encoding_node_id = encoding.id;
  out->target_node_id = target.id;
  out->mdn_node_id = mdn.id;

  std::string export_expectation_node{};
  if (!find_export_target(ir, "expectation", &export_expectation_node)) {
    if (error)
      *error = "ExpectedValue requires export `expectation`";
    return false;
  }
  if (export_expectation_node != out->mdn_node_id) {
    if (error) {
      *error = "`expectation` export must target MDN node `" +
               out->mdn_node_id + "`";
    }
    return false;
  }

  std::string raw{};
  if (!get_required_param(encoding, "De", &raw) &&
      !get_required_param(encoding, "encoding_dims", &raw)) {
    if (error)
      *error = "ENCODING.De (or encoding_dims) is required";
    return false;
  }
  if (!parse_int_strict(raw, &out->encoding_dims) || out->encoding_dims <= 0) {
    if (error)
      *error = "ENCODING.De must be a positive int";
    return false;
  }
  raw.clear();
  if (get_required_param(encoding, "temporal_reducer", &raw) ||
      get_required_param(encoding, "reducer", &raw)) {
    raw = lower_ascii_copy(trim_ascii_copy(raw));
    std::replace(raw.begin(), raw.end(), '-', '_');
    if (raw == "lastvalid")
      raw = "last_valid";
    if (raw == "maskedmean")
      raw = "masked_mean";
    if (raw != "last_valid" && raw != "masked_mean" && raw != "mean") {
      if (error) {
        *error = "ENCODING.temporal_reducer must be one of: last_valid, "
                 "masked_mean, mean";
      }
      return false;
    }
    out->encoding_temporal_reducer = raw;
  }

  if (!get_required_param(target, "target_dims", &raw) ||
      !parse_int_list_strict(raw, &out->target_dims)) {
    if (error)
      *error = "FUTURE_TARGET.target_dims must be a non-empty int list";
    return false;
  }
  for (const int dim : out->target_dims) {
    if (dim < 0) {
      if (error)
        *error = "FUTURE_TARGET.target_dims must be non-negative";
      return false;
    }
  }
  {
    auto sorted_dims = out->target_dims;
    std::sort(sorted_dims.begin(), sorted_dims.end());
    if (std::adjacent_find(sorted_dims.begin(), sorted_dims.end()) !=
        sorted_dims.end()) {
      if (error)
        *error = "FUTURE_TARGET.target_dims must be unique";
      return false;
    }
  }

  if (!get_required_param(mdn, "mixture_comps", &raw) ||
      !parse_int_strict(raw, &out->mixture_comps) || out->mixture_comps <= 0) {
    if (error)
      *error = "MDN.mixture_comps must be a positive int";
    return false;
  }
  if (!get_required_param(mdn, "features_hidden", &raw) ||
      !parse_int_strict(raw, &out->features_hidden) ||
      out->features_hidden <= 0) {
    if (error)
      *error = "MDN.features_hidden must be a positive int";
    return false;
  }
  if (!get_required_param(mdn, "residual_depth", &raw) ||
      !parse_int_strict(raw, &out->residual_depth) || out->residual_depth < 0) {
    if (error)
      *error = "MDN.residual_depth must be a non-negative int";
    return false;
  }

  return true;
}

} // namespace wikimyei
} // namespace cuwacunu
