// test_tsi_contract_wave.cpp

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <torch/torch.h>

#include "camahjucunu/dsl/tsiemene_wave/tsiemene_wave.h"
#include "piaabo/dconfig.h"
#include "tsiemene/board.h"
#include "tsiemene/tsi.sink.h"
#include "tsiemene/tsi.source.h"

namespace {

bool require(bool cond, std::string_view msg) {
  if (!cond) {
    std::cerr << "[FAIL] " << msg << "\n";
    return false;
  }
  return true;
}

struct contract_wave_state_t {
  std::vector<std::uint64_t> episodes{};
  std::vector<std::uint64_t> batches{};
};

class source_contract_wave_t final : public tsiemene::TsiSource {
 public:
  static constexpr tsiemene::DirectiveId IN_STEP = tsiemene::directive_id::Step;
  static constexpr tsiemene::DirectiveId OUT_PAYLOAD = tsiemene::directive_id::Payload;

  source_contract_wave_t(tsiemene::TsiId id, std::string instance_name)
      : id_(id), instance_name_(std::move(instance_name)) {}

  [[nodiscard]] std::string_view type_name() const noexcept override {
    return "test.source.contract_wave";
  }
  [[nodiscard]] std::string_view instance_name() const noexcept override {
    return instance_name_;
  }
  [[nodiscard]] tsiemene::TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const tsiemene::DirectiveSpec> directives() const noexcept override {
    static constexpr tsiemene::DirectiveSpec kDirectives[] = {
        tsiemene::directive(
            IN_STEP, tsiemene::DirectiveDir::In, tsiemene::KindSpec::String(), "episode start"),
        tsiemene::directive(
            OUT_PAYLOAD, tsiemene::DirectiveDir::Out, tsiemene::KindSpec::Tensor(), "payload"),
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

  void step(const tsiemene::Wave& wave,
            tsiemene::Ingress in,
            tsiemene::BoardContext&,
            tsiemene::Emitter& out) override {
    continue_requested_ = false;
    if (in.directive != IN_STEP) return;
    if (in.signal.kind != tsiemene::PayloadKind::String) return;

    if (!in.signal.text.empty()) {
      remaining_batches_ = (wave.max_batches_per_epoch > 0) ? wave.max_batches_per_epoch : 1;
    }
    if (remaining_batches_ == 0) return;

    out.emit_tensor(
        wave,
        OUT_PAYLOAD,
        torch::tensor(
            {static_cast<float>(wave.cursor.batch)},
            torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU)));

    --remaining_batches_;
    continue_requested_ = (remaining_batches_ > 0);
  }

  void reset(tsiemene::BoardContext&) override {
    remaining_batches_ = 0;
    continue_requested_ = false;
  }

 private:
  tsiemene::TsiId id_{};
  std::string instance_name_{};
  std::uint64_t remaining_batches_{0};
  bool continue_requested_{false};
};

class sink_contract_wave_t final : public tsiemene::TsiSink {
 public:
  static constexpr tsiemene::DirectiveId IN_STEP = tsiemene::directive_id::Step;

  sink_contract_wave_t(tsiemene::TsiId id, std::string instance_name)
      : id_(id),
        instance_name_(std::move(instance_name)),
        directives_{
            tsiemene::directive(
                IN_STEP, tsiemene::DirectiveDir::In, tsiemene::KindSpec::Tensor(), "payload sink")} {}

  [[nodiscard]] std::string_view type_name() const noexcept override {
    return "test.sink.contract_wave";
  }
  [[nodiscard]] std::string_view instance_name() const noexcept override {
    return instance_name_;
  }
  [[nodiscard]] tsiemene::TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const tsiemene::DirectiveSpec> directives() const noexcept override {
    return std::span<const tsiemene::DirectiveSpec>(directives_.data(), directives_.size());
  }

  void step(const tsiemene::Wave& wave,
            tsiemene::Ingress in,
            tsiemene::BoardContext& ctx,
            tsiemene::Emitter&) override {
    if (in.directive != IN_STEP) return;
    if (in.signal.kind != tsiemene::PayloadKind::Tensor) return;
    auto* state = static_cast<contract_wave_state_t*>(ctx.user);
    if (!state) return;
    state->episodes.push_back(wave.cursor.episode);
    state->batches.push_back(wave.cursor.batch);
  }

