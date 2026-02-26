// test_tsi_routing.cpp

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <torch/torch.h>

#include "tsiemene/board.h"
#include "tsiemene/tsi.sink.h"
#include "tsiemene/tsi.source.h"

namespace {

constexpr std::string_view kObsInstrumentsDsl =
    "observation.instruments = {\n"
    "  BTCUSDT = exchange.binance@spot\n"
    "}\n";
constexpr std::string_view kObsInputsDsl =
    "observation.inputs = {\n"
    "  open close volume\n"
    "}\n";
constexpr std::string_view kTrainingComponentsDsl =
    "training.components = {\n"
    "  vicreg\n"
    "}\n";

void seed_required_contract_dsl(
    tsiemene::BoardContract& c,
    std::string circuit_dsl) {
  c.set_dsl_segment(tsiemene::kBoardContractCircuitDslKey, std::move(circuit_dsl));
  c.set_dsl_segment(
      tsiemene::kBoardContractObservationSourcesDslKey,
      std::string(kObsInstrumentsDsl));
  c.set_dsl_segment(
      tsiemene::kBoardContractObservationChannelsDslKey,
      std::string(kObsInputsDsl));
  c.set_dsl_segment(
      tsiemene::kBoardContractJkimyeiSpecsDslKey,
      std::string(kTrainingComponentsDsl));
}

struct ProbeState {
  std::vector<std::string> hits{};
  std::vector<std::uint64_t> wave_i{};
  std::vector<std::uint64_t> wave_batch{};
  std::vector<std::uint64_t> wave_episode{};
};

class SourceProbe final : public tsiemene::TsiSource {
 public:
  static constexpr tsiemene::DirectiveId IN_STEP = tsiemene::directive_id::Step;
  static constexpr tsiemene::DirectiveId OUT_PAYLOAD = tsiemene::directive_id::Payload;

  SourceProbe(tsiemene::TsiId id, std::string instance_name)
      : id_(id), instance_name_(std::move(instance_name)) {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return "test.source.probe"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] tsiemene::TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const tsiemene::DirectiveSpec> directives() const noexcept override {
    static constexpr tsiemene::DirectiveSpec kDirectives[] = {
        tsiemene::directive(IN_STEP, tsiemene::DirectiveDir::In, tsiemene::KindSpec::String(), "trigger"),
        tsiemene::directive(OUT_PAYLOAD, tsiemene::DirectiveDir::Out, tsiemene::KindSpec::Tensor(), "payload"),
    };
    return std::span<const tsiemene::DirectiveSpec>(kDirectives, 2);
  }

  void step(const tsiemene::Wave& wave, tsiemene::Ingress in, tsiemene::TsiContext&, tsiemene::Emitter& out) override {
    if (in.directive != IN_STEP) return;
    if (in.signal.kind != tsiemene::PayloadKind::String) return;
    out.emit_tensor(
        wave,
        OUT_PAYLOAD,
        torch::ones({1}, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU)));
  }

 private:
  tsiemene::TsiId id_{};
  std::string instance_name_;
};

class SinkProbe final : public tsiemene::TsiSink {
 public:
  SinkProbe(tsiemene::TsiId id, std::string instance_name, tsiemene::DirectiveId in_directive)
      : id_(id),
        instance_name_(std::move(instance_name)),
        in_directive_(in_directive),
        directives_{tsiemene::directive(
            in_directive_, tsiemene::DirectiveDir::In, tsiemene::KindSpec::Tensor(), "probe sink")} {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return "test.sink.probe"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] tsiemene::TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const tsiemene::DirectiveSpec> directives() const noexcept override {
    return std::span<const tsiemene::DirectiveSpec>(directives_.data(), directives_.size());
  }

  void step(const tsiemene::Wave& wave, tsiemene::Ingress in, tsiemene::TsiContext& ctx, tsiemene::Emitter&) override {
    if (in.directive != in_directive_) return;
    if (in.signal.kind != tsiemene::PayloadKind::Tensor) return;
    auto* state = static_cast<ProbeState*>(ctx.user);
    if (!state) return;
    state->hits.push_back(instance_name_);
    state->wave_i.push_back(wave.i);
    state->wave_batch.push_back(wave.batch);
    state->wave_episode.push_back(wave.episode);
  }

 private:
  tsiemene::TsiId id_{};
  std::string instance_name_;
  tsiemene::DirectiveId in_directive_{};
  std::array<tsiemene::DirectiveSpec, 1> directives_{};
};

class PullSourceProbe final : public tsiemene::TsiSource {
 public:
  static constexpr tsiemene::DirectiveId IN_STEP = tsiemene::directive_id::Step;
  static constexpr tsiemene::DirectiveId OUT_PAYLOAD = tsiemene::directive_id::Payload;

  PullSourceProbe(tsiemene::TsiId id, std::string instance_name, std::uint64_t episode_batches)
      : id_(id),
        instance_name_(std::move(instance_name)),
        episode_batches_(episode_batches) {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return "test.source.pull"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] tsiemene::TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const tsiemene::DirectiveSpec> directives() const noexcept override {
    static constexpr tsiemene::DirectiveSpec kDirectives[] = {
        tsiemene::directive(IN_STEP, tsiemene::DirectiveDir::In, tsiemene::KindSpec::String(), "episode command"),
        tsiemene::directive(OUT_PAYLOAD, tsiemene::DirectiveDir::Out, tsiemene::KindSpec::Tensor(), "payload"),
    };
    return std::span<const tsiemene::DirectiveSpec>(kDirectives, 2);
  }

  [[nodiscard]] bool requests_runtime_continuation() const noexcept override {
    return continue_requested_;
  }

  [[nodiscard]] tsiemene::Ingress runtime_continuation_ingress() const override {
    return tsiemene::Ingress{
        .directive = IN_STEP,
        .signal = tsiemene::string_signal(""),
    };
  }

  void step(const tsiemene::Wave& wave, tsiemene::Ingress in, tsiemene::TsiContext&, tsiemene::Emitter& out) override {
    continue_requested_ = false;
    if (in.directive != IN_STEP) return;
    if (in.signal.kind != tsiemene::PayloadKind::String) return;

    if (!in.signal.text.empty()) remaining_ = episode_batches_;
    if (remaining_ == 0) return;

    out.emit_tensor(
        wave,
        OUT_PAYLOAD,
        torch::ones({1}, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU)));

    --remaining_;
    continue_requested_ = (remaining_ > 0);
  }

 private:
  tsiemene::TsiId id_{};
  std::string instance_name_;
  std::uint64_t episode_batches_{0};
  std::uint64_t remaining_{0};
  bool continue_requested_{false};
};

} // namespace

