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
#include "tsiemene/tsi.sink.h"
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

  tsiemene::RuntimeEventAction on_event(const tsiemene::RuntimeEvent& event,
                                        tsiemene::BoardContext&,
                                        tsiemene::Emitter&) override {
    ++on_event_calls;
    if (event.kind == tsiemene::RuntimeEventKind::EpochStart) ++epoch_start_calls;
    if (event.kind == tsiemene::RuntimeEventKind::EpochEnd) ++epoch_end_calls;
    if (event.kind == tsiemene::RuntimeEventKind::QueueDrained) {
      ++queue_drained_calls;
      tsiemene::RuntimeEventAction action{};
      action.request_continuation = true;
      action.continuation_ingress =
          tsiemene::Ingress{.directive = tsiemene::directive_id::Step,
                            .signal = tsiemene::string_signal("follow")};
      return action;
    }
    return tsiemene::RuntimeEventAction{};
  }

  std::uint64_t step_calls{0};
  std::uint64_t on_event_calls{0};
  std::uint64_t epoch_start_calls{0};
  std::uint64_t epoch_end_calls{0};
  std::uint64_t queue_drained_calls{0};
};

class runtime_source_t final : public tsiemene::Tsi {
 public:
  static constexpr tsiemene::DirectiveId IN_STEP = tsiemene::directive_id::Step;
  static constexpr tsiemene::DirectiveId OUT_PAYLOAD = tsiemene::directive_id::Payload;
  static constexpr tsiemene::DirectiveId OUT_META = tsiemene::directive_id::Meta;

  explicit runtime_source_t(tsiemene::TsiId id, int continuation_budget)
      : id_(id), continuation_budget_(continuation_budget) {}

  [[nodiscard]] std::string_view type_name() const noexcept override {
    return "tsi.test.runtime_source";
  }
  [[nodiscard]] std::string_view instance_name() const noexcept override {
    return "runtime_source";
  }
  [[nodiscard]] tsiemene::TsiId id() const noexcept override { return id_; }
  [[nodiscard]] tsiemene::TsiDomain domain() const noexcept override {
    return tsiemene::TsiDomain::Source;
  }

  [[nodiscard]] std::span<const tsiemene::DirectiveSpec> directives()
      const noexcept override {
    static constexpr tsiemene::DirectiveSpec kDirectives[] = {
        tsiemene::directive(IN_STEP, tsiemene::DirectiveDir::In,
                            tsiemene::KindSpec::String(), "test ingress"),
        tsiemene::directive(OUT_PAYLOAD, tsiemene::DirectiveDir::Out,
                            tsiemene::KindSpec::String(), "test payload"),
        tsiemene::directive(OUT_META, tsiemene::DirectiveDir::Out,
                            tsiemene::KindSpec::String(), "test meta"),
    };
    return std::span<const tsiemene::DirectiveSpec>(kDirectives, 3);
  }

  void step(const tsiemene::Wave& wave,
            tsiemene::Ingress in,
            tsiemene::BoardContext&,
            tsiemene::Emitter& out) override {
    if (in.directive != IN_STEP || in.signal.kind != tsiemene::PayloadKind::String) return;
    ++step_calls;
    out.emit_string(wave, OUT_PAYLOAD, "payload");
  }

  tsiemene::RuntimeEventAction on_event(const tsiemene::RuntimeEvent& event,
                                        tsiemene::BoardContext&,
                                        tsiemene::Emitter&) override {
    if (event.kind == tsiemene::RuntimeEventKind::StepStart) ++step_start_events;
    if (event.kind == tsiemene::RuntimeEventKind::StepDone) ++step_done_events;
    if (event.kind == tsiemene::RuntimeEventKind::QueueDrained) {
      ++queue_drained_events;
      if (continuation_budget_ > 0) {
        --continuation_budget_;
        tsiemene::RuntimeEventAction action{};
        action.request_continuation = true;
        action.continuation_ingress = tsiemene::Ingress{
            .directive = IN_STEP,
            .signal = tsiemene::string_signal("continue"),
        };
        return action;
      }
    }
    if (event.kind == tsiemene::RuntimeEventKind::Backpressure) {
      ++backpressure_events;
    }
    return tsiemene::RuntimeEventAction{};
  }

  std::uint64_t step_calls{0};
  std::uint64_t step_start_events{0};
  std::uint64_t step_done_events{0};
  std::uint64_t queue_drained_events{0};
  std::uint64_t backpressure_events{0};

 private:
  tsiemene::TsiId id_{};
  int continuation_budget_{0};
};

class runtime_sink_t final : public tsiemene::TsiSink {
 public:
  static constexpr tsiemene::DirectiveId IN_STEP = tsiemene::directive_id::Step;
  static constexpr tsiemene::DirectiveId IN_DEBUG = tsiemene::directive_id::Debug;

  explicit runtime_sink_t(tsiemene::TsiId id) : id_(id) {}

  [[nodiscard]] std::string_view type_name() const noexcept override {
    return "tsi.test.runtime_sink";
  }
  [[nodiscard]] std::string_view instance_name() const noexcept override {
    return "runtime_sink";
  }
  [[nodiscard]] tsiemene::TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const tsiemene::DirectiveSpec> directives()
      const noexcept override {
    static constexpr tsiemene::DirectiveSpec kDirectives[] = {
        tsiemene::directive(IN_STEP, tsiemene::DirectiveDir::In,
                            tsiemene::KindSpec::String(), "test payload"),
        tsiemene::directive(IN_DEBUG, tsiemene::DirectiveDir::In,
                            tsiemene::KindSpec::String(), "runtime trace"),
    };
    return std::span<const tsiemene::DirectiveSpec>(kDirectives, 2);
  }

  void step(const tsiemene::Wave&,
            tsiemene::Ingress in,
            tsiemene::BoardContext&,
            tsiemene::Emitter&) override {
    if (in.signal.kind != tsiemene::PayloadKind::String) return;
    if (in.directive == IN_STEP) {
      ++payload_messages;
      return;
    }
    if (in.directive == IN_DEBUG) {
      ++debug_messages;
      if (in.signal.text.rfind("step tsi=", 0) == 0) seen_step_trace = true;
      if (in.signal.text.rfind("step.done tsi=", 0) == 0) seen_step_done_trace = true;
      if (in.signal.text.rfind("route from=", 0) == 0) seen_route_trace = true;
      if (in.signal.text.rfind("drop from=", 0) == 0) seen_drop_trace = true;
      return;
    }
  }

  std::uint64_t payload_messages{0};
  std::uint64_t debug_messages{0};
  bool seen_step_trace{false};
  bool seen_step_done_trace{false};
  bool seen_route_trace{false};
  bool seen_drop_trace{false};

 private:
  tsiemene::TsiId id_{};
};

class backpressure_source_t final : public tsiemene::Tsi {
 public:
  static constexpr tsiemene::DirectiveId IN_STEP = tsiemene::directive_id::Step;
  static constexpr tsiemene::DirectiveId OUT_PAYLOAD = tsiemene::directive_id::Payload;

  explicit backpressure_source_t(tsiemene::TsiId id) : id_(id) {}

  [[nodiscard]] std::string_view type_name() const noexcept override {
    return "tsi.test.backpressure_source";
  }
  [[nodiscard]] std::string_view instance_name() const noexcept override {
    return "backpressure_source";
  }
  [[nodiscard]] tsiemene::TsiId id() const noexcept override { return id_; }
  [[nodiscard]] tsiemene::TsiDomain domain() const noexcept override {
    return tsiemene::TsiDomain::Source;
  }

  [[nodiscard]] std::span<const tsiemene::DirectiveSpec> directives()
      const noexcept override {
    static constexpr tsiemene::DirectiveSpec kDirectives[] = {
        tsiemene::directive(IN_STEP, tsiemene::DirectiveDir::In,
                            tsiemene::KindSpec::String(), "test ingress"),
        tsiemene::directive(OUT_PAYLOAD, tsiemene::DirectiveDir::Out,
                            tsiemene::KindSpec::String(), "test payload"),
    };
    return std::span<const tsiemene::DirectiveSpec>(kDirectives, 2);
  }

  void step(const tsiemene::Wave& wave,
            tsiemene::Ingress in,
            tsiemene::BoardContext&,
            tsiemene::Emitter& out) override {
    if (in.directive != IN_STEP || in.signal.kind != tsiemene::PayloadKind::String) return;
    out.emit_string(wave, OUT_PAYLOAD, "a");
    out.emit_string(wave, OUT_PAYLOAD, "b");
  }

  tsiemene::RuntimeEventAction on_event(const tsiemene::RuntimeEvent& event,
                                        tsiemene::BoardContext&,
                                        tsiemene::Emitter&) override {
    if (event.kind == tsiemene::RuntimeEventKind::Backpressure) {
      ++backpressure_events;
    }
    return tsiemene::RuntimeEventAction{};
  }

  std::uint64_t backpressure_events{0};

 private:
  tsiemene::TsiId id_{};
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
  ok = ok && require(!probe_transfer.has_value(),
                     "legacy transfer-matrix probe family must be removed");
  ok = ok && require(!probe_generic.has_value(),
                     "generic probe canonical tsi type must be removed");

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
  ok = ok && require(required_keys.count(std::string(kBoardContractWaveDslKey)) == 1,
                     "missing board.contract.wave@DSL:str");

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
  RuntimeEvent epoch_start{};
  epoch_start.kind = RuntimeEventKind::EpochStart;
  epoch_start.wave = &wave;
  epoch_start.source = tsi_base;
  epoch_start.target = tsi_base;
  (void)tsi_base->on_event(epoch_start, ctx, emitter);
  RuntimeEvent epoch_end{};
  epoch_end.kind = RuntimeEventKind::EpochEnd;
  epoch_end.wave = &wave;
  epoch_end.source = tsi_base;
  epoch_end.target = tsi_base;
  (void)tsi_base->on_event(epoch_end, ctx, emitter);
  RuntimeEvent queue_drained{};
  queue_drained.kind = RuntimeEventKind::QueueDrained;
  queue_drained.wave = &wave;
  queue_drained.source = tsi_base;
  queue_drained.target = tsi_base;
  const RuntimeEventAction continuation =
      tsi_base->on_event(queue_drained, ctx, emitter);

  ok = ok && require(mock_probe.step_calls == 1,
                     "virtual dispatch (Tsi::step) did not reach probe representation override");
  ok = ok && require(mock_probe.on_event_calls == 3,
                     "virtual dispatch (Tsi::on_event) did not reach probe representation override");
  ok = ok && require(mock_probe.epoch_start_calls == 1,
                     "virtual dispatch (Tsi::on_event EpochStart) did not reach override");
  ok = ok && require(mock_probe.epoch_end_calls == 1,
                     "virtual dispatch (Tsi::on_event EpochEnd) did not reach override");
  ok = ok && require(mock_probe.queue_drained_calls == 1,
                     "virtual dispatch (Tsi::on_event QueueDrained) did not reach override");
  ok = ok && require(continuation.request_continuation,
                     "QueueDrained event must return continuation request");
  ok = ok && require(continuation.continuation_ingress.directive == directive_id::Step,
                     "QueueDrained continuation ingress directive mismatch");
  ok = ok &&
       require(continuation.continuation_ingress.signal.kind == PayloadKind::String,
               "QueueDrained continuation signal kind mismatch");
  ok = ok && require(continuation.continuation_ingress.signal.text == "follow",
                     "QueueDrained continuation payload mismatch");
  ok = ok && require(emitter.emit_count == 1,
                     "probe protocol meta emission failed");
  ok = ok && require(emitter.last_directive == directive_id::Meta,
                     "probe meta must emit @meta directive");
  ok = ok && require(emitter.last_kind == PayloadKind::String,
                     "probe meta must emit string payload");
  ok = ok && require(emitter.last_text == "mock-step",
                     "probe meta payload mismatch");

  runtime_source_t source(100, /*continuation_budget=*/1);
  runtime_sink_t sink(101);
  const Hop flow_hops[] = {
      hop(ep(source, directive_id::Payload), ep(sink, directive_id::Step)),
      hop(ep(source, directive_id::Meta), ep(sink, directive_id::Debug)),
  };
  CompiledCircuit flow_cc{};
  CircuitIssue flow_issue{};
  ok = ok && require(
                 compile_circuit(circuit(flow_hops, "runtime.flow"), &flow_cc,
                                 &flow_issue),
                 "compile_circuit failed for runtime flow test");
  if (ok) {
    RuntimeFlowControl flow_cfg{};
    flow_cfg.trace_level = RuntimeTraceLevel::Verbose;
    flow_cfg.max_queue_size = 0;
    BoardContext flow_ctx{};
    const std::uint64_t flow_steps = run_wave_compiled(
        flow_cc,
        Wave{},
        Ingress{.directive = directive_id::Step, .signal = string_signal("start")},
        flow_ctx,
        flow_cfg);
    ok = ok && require(flow_steps >= 4, "runtime flow must execute source/sink over continuation");
    ok = ok && require(source.step_calls == 2, "QueueDrained continuation must schedule one extra root step");
    ok = ok && require(source.queue_drained_events >= 2, "runtime must emit QueueDrained events");
    ok = ok && require(source.step_start_events >= 2 && source.step_done_events >= 2,
                       "runtime must emit StepStart/StepDone events");
    ok = ok && require(sink.payload_messages == 2, "sink must receive two payloads from continued source");
    ok = ok && require(sink.debug_messages > 0, "trace meta mirror must produce debug messages");
    ok = ok && require(sink.seen_step_trace, "trace meta mirror missing step trace");
    ok = ok && require(sink.seen_step_done_trace, "trace meta mirror missing step.done trace");
    ok = ok && require(sink.seen_route_trace, "trace meta mirror missing route trace");
  }

  backpressure_source_t bp_source(200);
  runtime_sink_t bp_sink(201);
  const Hop bp_hops[] = {
      hop(ep(bp_source, directive_id::Payload), ep(bp_sink, directive_id::Step)),
  };
  CompiledCircuit bp_cc{};
  CircuitIssue bp_issue{};
  ok = ok && require(
                 compile_circuit(circuit(bp_hops, "runtime.backpressure"), &bp_cc,
                                 &bp_issue),
                 "compile_circuit failed for backpressure test");
  if (ok) {
    RuntimeFlowControl bp_flow{};
    bp_flow.trace_level = RuntimeTraceLevel::Off;
    bp_flow.max_queue_size = 1;
    BoardContext bp_ctx{};
    (void)run_wave_compiled(
        bp_cc,
        Wave{},
        Ingress{.directive = directive_id::Step, .signal = string_signal("start")},
        bp_ctx,
        bp_flow);
    ok = ok && require(
                   bp_source.backpressure_events > 0,
                   "runtime must dispatch Backpressure event when queue limit is hit");
  }

  if (!ok) return 1;
  std::cout << "[test_tsi_board_paths] pass\n";
  return 0;
}
