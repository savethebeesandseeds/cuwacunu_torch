/* canonical_path.h */
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace cuwacunu {
namespace camahjucunu {

enum class canonical_path_kind_e {
  Node = 0,
  Endpoint = 1,
  Call = 2,
};

enum class canonical_facet_e {
  none = 0,
  jkimyei = 1,
};

struct canonical_path_arg_t {
  std::string key{};
  std::string value{};
};

struct canonical_path_t {
  bool ok{false};
  std::string error{};

  std::string raw{};
  std::string canonical{};
  std::string canonical_identity{};
  std::string canonical_endpoint{};

  canonical_path_kind_e path_kind{canonical_path_kind_e::Node};
  canonical_facet_e facet{canonical_facet_e::none};

  std::vector<std::string> segments{};
  std::vector<canonical_path_arg_t> args{};

  std::string hashimyei{};
  std::string directive{};
  std::string kind{};

  std::string identity_hash_name{};
  std::string endpoint_hash_name{};
};

namespace dsl {

class canonicalPath {
 public:
  std::string CANONICAL_PATH_GRAMMAR_TEXT{};

  canonicalPath();
  explicit canonicalPath(std::string grammar_text);

  canonical_path_t decode(std::string instruction) const;
};

}  // namespace dsl

canonical_path_t decode_canonical_path(const std::string& text);
bool validate_canonical_path(const canonical_path_t& path, std::string* error = nullptr);
std::string canonicalize_canonical_path(const canonical_path_t& path);

// Migration adapters (stubs): primitive text surfaces to the canonical DSL.
canonical_path_t decode_primitive_endpoint_text(const std::string& text);
canonical_path_t decode_primitive_command_text(const std::string& text);

std::string hashimyei_round_note();

}  // namespace camahjucunu
}  // namespace cuwacunu