int main() {
  using namespace tsiemene;

  Board board{};
  auto& c = board.circuits.emplace_back();
  c.spec.sourced_from_config = false;
  c.name = "fanout_cache";
  c.invoke_name = "fanout_cache";
  c.invoke_payload = "go";

  auto& src = c.emplace_node<SourceProbe>(1, "src");
  auto& sink_step = c.emplace_node<SinkProbe>(2, "sink.step", directive_id::Step);
  auto& sink_info = c.emplace_node<SinkProbe>(3, "sink.info", directive_id::Info);

  c.hops = {
      hop(ep(src, SourceProbe::OUT_PAYLOAD), ep(sink_step, directive_id::Step), query("")),
      hop(ep(src, SourceProbe::OUT_PAYLOAD), ep(sink_info, directive_id::Info), query("")),
  };
  c.wave0 = Wave{.id = 1, .i = 0};
  c.ingress0 = Ingress{
      .directive = directive_id::Step,
      .signal = string_signal("go"),
  };
  seed_required_contract_dsl(
      c,
      "fanout_cache = {\n"
      "  src = test.source.probe\n"
      "  sink.step = test.sink.probe\n"
      "  sink.info = test.sink.probe\n"
      "  src@payload:tensor -> sink.step@step\n"
      "  src@payload:tensor -> sink.info@info\n"
      "}\n"
      "fanout_cache(go);\n");

  BoardIssue issue{};
  if (!validate_board(board, &issue)) {
    std::cerr << "[test_tsi_routing] invalid board: " << issue.what << "\n";
    return 1;
  }
  assert(board.contracts.size() == 1);
  assert(board.contracts[0].spec.instrument.empty());

  ProbeState state{};
  TsiContext ctx{.user = &state};

  const std::uint64_t steps_1 = run_circuit(c, ctx);
  const std::uint64_t builds_1 = c.compiled_build_count;
  const std::uint64_t steps_2 = run_circuit(c, ctx);
  const std::uint64_t builds_2 = c.compiled_build_count;

  assert(steps_1 > 0);
  assert(steps_2 > 0);
  assert(builds_1 == 1);
  assert(builds_2 == 1);
  assert(state.hits.size() == 4);
  assert(state.hits[0] == "sink.step");
  assert(state.hits[1] == "sink.info");
  assert(state.hits[2] == "sink.step");
  assert(state.hits[3] == "sink.info");

  auto& sink_warn = c.emplace_node<SinkProbe>(4, "sink.warn", directive_id::Warn);
  c.hops.push_back(hop(ep(src, SourceProbe::OUT_PAYLOAD), ep(sink_warn, directive_id::Warn), query("")));
  c.invalidate_compiled_runtime();

  BoardIssue issue_after{};
  if (!validate_board(board, &issue_after)) {
    std::cerr << "[test_tsi_routing] invalid board after topology change: " << issue_after.what << "\n";
    return 1;
  }

  const std::size_t hits_before = state.hits.size();
  const std::uint64_t steps_3 = run_circuit(c, ctx);
  assert(steps_3 > 0);
  assert(c.compiled_build_count == 2);
  assert(state.hits.size() == hits_before + 3);
  assert(state.hits[hits_before + 0] == "sink.step");
  assert(state.hits[hits_before + 1] == "sink.info");
  assert(state.hits[hits_before + 2] == "sink.warn");

  auto& c_multi = board.circuits.emplace_back();
  c_multi.spec.sourced_from_config = false;
  c_multi.name = "fanout_cache_aux";
  c_multi.invoke_name = "fanout_cache_aux";
  c_multi.invoke_payload = "go_aux";
  auto& src_aux = c_multi.emplace_node<SourceProbe>(20, "src_aux");
  auto& sink_aux = c_multi.emplace_node<SinkProbe>(21, "sink_aux", directive_id::Step);
  c_multi.hops = {
      hop(ep(src_aux, SourceProbe::OUT_PAYLOAD), ep(sink_aux, directive_id::Step), query("")),
  };
  c_multi.wave0 = Wave{.id = 2, .i = 0};
  c_multi.ingress0 = Ingress{
      .directive = directive_id::Step,
      .signal = string_signal("go_aux"),
  };
  seed_required_contract_dsl(
      c_multi,
      "fanout_cache_aux = {\n"
      "  src_aux = test.source.probe\n"
      "  sink_aux = test.sink.probe\n"
      "  src_aux@payload:tensor -> sink_aux@step\n"
      "}\n"
      "fanout_cache_aux(go_aux);\n");

  BoardIssue issue_multi{};
  if (!validate_board(board, &issue_multi)) {
    std::cerr << "[test_tsi_routing] invalid multi-contract board: " << issue_multi.what << "\n";
    return 1;
  }
  assert(board.contracts.size() == 2);
  std::unordered_set<std::string> unique_circuit_dsl;
  unique_circuit_dsl.reserve(board.contracts.size());
  const std::string shared_obs_inst =
      board.contracts.front().dsl_segment_or(kBoardContractObservationSourcesDslKey);
  const std::string shared_obs_inputs =
      board.contracts.front().dsl_segment_or(kBoardContractObservationChannelsDslKey);
  const std::string shared_train =
      board.contracts.front().dsl_segment_or(kBoardContractJkimyeiSpecsDslKey);
  assert(!shared_obs_inst.empty());
  assert(!shared_obs_inputs.empty());
  assert(!shared_train.empty());
  for (const auto& contract : board.contracts) {
    std::string_view missing_key{};
    assert(contract.has_required_dsl_segments(&missing_key));
    const std::string circuit_dsl = contract.dsl_segment_or(kBoardContractCircuitDslKey);
    assert(!circuit_dsl.empty());
    assert(unique_circuit_dsl.insert(circuit_dsl).second);
    assert(contract.dsl_segment_or(kBoardContractObservationSourcesDslKey) == shared_obs_inst);
    assert(contract.dsl_segment_or(kBoardContractObservationChannelsDslKey) == shared_obs_inputs);
    assert(contract.dsl_segment_or(kBoardContractJkimyeiSpecsDslKey) == shared_train);
  }

  Board board_seq{};
  auto& c2 = board_seq.circuits.emplace_back();
  c2.spec.sourced_from_config = false;
  c2.name = "runtime_continuation";
  c2.invoke_name = "runtime_continuation";
  c2.invoke_payload = "episode";

  auto& pull_src = c2.emplace_node<PullSourceProbe>(10, "pull.src", 3);
  auto& pull_sink = c2.emplace_node<SinkProbe>(11, "pull.sink", directive_id::Step);
  c2.hops = {
      hop(ep(pull_src, PullSourceProbe::OUT_PAYLOAD), ep(pull_sink, directive_id::Step), query("")),
  };
  c2.wave0 = Wave{.id = 9, .i = 0};
  c2.wave0.episode = 4;
  c2.wave0.batch = 0;
  c2.ingress0 = Ingress{
      .directive = directive_id::Step,
      .signal = string_signal("episode"),
  };
  seed_required_contract_dsl(
      c2,
      "runtime_continuation = {\n"
      "  pull.src = test.source.pull\n"
      "  pull.sink = test.sink.probe\n"
      "  pull.src@payload:tensor -> pull.sink@step\n"
      "}\n"
      "runtime_continuation(episode);\n");

  BoardIssue issue_seq{};
  if (!validate_board(board_seq, &issue_seq)) {
    std::cerr << "[test_tsi_routing] invalid continuation board: " << issue_seq.what << "\n";
    return 1;
  }
  assert(board_seq.contracts.size() == 1);

  ProbeState state_seq{};
  TsiContext ctx_seq{.user = &state_seq};
  const std::uint64_t seq_steps = run_circuit(c2, ctx_seq);
  assert(seq_steps == 6);
  assert(state_seq.hits.size() == 3);
  assert(state_seq.wave_i.size() == 3);
  assert(state_seq.wave_batch.size() == 3);
  assert(state_seq.wave_episode.size() == 3);
  assert(state_seq.wave_i[0] == 0);
  assert(state_seq.wave_i[1] == 1);
  assert(state_seq.wave_i[2] == 2);
  assert(state_seq.wave_batch[0] == 0);
  assert(state_seq.wave_batch[1] == 1);
  assert(state_seq.wave_batch[2] == 2);
  assert(state_seq.wave_episode[0] == 4);
  assert(state_seq.wave_episode[1] == 4);
  assert(state_seq.wave_episode[2] == 4);

  std::cout << "[test_tsi_routing] pass\n";
  return 0;
}
