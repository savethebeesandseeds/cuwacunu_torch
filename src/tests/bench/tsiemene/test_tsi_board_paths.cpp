// test_tsi_board_paths.cpp
#include <iostream>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "iitepi/board/board.contract.h"
#include "iitepi/board/board.contract.init.h"
#include "tsiemene/tsi.h"
#include "tsiemene/tsi.directive.registry.h"
#include "tsiemene/tsi.probe.h"
#include "tsiemene/tsi.probe.representation.h"
#include "tsiemene/tsi.probe.representation.transfer_matrix_evaluation.h"
#include "tsiemene/tsi.type.registry.h"

namespace {

class capture_emitter_t final : public tsiemene::Emitter {
 public:
  void emit(const tsiemene::Wave&,
            tsiemene::DirectiveId out_directive,
            tsiemene::Signal out) override {
    ++emit_count;
    last_directive = out_directive;
    last_kind = out.kind;
    last_text = std::move(out.text);
  }

  std::uint64_t emit_count{0};
  tsiemene::DirectiveId last_directive{tsiemene::directive_id::Meta};
  tsiemene::PayloadKind last_kind{tsiemene::PayloadKind::String};
  std::string last_text{};
};

class mock_probe_representation_t final : public tsiemene::TsiProbeRepresentation {
 public:
  explicit mock_probe_representation_t(tsiemene::TsiId id, std::string instance_name)
      : TsiProbeRepresentation(id, std::move(instance_name)) {}

  [[nodiscard]] std::string_view type_name() const noexcept override {
    return "tsi.probe.representation.mock";
  }

  void step(const tsiemene::Wave& wave,
            tsiemene::Ingress,
            tsiemene::BoardContext&,
            tsiemene::Emitter& out) override {
    ++step_calls;
    emit_meta_(wave, out, "mock-step");
  }

  void on_epoch_end(tsiemene::BoardContext&) override { ++epoch_end_calls; }
  void reset(tsiemene::BoardContext&) override { ++reset_calls; }

  std::uint64_t step_calls{0};
  std::uint64_t epoch_end_calls{0};
  std::uint64_t reset_calls{0};
};

static_assert(std::is_polymorphic_v<tsiemene::Tsi>,
              "Tsi must remain polymorphic");
static_assert(std::has_virtual_destructor_v<tsiemene::Tsi>,
              "Tsi must keep virtual destructor");
static_assert(std::is_base_of_v<tsiemene::Tsi, tsiemene::TsiProbe>,
              "TsiProbe must inherit from Tsi");
static_assert(
    std::is_base_of_v<tsiemene::TsiProbe, tsiemene::TsiProbeRepresentation>,
    "TsiProbeRepresentation must inherit from TsiProbe");
static_assert(
    std::is_base_of_v<tsiemene::TsiProbeRepresentation,
                      tsiemene::TsiProbeRepresentationTransferMatrixEvaluation>,
    "transfer_matrix_evaluation probe must inherit from TsiProbeRepresentation");
static_assert(
    std::is_convertible_v<tsiemene::TsiProbeRepresentationTransferMatrixEvaluation*,
                          tsiemene::Tsi*>,
    "transfer_matrix_evaluation probe must be usable through Tsi*");

}  // namespace

int main() {
  using namespace tsiemene;

  auto require = [](bool cond, const char* msg) {
    if (!cond) std::cerr << "[test_tsi_board_paths] " << msg << "\n";
    return cond;
  };

  bool ok = true;

  const auto d_init = parse_directive_id("@init");
  const auto d_jk = parse_directive_id("@jkimyei");
  const auto d_step = parse_directive_id("@impulse");
  ok = ok && require(d_init.has_value() && *d_init == directive_id::Init,
                     "expected @init directive from board.paths.def");
  ok = ok && require(d_jk.has_value() && *d_jk == directive_id::Jkimyei,
                     "expected @jkimyei directive from board.paths.def");
  ok = ok && require(d_step.has_value() && *d_step == directive_id::Step,
                     "expected @impulse directive from tsi.paths.def");

  const auto m_init = parse_method_id("init");
  const auto m_jk = parse_method_id("jkimyei");
  ok = ok && require(m_init.has_value() && *m_init == method_id::Init,
                     "expected init method from board.paths.def");
  ok = ok && require(m_jk.has_value() && *m_jk == method_id::Jkimyei,
                     "expected jkimyei method from board.paths.def");

  const auto probe_transfer =
      parse_tsi_type_id("tsi.probe.representation.transfer_matrix_evaluation");
  const auto probe_generic = parse_tsi_type_id("tsi.probe.representation");
  ok = ok && require(
                 probe_transfer.has_value() &&
                     *probe_transfer ==
                         TsiTypeId::ProbeRepresentationTransferMatrixEvaluation,
                 "expected transfer-matrix probe canonical tsi type");
  ok = ok && require(!probe_generic.has_value(),
                     "generic probe canonical tsi type must be removed");
  const auto probe_hashed_path =
      cuwacunu::camahjucunu::decode_canonical_path(
          "tsi.probe.representation.transfer_matrix_evaluation.0x0000");
  ok = ok && require(probe_hashed_path.ok,
                     "hashed probe canonical path must decode");
  ok = ok && require(
                 probe_hashed_path.path_kind ==
                     cuwacunu::camahjucunu::canonical_path_kind_e::Node,
                 "hashed probe canonical path must decode as node");
  ok = ok && require(
                 probe_hashed_path.canonical_identity ==
                     "tsi.probe.representation.transfer_matrix_evaluation.0x0000",
                 "hashed probe canonical identity mismatch");
  ok = ok && require(probe_hashed_path.hashimyei == "0x0000",
                     "hashed probe canonical hashimyei mismatch");

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

  mock_probe_representation_t mock_probe(77, "mock_probe");
  capture_emitter_t emitter{};
  Wave wave{};
  BoardContext ctx{};
  Ingress ingress{.directive = directive_id::Step, .signal = string_signal("noop")};
  Tsi* tsi_base = &mock_probe;
  TsiProbe* probe_base = &mock_probe;
  TsiProbeRepresentation* repr_base = &mock_probe;
  (void)probe_base;
  (void)repr_base;

  tsi_base->step(wave, ingress, ctx, emitter);
  tsi_base->on_epoch_end(ctx);
  tsi_base->reset(ctx);

  ok = ok && require(mock_probe.step_calls == 1,
                     "virtual dispatch (Tsi::step) did not reach probe representation override");
  ok = ok && require(mock_probe.epoch_end_calls == 1,
                     "virtual dispatch (Tsi::on_epoch_end) did not reach probe representation override");
  ok = ok && require(mock_probe.reset_calls == 1,
                     "virtual dispatch (Tsi::reset) did not reach probe representation override");
  ok = ok && require(emitter.emit_count == 1,
                     "probe protocol meta emission failed");
  ok = ok && require(emitter.last_directive == directive_id::Meta,
                     "probe meta must emit @meta directive");
  ok = ok && require(emitter.last_kind == PayloadKind::String,
                     "probe meta must emit string payload");
  ok = ok && require(emitter.last_text == "mock-step",
                     "probe meta payload mismatch");

  if (!ok) return 1;
  std::cout << "[test_tsi_board_paths] pass\n";
  return 0;
}
