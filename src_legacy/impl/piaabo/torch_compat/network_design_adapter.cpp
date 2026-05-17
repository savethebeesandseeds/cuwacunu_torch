/* network_design_adapter.cpp */
#include "piaabo/torch_compat/network_design_adapter.h"

#include <algorithm>
#include <cctype>

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {

namespace {

[[nodiscard]] std::string trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] std::string lower_ascii_copy(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

[[nodiscard]] bool parse_norm_kind(
    const std::string& value,
    cuwacunu::wikimyei::vicreg_rank4::NormKind* out) {
  if (out == nullptr) return false;
  const std::string v = lower_ascii_copy(trim_ascii_copy(value));
  if (v == "batchnorm1d") {
    *out = cuwacunu::wikimyei::vicreg_rank4::NormKind::BatchNorm1d;
    return true;
  }
  if (v == "layernorm") {
    *out = cuwacunu::wikimyei::vicreg_rank4::NormKind::LayerNorm;
    return true;
  }
  if (v == "none") {
    *out = cuwacunu::wikimyei::vicreg_rank4::NormKind::None;
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_act_kind(
    const std::string& value,
    cuwacunu::wikimyei::vicreg_rank4::ActKind* out) {
  if (out == nullptr) return false;
  const std::string v = lower_ascii_copy(trim_ascii_copy(value));
  if (v == "relu") {
    *out = cuwacunu::wikimyei::vicreg_rank4::ActKind::ReLU;
    return true;
  }
  if (v == "silu") {
    *out = cuwacunu::wikimyei::vicreg_rank4::ActKind::SiLU;
    return true;
  }
  return false;
}

}  // namespace

bool build_vicreg_torch_adapter_spec(
    const cuwacunu::wikimyei::vicreg_rank4::vicreg_network_design_spec_t& semantic_spec,
    vicreg_torch_adapter_spec_t* out,
    std::string* error) {
  if (error) error->clear();
  if (out == nullptr) {
    if (error) *error = "build_vicreg_torch_adapter_spec requires non-null output";
    return false;
  }

  cuwacunu::wikimyei::vicreg_rank4::NormKind norm_kind{};
  if (!parse_norm_kind(semantic_spec.projector_norm, &norm_kind)) {
    if (error) {
      *error = "unsupported projector norm token: `" +
               semantic_spec.projector_norm + "`";
    }
    return false;
  }

  cuwacunu::wikimyei::vicreg_rank4::ActKind act_kind{};
  if (!parse_act_kind(semantic_spec.projector_activation, &act_kind)) {
    if (error) {
      *error = "unsupported projector activation token: `" +
               semantic_spec.projector_activation + "`";
    }
    return false;
  }

  out->projector_mlp_spec = semantic_spec.projector_mlp_spec;
  out->projector_options = cuwacunu::wikimyei::vicreg_rank4::ProjectorOptions{
      .norm_kind = norm_kind,
      .act_kind = act_kind,
      .use_hidden_bias = semantic_spec.projector_hidden_bias,
      .use_last_bias = semantic_spec.projector_last_bias,
      .bn_in_fp32 = semantic_spec.projector_bn_in_fp32,
  };
  return true;
}

} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */
