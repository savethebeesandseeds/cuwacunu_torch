/* network_design.h */
#pragma once

#include <cstddef>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace cuwacunu {
namespace camahjucunu {

struct network_design_param_t {
  std::string key{};
  std::string declared_type{};
  std::string value{};
  std::size_t line{0};
};

struct network_design_node_t {
  std::string id{};
  std::string kind{};
  std::vector<network_design_param_t> params{};

  [[nodiscard]] std::map<std::string, std::string> to_param_map() const;
};

struct network_design_export_t {
  std::string name{};
  std::string node_id{};
};

struct network_design_instruction_t {
  std::string network_id{};
  std::string join_policy{};
  std::vector<network_design_node_t> nodes{};
  std::vector<network_design_export_t> exports{};

  [[nodiscard]] const network_design_node_t* find_node_by_id(
      const std::string& id) const;
  [[nodiscard]] std::vector<const network_design_node_t*> find_nodes_by_kind(
      const std::string& kind) const;
  [[nodiscard]] std::string str() const;
};

namespace dsl {

class networkDesignPipeline {
 public:
  explicit networkDesignPipeline(std::string grammar_text);
  network_design_instruction_t decode(std::string instruction);

 private:
  std::mutex current_mutex_{};
  std::string grammar_text_{};
};

network_design_instruction_t decode_network_design_from_dsl(
    std::string grammar_text,
    std::string instruction_text);

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
