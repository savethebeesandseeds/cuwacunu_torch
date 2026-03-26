#pragma once

#include <mutex>
#include <string>

namespace cuwacunu {
namespace camahjucunu {

struct super_objective_instruction_t {
  std::string campaign_dsl_path{};
  std::string objective_md_path{};
  std::string guidance_md_path{};
  std::string objective_name{};
  std::string loop_id{};
  std::string str() const;
};

namespace dsl {

class superObjectivePipeline {
 public:
  explicit superObjectivePipeline(std::string grammar_text);
  super_objective_instruction_t decode(std::string instruction);

 private:
  std::mutex current_mutex_{};
  std::string grammar_text_{};
};

super_objective_instruction_t decode_super_objective_from_dsl(
    std::string grammar_text,
    std::string instruction_text);

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
