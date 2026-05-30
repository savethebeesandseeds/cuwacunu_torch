#pragma once

#include <mutex>
#include <string>

namespace cuwacunu {
namespace camahjucunu {

struct marshal_objective_instruction_t {
  std::string campaign_dsl_path{};
  std::string objective_md_path{};
  std::string guidance_md_path{};
  std::string objective_name{};
  std::string marshal_codex_model{};
  std::string marshal_codex_reasoning_effort{};
  std::string marshal_session_id{};
  std::string str() const;
};

namespace dsl {

class marshalObjectivePipeline {
 public:
  explicit marshalObjectivePipeline(std::string grammar_text);
  marshal_objective_instruction_t decode(std::string instruction);

 private:
  std::mutex current_mutex_{};
  std::string grammar_text_{};
};

marshal_objective_instruction_t decode_marshal_objective_from_dsl(
    std::string grammar_text,
    std::string instruction_text);

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
