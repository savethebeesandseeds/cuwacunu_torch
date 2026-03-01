#pragma once

#include <mutex>
#include <string>
#include <vector>

namespace cuwacunu {
namespace camahjucunu {

struct tsiemene_board_contract_decl_t {
  std::string id{};
  std::string file{};
};

struct tsiemene_board_wave_decl_t {
  std::string id{};
  std::string file{};
};

struct tsiemene_board_bind_decl_t {
  std::string id{};
  std::string contract_ref{};
  std::string wave_ref{};
};

struct tsiemene_board_instruction_t {
  std::vector<tsiemene_board_contract_decl_t> contracts{};
  std::vector<tsiemene_board_wave_decl_t> waves{};
  std::vector<tsiemene_board_bind_decl_t> binds{};
  std::string str() const;
};

namespace dsl {

class tsiemeneBoardPipeline {
 public:
  explicit tsiemeneBoardPipeline(std::string grammar_text);
  tsiemene_board_instruction_t decode(std::string instruction);

 private:
  std::mutex current_mutex_{};
  std::string grammar_text_{};
};

tsiemene_board_instruction_t decode_tsiemene_board_from_dsl(
    std::string grammar_text,
    std::string instruction_text);

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