 private:
  tsiemene::TsiId id_{};
  std::string instance_name_{};
  std::array<tsiemene::DirectiveSpec, 1> directives_{};
};

std::string compose_source_command(
    const cuwacunu::camahjucunu::tsiemene_wave_source_decl_t& source) {
  return source.symbol + "[" + source.from + "," + source.to + "]";
}

}  // namespace

int main() try {
  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  const std::string contract_hash =
      cuwacunu::piaabo::dconfig::config_space_t::locked_contract_hash();

  const std::string grammar =
      cuwacunu::piaabo::dconfig::contract_space_t::tsiemene_wave_grammar(contract_hash);
  const std::string wave_dsl =
      "WAVE_PROFILE clean_contract_wave {\n"
      "  MODE = train;\n"
      "  EPOCHS = 2;\n"
      "  BATCH_SIZE = 4;\n"
      "  MAX_BATCHES_PER_EPOCH = 3;\n"
      "  WIKIMYEI w_rep {\n"
      "    TRAIN = true;\n"
      "    PROFILE_ID = stable_pretrain;\n"
      "  };\n"
      "  SOURCE w_source {\n"
      "    SYMBOL = BTCUSDT;\n"
      "    FROM = 01.01.2009;\n"
      "    TO = 31.12.2009;\n"
      "  };\n"
      "}\n";

  const auto wave_instruction =
      cuwacunu::camahjucunu::dsl::decode_tsiemene_wave_from_dsl(grammar, wave_dsl);

  bool ok = true;
  ok = ok && require(wave_instruction.profiles.size() == 1, "expected one wave profile");
  if (!ok) return 1;
  const auto& p = wave_instruction.profiles.front();

  ok = ok && require(p.epochs == 2, "expected EPOCHS=2");
  ok = ok && require(p.batch_size == 4, "expected BATCH_SIZE=4");
  ok = ok && require(p.max_batches_per_epoch == 3, "expected MAX_BATCHES_PER_EPOCH=3");
  ok = ok && require(!p.sources.empty(), "expected one SOURCE block");
  if (!ok) return 1;

  tsiemene::Board board{};
  auto& c = board.contracts.emplace_back();
  c.spec.sourced_from_config = false;
  c.name = "clean_contract_wave";
  c.invoke_name = "clean_contract_wave";
  c.invoke_payload = compose_source_command(p.sources.front());
  c.set_dsl_segment(tsiemene::kBoardContractCircuitDslKey, "CIRCUIT clean_contract_wave {}");
  c.set_dsl_segment(
      tsiemene::kBoardContractObservationSourcesDslKey,
      "OBSERVATION_SOURCE w_source {}");
  c.set_dsl_segment(
      tsiemene::kBoardContractObservationChannelsDslKey,
      "OBSERVATION_CHANNEL w_channel {}");
  c.set_dsl_segment(
      tsiemene::kBoardContractJkimyeiSpecsDslKey,
      "COMPONENT w_rep PROFILE stable_pretrain {}");
  c.set_dsl_segment(tsiemene::kBoardContractWaveDslKey, wave_dsl);

  auto& src = c.emplace_node<source_contract_wave_t>(1, "w_source");
  auto& sink = c.emplace_node<sink_contract_wave_t>(2, "w_sink");

  c.hops = {
      tsiemene::hop(
          tsiemene::ep(src, source_contract_wave_t::OUT_PAYLOAD),
          tsiemene::ep(sink, sink_contract_wave_t::IN_STEP),
          tsiemene::query("")),
  };

  c.execution.epochs = p.epochs;
  c.execution.batch_size = p.batch_size;
  c.seed_wave = tsiemene::Wave{
      .cursor = tsiemene::WaveCursor{
          .id = 100,
          .i = 0,
          .episode = 0,
          .batch = 0,
      },
      .max_batches_per_epoch = p.max_batches_per_epoch,
  };
  c.seed_ingress = tsiemene::Ingress{
      .directive = source_contract_wave_t::IN_STEP,
      .signal = tsiemene::string_signal(c.invoke_payload),
  };

  tsiemene::BoardIssue issue{};
  if (!tsiemene::validate_board(board, &issue)) {
    std::cerr << "[FAIL] board validation must pass: " << issue.what << "\n";
    return 1;
  }
  ok = ok && require(true, "board validation must pass");
  if (!ok) return 1;

  contract_wave_state_t state{};
  tsiemene::BoardContext ctx{.user = &state};
  const std::uint64_t steps = tsiemene::run_contract(c, ctx);

  const std::uint64_t expected_batches = p.epochs * p.max_batches_per_epoch;
  const std::uint64_t expected_steps = expected_batches * 2;  // source + sink per emitted batch

  ok = ok && require(state.batches.size() == expected_batches, "unexpected emitted batch count");
  ok = ok && require(state.episodes.size() == expected_batches, "unexpected emitted episode count");
  ok = ok && require(steps == expected_steps, "unexpected runtime event step count");

  for (std::uint64_t i = 0; i < expected_batches; ++i) {
    const std::uint64_t expected_episode = i / p.max_batches_per_epoch;
    const std::uint64_t expected_batch = i % p.max_batches_per_epoch;
    if (state.episodes[static_cast<std::size_t>(i)] != expected_episode) {
      ok = false;
      std::cerr << "[FAIL] episode mismatch at index " << i << "\n";
      break;
    }
    if (state.batches[static_cast<std::size_t>(i)] != expected_batch) {
      ok = false;
      std::cerr << "[FAIL] batch mismatch at index " << i << "\n";
      break;
    }
  }

  if (!ok) return 1;
  std::cout << "[test_tsi_contract_wave] pass epochs=" << p.epochs
            << " max_batches_per_epoch=" << p.max_batches_per_epoch
            << " emitted_batches=" << expected_batches
            << " steps=" << steps << "\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << "[test_tsi_contract_wave] exception: " << e.what() << "\n";
  return 1;
}
