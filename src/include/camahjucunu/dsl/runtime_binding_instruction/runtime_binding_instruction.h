#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"

namespace cuwacunu {
namespace camahjucunu {

struct runtime_binding_contract_decl_t {
  std::string id{};
  std::string file{};
};

struct runtime_binding_wave_decl_t {
  std::string id{};
  std::string file{};
};

using runtime_binding_bind_decl_t = wave_contract_binding_decl_t;

struct runtime_binding_instruction_t {
  std::string active_bind_id{};
  std::vector<runtime_binding_contract_decl_t> contracts{};
  std::vector<runtime_binding_wave_decl_t> waves{};
  std::vector<runtime_binding_bind_decl_t> binds{};
  std::string str() const;
};

namespace dsl {

class runtimeBindingPipeline {
 public:
  explicit runtimeBindingPipeline(std::string grammar_text);
  runtime_binding_instruction_t decode(std::string instruction);

 private:
  std::mutex current_mutex_{};
  std::string grammar_text_{};
};

runtime_binding_instruction_t decode_runtime_binding_from_dsl(
    std::string grammar_text,
    std::string instruction_text);

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
