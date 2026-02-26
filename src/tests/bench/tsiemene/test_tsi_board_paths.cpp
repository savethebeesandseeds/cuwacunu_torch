// test_tsi_board_paths.cpp
#include <iostream>
#include <set>
#include <string>

#include "tsiemene/board.contract.h"
#include "tsiemene/board.contract.init.h"
#include "tsiemene/tsi.directive.registry.h"

int main() {
  using namespace tsiemene;

  auto require = [](bool cond, const char* msg) {
    if (!cond) std::cerr << "[test_tsi_board_paths] " << msg << "\n";
    return cond;
  };

  bool ok = true;

  const auto d_init = parse_directive_id("@init");
  const auto d_jk = parse_directive_id("@jkimyei");
  const auto d_weights = parse_directive_id("@weights");
  const auto d_step = parse_directive_id("@step");
  ok = ok && require(d_init.has_value() && *d_init == directive_id::Init,
                     "expected @init directive from board.paths.def");
  ok = ok && require(d_jk.has_value() && *d_jk == directive_id::Jkimyei,
                     "expected @jkimyei directive from board.paths.def");
  ok = ok && require(d_weights.has_value() && *d_weights == directive_id::Weights,
                     "expected @weights directive from board.paths.def");
  ok = ok && require(d_step.has_value() && *d_step == directive_id::Step,
                     "expected @step directive from tsi.paths.def");

  const auto m_init = parse_method_id("init");
  const auto m_jk = parse_method_id("jkimyei");
  ok = ok && require(m_init.has_value() && *m_init == method_id::Init,
                     "expected init method from board.paths.def");
  ok = ok && require(m_jk.has_value() && *m_jk == method_id::Jkimyei,
                     "expected jkimyei method from board.paths.def");

  ok = ok && require(std::string(kBoardContractInitCanonicalAction) == "board.contract@init",
                     "canonical board.contract init action mismatch");

  std::set<std::string> required_keys;
  for (const std::string_view key : kBoardContractRequiredDslKeys) {
    ok = ok && require(!key.empty(), "required DSL key is empty");
    required_keys.insert(std::string(key));
  }
  ok = ok && require(required_keys.size() == kBoardContractRequiredDslKeys.size(),
                     "required DSL keys must be unique");
  ok = ok && require(required_keys.count(std::string(kBoardContractCircuitDslKey)) == 1,
                     "missing board.contract.circuit@DSL:str");
  ok = ok && require(
                 required_keys.count(std::string(kBoardContractObservationSourcesDslKey)) == 1,
                 "missing board.contract.observation_sources@DSL:str");
  ok = ok && require(
                 required_keys.count(std::string(kBoardContractObservationChannelsDslKey)) == 1,
                 "missing board.contract.observation_channels@DSL:str");
  ok = ok && require(
                 required_keys.count(std::string(kBoardContractJkimyeiSpecsDslKey)) == 1,
                 "missing board.contract.jkimyei_specs@DSL:str");

  if (!ok) return 1;
  std::cout << "[test_tsi_board_paths] pass\n";
  return 0;
}
