#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "camahjucunu/dsl/instrument_signature.h"

namespace cuwacunu {
namespace camahjucunu {

struct lattice_target_contract_decl_t {
  std::string id{};
  std::string file{};
};

struct lattice_target_component_decl_t {
  std::string slot{};
  std::string tag{};
};

struct lattice_target_source_decl_t {
  instrument_signature_t signature{};
  std::string train_window{};
  std::string validate_window{};
  std::string test_window{};
};

struct lattice_target_effort_decl_t {
  std::uint64_t epochs{0};
  std::uint64_t batch_size{0};
  std::uint64_t max_batches_per_epoch{0};
};

struct lattice_target_require_decl_t {
  bool trained{false};
  bool validated{false};
  bool tested{false};
};

struct lattice_target_decl_t {
  std::string id{};
  std::string contract_ref{};
  std::vector<lattice_target_component_decl_t> component_targets{};
  lattice_target_source_decl_t source{};
  lattice_target_effort_decl_t effort{};
  lattice_target_require_decl_t require{};
};

struct lattice_target_obligation_t {
  std::string target_id{};
  std::string contract_ref{};
  std::string component_slot{};
  std::string component_tag{};
  instrument_signature_t source_signature{};
  std::string train_window{};
  std::string validate_window{};
  std::string test_window{};
  std::uint64_t epochs{0};
  std::uint64_t batch_size{0};
  std::uint64_t max_batches_per_epoch{0};
  bool require_trained{false};
  bool require_validated{false};
  bool require_tested{false};
};

struct lattice_target_instruction_t {
  std::string id{};
  std::vector<lattice_target_contract_decl_t> contracts{};
  std::vector<lattice_target_decl_t> targets{};

  [[nodiscard]] std::vector<lattice_target_obligation_t> obligations() const;
  [[nodiscard]] std::string str() const;
};

namespace dsl {

class latticeTargetPipeline {
public:
  explicit latticeTargetPipeline(std::string grammar_text);
  lattice_target_instruction_t decode(std::string instruction);

private:
  std::mutex current_mutex_{};
  std::string grammar_text_{};
};

lattice_target_instruction_t
decode_lattice_target_from_dsl(std::string grammar_text,
                               std::string instruction_text);

} // namespace dsl

} // namespace camahjucunu
} // namespace cuwacunu
