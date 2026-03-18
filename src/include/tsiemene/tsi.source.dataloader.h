// ./include/tsiemene/tsi.source.dataloader.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "tsiemene/tsi.source.h"
#include "tsiemene/tsi.cargo.validation.h"

// ---- Real dataloader hook -------------------------------------------------
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/data/observation_sample.h"
#include "piaabo/torch_compat/data_analytics.h"
#include "source/dataloader/dataloader_component.h"
#include "source/evaluation/source_data_analytics.h"

// ------------------------------------------------------------
// RUNTIME WARNINGS (non-blocking / informational) .h
// ------------------------------------------------------------
DEV_WARNING("[tsi.source.dataloader.h] TODO: add regime/OOD-aware sampling policies for representation evaluation (blocked temporal splits, regime holdout, cross-instrument holdout). Current sampler policy is sequential/random index traversal only.\n");
DEV_WARNING("[tsi.source.dataloader.h] TODO: there are other forms of normalization e.g. log(x_t/x_(t-1)), volatility scaling, or z-score windows.");

namespace tsiemene {

// NOTE:
//  - Datatype_t is your record struct type (e.g. exchange::kline_t).
//  - Sampler_t controls determinism/order (SequentialSampler vs RandomSampler).
//  - Reproducibility contract: same dataset files + config + seed + sampler +
//    command sequence yields the same emitted batch/key sequence.
template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::RandomSampler>
class TsiSourceDataloader final : public TsiSource {
 public:
  static constexpr DirectiveId IN_STEP     = directive_id::Step;
  static constexpr DirectiveId OUT_PAYLOAD = directive_id::Payload;
  static constexpr DirectiveId OUT_META    = directive_id::Meta;

  using Dataset_t    = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
  using Sample_t     = cuwacunu::camahjucunu::data::observation_sample_t;
  using Loader_t     = cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Sample_t, Datatype_t, Sampler_t>;
  using Iterator_t   = decltype(std::declval<Loader_t&>().begin());

  TsiSourceDataloader(TsiId id,
                      std::string instrument,
                      const cuwacunu::camahjucunu::observation_spec_t& observation_instruction,
                      torch::Device device = torch::kCPU,
                      std::size_t batch_size_override = 0,
                      std::uint64_t dataloader_workers = 0,
                      bool force_rebuild_cache = true,
                      std::uint64_t range_warn_batches = 256,
                      std::string contract_hash = {})
      : id_(id),
        instrument_(std::move(instrument)),
        type_name_(std::string(cuwacunu::source::dataloader::kCanonicalType)),
        instance_name_(
            cuwacunu::source::dataloader::make_display_instance_name(
                instrument_)),
        contract_hash_(std::move(contract_hash)),
        dataloader_workers_(dataloader_workers),
        force_rebuild_cache_(force_rebuild_cache),
        range_warn_batches_(std::max<std::uint64_t>(1, range_warn_batches)),
        device_(device),
        dataset_(make_dataset_(
            instrument_, observation_instruction, force_rebuild_cache)),
        dl_(make_loader_(dataset_, batch_size_override, dataloader_workers)),
        data_analytics_options_(
            data_analytics_options_from_observation_(observation_instruction)),
        data_symbolic_channel_descriptors_(
            make_data_symbolic_channel_descriptors_(
                instrument_, observation_instruction)) {
    C_ = dl_.C_;
    T_ = dl_.T_;
    D_ = dl_.D_;
    B_hint_ = static_cast<int64_t>(dl_.inner().options().batch_size);
    it_ = dl_.begin();
    end_ = dl_.end();
    iter_ready_ = true;
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return type_name_; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }

  // Optional: expose discovered dims so callers can instantiate VICReg consistently.
  [[nodiscard]] int64_t C() const noexcept { return C_; }
  [[nodiscard]] int64_t T() const noexcept { return T_; }
  [[nodiscard]] int64_t D() const noexcept { return D_; }
  [[nodiscard]] int64_t batch_size_hint() const noexcept { return B_hint_; }
  [[nodiscard]] const std::filesystem::path& latest_data_analytics_file() const noexcept {
    return latest_data_analytics_file_;
  }
  [[nodiscard]] const std::filesystem::path&
  latest_data_analytics_symbolic_file() const noexcept {
    return latest_data_analytics_symbolic_file_;
  }

  [[nodiscard]] std::span<const DirectiveSpec> directives() const noexcept override {
    static constexpr DirectiveSpec kDirectives[] = {
      directive(IN_STEP, DirectiveDir::In, KindSpec::String(),
                "episode command (e.g. \"batches=10\" or \"BTCUSDT[01.01.2009,31.12.2009]\"); empty means continue active episode; wave time-span can define range"),
      directive(OUT_PAYLOAD, DirectiveDir::Out, KindSpec::Cargo(),
                "observation cargo payload (features/mask/future_features/keys/stats/encoding)"),
      directive(OUT_META, DirectiveDir::Out, KindSpec::String(),
                "runtime trace/meta stream with cursor/state diagnostics"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 3);
  }
  [[nodiscard]] bool supports_init_report_fragments() const noexcept override {
    return true;
  }
  [[nodiscard]] std::string_view init_report_fragment_schema() const noexcept override {
    return "tsi.source.dataloader.init.v1";
  }

  [[nodiscard]] Determinism determinism() const noexcept override {
    if constexpr (std::is_same_v<Sampler_t, torch::data::samplers::SequentialSampler>) {
      return Determinism::Deterministic;
    } else {
      return Determinism::SeededStochastic;
    }
  }

  RuntimeEventAction on_event(const RuntimeEvent& event,
                              RuntimeContext& ctx,
                              Emitter& out) override {
    RuntimeEventAction action{};
    if (event.kind == RuntimeEventKind::RunStart) {
      clear_episode_state_();
      it_ = dl_.begin();
      end_ = dl_.end();
      iter_ready_ = true;
      continue_requested_ = false;
      data_analytics_signature_.clear();
      data_analytics_report_ready_ = false;
      const Wave empty_wave{};
      const Wave& runtime_wave = event.wave ? *event.wave : empty_wave;
      ensure_data_analytics_report_(
          runtime_wave,
          analytics_command_from_wave_(event.wave),
          ctx,
          out);
      return action;
    }
    if (event.kind == RuntimeEventKind::EpochStart) {
      clear_episode_state_();
      it_ = dl_.begin();
      end_ = dl_.end();
      iter_ready_ = true;
      continue_requested_ = false;
      return action;
    }
    if (event.kind == RuntimeEventKind::RunEnd) {
      clear_episode_state_();
      continue_requested_ = false;
      return action;
    }
    if (event.kind == RuntimeEventKind::QueueDrained && continue_requested_) {
      action.request_continuation = true;
      action.continuation_ingress = Ingress{
          .directive = IN_STEP,
          .signal = string_signal(""),
      };
      return action;
    }
    return action;
  }

  void step(const Wave& wave, Ingress in, RuntimeContext& ctx, Emitter& out) override {
    continue_requested_ = false;
    if (in.directive != IN_STEP) return;
    if (in.signal.kind != PayloadKind::String) return;

    const std::string_view cmd_text = trim_(in.signal.text);
    if (!cmd_text.empty()) {
      if (!start_episode_(wave, cmd_text, out)) return;
    } else if (!episode_active_) {
      emit_meta_(wave, out, "dataloader.continue noop reason=no-active-episode");
      return;
    }

    PackedBatch pb = next_episode_batch_();
    if (!pb.sample) {
      std::ostringstream oss;
      oss << "dataloader.episode done emitted=" << episode_emitted_;
      if (!active_cmd_.has_range && batch_remaining_ > 0) {
        oss << " reason=exhausted";
      }
      finish_episode_(ctx, out, oss.str());
      return;
    }

    const Wave witem{
        .cursor = WaveCursor{
            .id = episode_wave_id_,
            .i = episode_next_wave_i_,
            .episode = episode_wave_episode_,
            .batch = episode_next_batch_,
        },
        .span_begin_ms = episode_wave_span_begin_ms_,
        .span_end_ms = episode_wave_span_end_ms_,
        .has_time_span = episode_wave_has_time_span_,
    };
    emit_batch_state_meta_(witem, out);
    std::string cargo_error;
    if (!validate_observation_cargo(pb.sample, CargoValidationStage::SourceOut, &cargo_error)) {
      emit_meta_(witem, out, std::string("cargo.invalid stage=source.out reason=") + cargo_error);
    } else {
      out.emit_cargo(witem, OUT_PAYLOAD, std::move(pb.sample));
      ++episode_emitted_;
    }
    ++episode_next_wave_i_;
    ++episode_next_batch_;

    continue_requested_ = episode_has_more_();
    if (!continue_requested_) {
      std::ostringstream oss;
      oss << "dataloader.episode done emitted=" << episode_emitted_;
      if (episode_emitted_ > 0) {
        oss << " wave_i=[" << episode_wave_i0_ << "," << (episode_next_wave_i_ - 1) << "]";
        oss << " batch=[" << episode_batch_i0_ << "," << (episode_next_batch_ - 1) << "]";
        oss << " episode=" << episode_wave_episode_;
      } else {
        oss << " wave_i=<none>";
      }
      finish_episode_(ctx, out, oss.str());
    }
  }

 private:
  using Key_t = typename Datatype_t::key_type_t;

  struct CommandSpec {
    std::uint64_t batches{0};
    bool has_range{false};
    bool range_from_wave{false};
    Key_t key_left{};
    Key_t key_right{};
  };

  [[nodiscard]] static std::string_view trim_(std::string_view s) noexcept {
    while (!s.empty() && static_cast<unsigned char>(s.front()) <= ' ') s.remove_prefix(1);
    while (!s.empty() && static_cast<unsigned char>(s.back()) <= ' ') s.remove_suffix(1);
    return s;
  }

  [[nodiscard]] static std::optional<std::uint64_t> parse_u64_(std::string_view s) noexcept {
    std::uint64_t v = 0;
    auto r = std::from_chars(s.data(), s.data() + s.size(), v);
    if (r.ec != std::errc() || r.ptr != s.data() + s.size()) return std::nullopt;
    return v;
  }

  [[nodiscard]] static std::optional<int> parse_i32_(std::string_view s) noexcept {
    int v = 0;
    auto r = std::from_chars(s.data(), s.data() + s.size(), v);
    if (r.ec != std::errc() || r.ptr != s.data() + s.size()) return std::nullopt;
    return v;
  }

  [[nodiscard]] static constexpr bool is_leap_year_(int y) noexcept {
    return ((y % 4) == 0 && (y % 100) != 0) || ((y % 400) == 0);
  }

  [[nodiscard]] static constexpr int days_in_month_(int y, int m) noexcept {
    constexpr int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m < 1 || m > 12) return 0;
    if (m == 2 && is_leap_year_(y)) return 29;
    return kDays[m - 1];
  }

  // Howard Hinnant's civil-from-days conversion.
  // Returns whole UTC days since unix epoch (1970-01-01).
  [[nodiscard]] static constexpr std::int64_t days_from_civil_utc_(int y, unsigned m, unsigned d) noexcept {
    y -= (m <= 2) ? 1 : 0;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);                 // [0, 399]
    const unsigned mp = (m > 2) ? (m - 3) : (m + 9);
    const unsigned doy = (153 * mp + 2) / 5 + d - 1;                            // [0, 365]
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;                // [0, 146096]
    return static_cast<std::int64_t>(era) * 146097 + static_cast<std::int64_t>(doe) - 719468;
  }

  [[nodiscard]] static std::optional<std::int64_t>
  parse_ddmmyyyy_to_unix_ms_(std::string_view ddmmyyyy, bool end_of_day) noexcept {
    const std::size_t p1 = ddmmyyyy.find('.');
    if (p1 == std::string_view::npos) return std::nullopt;
    const std::size_t p2 = ddmmyyyy.find('.', p1 + 1);
    if (p2 == std::string_view::npos) return std::nullopt;

    const auto d = parse_i32_(ddmmyyyy.substr(0, p1));
    const auto m = parse_i32_(ddmmyyyy.substr(p1 + 1, p2 - p1 - 1));
    const auto y = parse_i32_(ddmmyyyy.substr(p2 + 1));
    if (!d || !m || !y) return std::nullopt;
    if (*m < 1 || *m > 12 || *y < 1970) return std::nullopt;

    const int dim = days_in_month_(*y, *m);
    if (*d < 1 || *d > dim) return std::nullopt;

    constexpr std::int64_t kMsPerDay = 24LL * 60LL * 60LL * 1000LL;
    const std::int64_t day_index = days_from_civil_utc_(*y, static_cast<unsigned>(*m), static_cast<unsigned>(*d));
    if (day_index < 0) return std::nullopt;

    const std::int64_t day_start_ms = day_index * kMsPerDay;
    return day_start_ms + (end_of_day ? (kMsPerDay - 1) : 0);
  }

  static std::optional<std::uint64_t> parse_batches_explicit_(std::string_view s) noexcept {
    // Preferred form: "batches=10"
    constexpr std::string_view kKey = "batches=";
    const std::size_t at = s.find(kKey);
    if (at == std::string_view::npos) return std::nullopt;

    std::size_t i = at + kKey.size();
    std::size_t j = i;
    while (j < s.size() && s[j] >= '0' && s[j] <= '9') ++j;
    if (j == i) return std::uint64_t{0};
    if (auto v = parse_u64_(s.substr(i, j - i)); v.has_value()) return *v;
    return std::uint64_t{0};
  }

  [[nodiscard]] bool parse_range_keys_(std::string_view s, Key_t& out_left, Key_t& out_right) const noexcept {
    const std::size_t lb = s.find('[');
    const std::size_t rb = s.rfind(']');
    if (lb == std::string_view::npos || rb == std::string_view::npos || rb <= lb + 1) return false;

    std::string_view symbol = trim_(s.substr(0, lb));
    const std::size_t semi = symbol.rfind(';');
    if (semi != std::string_view::npos) symbol = trim_(symbol.substr(semi + 1));
    if (!symbol.empty() && symbol != std::string_view(instrument_)) return false;

    const std::string_view inside = trim_(s.substr(lb + 1, rb - lb - 1));
    const std::size_t comma = inside.find(',');
    if (comma == std::string_view::npos) return false;

    const std::string_view d0 = trim_(inside.substr(0, comma));
    const std::string_view d1 = trim_(inside.substr(comma + 1));
    const auto ms0 = parse_ddmmyyyy_to_unix_ms_(d0, /*end_of_day=*/false);
    const auto ms1 = parse_ddmmyyyy_to_unix_ms_(d1, /*end_of_day=*/true);
    if (!ms0 || !ms1) return false;

    std::int64_t left = *ms0;
    std::int64_t right = *ms1;
    if (left > right) std::swap(left, right);

    out_left = static_cast<Key_t>(left);
    out_right = static_cast<Key_t>(right);
    return true;
  }

  [[nodiscard]] CommandSpec parse_command_(std::string_view s, const Wave& wave) const noexcept {
    CommandSpec cmd{};
    cmd.has_range = parse_range_keys_(s, cmd.key_left, cmd.key_right);
    if (!cmd.has_range && wave.has_time_span) {
      cmd.has_range = true;
      cmd.range_from_wave = true;
      cmd.key_left = static_cast<Key_t>(std::min(wave.span_begin_ms, wave.span_end_ms));
      cmd.key_right = static_cast<Key_t>(std::max(wave.span_begin_ms, wave.span_end_ms));
    }
    // In range mode, do not infer batches from date digits.
    cmd.batches = parse_batches_explicit_(s).value_or(0);
    if (cmd.has_range && cmd.batches == 0 && wave.max_batches_per_epoch > 0) {
      cmd.batches = wave.max_batches_per_epoch;
    }
    return cmd;
  }

  [[nodiscard]] CommandSpec analytics_command_from_wave_(
      const Wave* wave) const noexcept {
    CommandSpec cmd{};
    if (!wave || !wave->has_time_span) return cmd;
    cmd.has_range = true;
    cmd.range_from_wave = true;
    cmd.key_left =
        static_cast<Key_t>(std::min(wave->span_begin_ms, wave->span_end_ms));
    cmd.key_right =
        static_cast<Key_t>(std::max(wave->span_begin_ms, wave->span_end_ms));
    return cmd;
  }

  static Dataset_t make_dataset_(
      std::string_view instrument,
      const cuwacunu::camahjucunu::observation_spec_t& observation_instruction,
      bool force_rebuild_cache) {
    return cuwacunu::camahjucunu::data::create_memory_mapped_concat_dataset<Datatype_t>(
        std::string(instrument), observation_instruction,
        force_rebuild_cache);
  }

  static Loader_t make_loader_(Dataset_t& dataset,
                               std::size_t batch_size_override,
                               std::uint64_t dataloader_workers) {
    constexpr std::size_t kDefaultBatchSize = 64;
    std::size_t batch_size = kDefaultBatchSize;
    if (batch_size_override > 0) {
      batch_size = batch_size_override;
    }
    const std::size_t workers = static_cast<std::size_t>(dataloader_workers);

    if constexpr (std::is_same_v<Sampler_t, torch::data::samplers::SequentialSampler>) {
      auto sampler = dataset.SequentialSampler();
      auto opts = dataset.SequentialSampler_options(batch_size, workers);
      return Loader_t(dataset, sampler, opts);
    } else {
      auto sampler = dataset.RandomSampler();
      auto opts = dataset.RandomSampler_options(batch_size, workers);
      return Loader_t(dataset, sampler, opts);
    }
  }

  [[nodiscard]] static constexpr std::string_view record_type_name_for_datatype_() noexcept {
    if constexpr (std::is_same_v<Datatype_t, cuwacunu::camahjucunu::exchange::kline_t> ||
                  std::is_same_v<Datatype_t, cuwacunu::camahjucunu::exchange::kline_cache_t>) {
      return "kline";
    } else if constexpr (std::is_same_v<Datatype_t, cuwacunu::camahjucunu::exchange::trade_t> ||
                         std::is_same_v<Datatype_t, cuwacunu::camahjucunu::exchange::trade_cache_t>) {
      return "trade";
    } else if constexpr (std::is_same_v<Datatype_t, cuwacunu::camahjucunu::exchange::basic_t> ||
                         std::is_same_v<Datatype_t, cuwacunu::camahjucunu::exchange::basic_cache_t>) {
      return "basic";
    } else {
      return {};
    }
  }

  [[nodiscard]] static std::string join_csv_(
      const std::vector<std::string>& parts) {
    if (parts.empty()) return {};
    std::ostringstream oss;
    for (std::size_t i = 0; i < parts.size(); ++i) {
      if (i > 0) oss << ",";
      oss << parts[i];
    }
    return oss.str();
  }

  [[nodiscard]] static std::vector<
      cuwacunu::piaabo::torch_compat::data_symbolic_channel_descriptor_t>
  make_data_symbolic_channel_descriptors_(
      std::string_view instrument,
      const cuwacunu::camahjucunu::observation_spec_t& observation_instruction) {
    using descriptor_t =
        cuwacunu::piaabo::torch_compat::data_symbolic_channel_descriptor_t;

    std::vector<descriptor_t> out{};
    const std::string_view expected_record_type = record_type_name_for_datatype_();
    if (expected_record_type.empty()) return out;

    for (const auto& channel_form : observation_instruction.channel_forms) {
      if (channel_form.active != "true") continue;
      if (channel_form.record_type != expected_record_type) continue;

      for (const auto& source_form : observation_instruction.filter_source_forms(
               std::string(instrument),
               channel_form.record_type,
               channel_form.interval)) {
        descriptor_t descriptor{};
        descriptor.record_type = source_form.record_type;
        descriptor.label = source_form.instrument + "/" +
                           cuwacunu::camahjucunu::exchange::enum_to_string(
                               source_form.interval) +
                           "/" + source_form.record_type;
        descriptor.anchor_feature = std::string(
            cuwacunu::piaabo::torch_compat::
                data_symbolic_anchor_feature_name_for_record_type(
                    source_form.record_type));
        descriptor.anchor_feature_index =
            cuwacunu::piaabo::torch_compat::
                data_symbolic_anchor_feature_index_for_record_type(
                    source_form.record_type);
        descriptor.feature_names = join_csv_(
            cuwacunu::piaabo::torch_compat::data_feature_names_for_record_type(
                source_form.record_type));
        out.push_back(std::move(descriptor));
      }
    }

    return out;
  }

  [[nodiscard]] std::uint64_t range_warn_batches_threshold_() const {
    return range_warn_batches_;
  }

  struct PackedBatch {
    ObservationCargoPtr sample{};
  };

  void move_sample_to_device_(Sample_t* sample) const {
    if (!sample) return;
    if (device_ == torch::kCPU) return;
    auto move_tensor = [&](torch::Tensor* t) {
      if (!t || !t->defined()) return;
      *t = t->to(device_);
    };
    move_tensor(&sample->features);
    move_tensor(&sample->mask);
    move_tensor(&sample->future_features);
    move_tensor(&sample->future_mask);
    move_tensor(&sample->encoding);
    move_tensor(&sample->feature_mean);
    move_tensor(&sample->feature_std);
    move_tensor(&sample->past_keys);
    move_tensor(&sample->future_keys);
  }

  [[nodiscard]] PackedBatch pack_batch_(std::vector<Sample_t>&& sample_batch) const {
    if (sample_batch.empty()) return PackedBatch{};

    Sample_t coll = Sample_t::collate_fn(sample_batch);
    move_sample_to_device_(&coll);
    return PackedBatch{
        .sample = std::make_shared<Sample_t>(std::move(coll)),
    };
  }

  [[nodiscard]] PackedBatch next_loader_batch_() {
    if (!iter_ready_) {
      it_  = dl_.begin();
      end_ = dl_.end();
      iter_ready_ = true;
    }

    // If epoch ended, start a new one.
    // IMPORTANT: only do this after iterator is exhausted.
    if (it_ == end_) {
      it_  = dl_.begin();
      end_ = dl_.end();
      if (it_ == end_) return PackedBatch{}; // empty loader
    }

    // Move the produced batch out of the iterator to avoid copying.
    auto& batch_ref = *it_;
    std::vector<Sample_t> batch = std::move(batch_ref);
    ++it_;

    return pack_batch_(std::move(batch));
  }

  void clear_episode_state_() {
    episode_active_ = false;
    continue_requested_ = false;
    active_cmd_ = CommandSpec{};
    batch_remaining_ = 0;
    range_batch_limit_ = 0;
    range_begin_idx_ = 0;
    range_count_ = 0;
    range_cursor_ = 0;
    episode_emitted_ = 0;
    episode_wave_id_ = 0;
    episode_wave_i0_ = 0;
    episode_next_wave_i_ = 0;
    episode_wave_episode_ = 0;
    episode_batch_i0_ = 0;
    episode_next_batch_ = 0;
    episode_wave_has_time_span_ = false;
    episode_wave_span_begin_ms_ = 0;
    episode_wave_span_end_ms_ = 0;
  }

  [[nodiscard]] bool start_episode_(const Wave& wave,
                                    std::string_view cmd_text,
                                    Emitter& out) {
    clear_episode_state_();
    active_cmd_ = parse_command_(cmd_text, wave);
    emit_command_meta_(wave, active_cmd_, out);

    episode_wave_id_ = wave.cursor.id;
    episode_wave_i0_ = wave.cursor.i;
    episode_next_wave_i_ = wave.cursor.i;
    episode_wave_episode_ = wave.cursor.episode;
    episode_batch_i0_ = wave.cursor.batch;
    episode_next_batch_ = wave.cursor.batch;
    episode_wave_has_time_span_ = wave.has_time_span;
    episode_wave_span_begin_ms_ = wave.span_begin_ms;
    episode_wave_span_end_ms_ = wave.span_end_ms;

    if (active_cmd_.has_range) {
      const bool has_range =
          dataset_.compute_index_range_by_keys(
              active_cmd_.key_left, active_cmd_.key_right, &range_begin_idx_, &range_count_);
      range_cursor_ = 0;
      range_batch_limit_ = (active_cmd_.batches > 0)
          ? active_cmd_.batches
          : std::numeric_limits<std::uint64_t>::max();
      const std::size_t batch_size =
          (B_hint_ > 0) ? static_cast<std::size_t>(B_hint_) : std::size_t(64);
      const std::uint64_t estimated_batches =
          (batch_size == 0)
              ? 0ULL
              : static_cast<std::uint64_t>((range_count_ + batch_size - 1) / batch_size);

      std::ostringstream oss;
      oss << "dataloader.range-mode setup samples=" << range_count_
          << " estimated_batches=" << estimated_batches
          << " batch_size=" << ((B_hint_ > 0) ? B_hint_ : 64);
      if (active_cmd_.range_from_wave) {
        oss << " source=wave.span";
      } else {
        oss << " source=command";
      }
      if (active_cmd_.batches > 0) {
        oss << " max_batches=" << range_batch_limit_;
      } else {
        oss << " max_batches=unbounded";
      }
      emit_meta_(wave, out, oss.str());

      if (!has_range || range_count_ == 0) {
        emit_meta_(wave, out, "dataloader.range-mode noop reason=no-samples wave_i=<none>");
        clear_episode_state_();
        return false;
      }

      if (active_cmd_.batches == 0) {
        const std::uint64_t warn_threshold = range_warn_batches_threshold_();
        if (estimated_batches > warn_threshold) {
          std::ostringstream warn;
          warn << "dataloader.range-mode warning=large-range-unbounded"
               << " estimated_batches=" << estimated_batches
               << " threshold=" << warn_threshold;
          emit_meta_(wave, out, warn.str());
        }
      }

      episode_active_ = true;
      return true;
    }

    if (active_cmd_.batches == 0) {
      emit_meta_(wave, out, "dataloader.batch-mode noop requested=0 wave_i=<none>");
      clear_episode_state_();
      return false;
    }

    // Batch-mode episodes intentionally continue from the shared loader cursor.
    // Starting a new episode does not rewind the loader iterator.
    batch_remaining_ = active_cmd_.batches;
    {
      std::ostringstream oss;
      oss << "dataloader.batch-mode setup requested=" << batch_remaining_
          << " cursor=continue-from-loader";
      emit_meta_(wave, out, oss.str());
    }
    episode_active_ = true;
    return true;
  }

  [[nodiscard]] PackedBatch next_episode_batch_() {
    if (!episode_active_) return PackedBatch{};

    if (active_cmd_.has_range) {
      if (range_cursor_ >= range_count_) return PackedBatch{};
      if (episode_emitted_ >= range_batch_limit_) return PackedBatch{};

      const std::size_t batch_size = (B_hint_ > 0) ? static_cast<std::size_t>(B_hint_) : std::size_t(64);
      const std::size_t e = std::min(range_cursor_ + batch_size, range_count_);
      std::vector<Sample_t> chunk;
      chunk.reserve(e - range_cursor_);
      for (std::size_t j = range_cursor_; j < e; ++j) {
        chunk.emplace_back(dataset_.get(range_begin_idx_ + j));
      }
      range_cursor_ = e;
      return pack_batch_(std::move(chunk));
    }

    if (batch_remaining_ == 0) return PackedBatch{};
    PackedBatch out = next_loader_batch_();
    if (!out.sample) return PackedBatch{};
    --batch_remaining_;
    return out;
  }

  [[nodiscard]] bool episode_has_more_() const {
    if (!episode_active_) return false;
    if (active_cmd_.has_range) {
      if (episode_emitted_ >= range_batch_limit_) return false;
      return range_cursor_ < range_count_;
    }
    return batch_remaining_ > 0;
  }

  void finish_episode_(const RuntimeContext& ctx, Emitter& out, std::string msg) {
    (void)ctx;
    const Wave w{
        .cursor = WaveCursor{
            .id = episode_wave_id_,
            .i = (episode_emitted_ > 0) ? (episode_next_wave_i_ - 1) : episode_wave_i0_,
            .episode = episode_wave_episode_,
            .batch = (episode_emitted_ > 0) ? (episode_next_batch_ - 1) : episode_batch_i0_,
        },
        .span_begin_ms = episode_wave_span_begin_ms_,
        .span_end_ms = episode_wave_span_end_ms_,
        .has_time_span = episode_wave_has_time_span_,
    };
    emit_meta_(w, out, std::move(msg));
    clear_episode_state_();
  }

  void emit_command_meta_(const Wave& wave, const CommandSpec& cmd, Emitter& out) const {
    std::ostringstream oss;
    if (cmd.has_range) {
      oss << "dataloader.command mode=range";
      if (cmd.range_from_wave) {
        oss << " source=wave.span";
      } else {
        oss << " source=command";
      }
      if constexpr (std::is_integral_v<Key_t>) {
        if constexpr (std::is_signed_v<Key_t>) {
          oss << " key_ms=[" << static_cast<long long>(cmd.key_left)
              << "," << static_cast<long long>(cmd.key_right) << "]";
        } else {
          oss << " key_ms=[" << static_cast<unsigned long long>(cmd.key_left)
              << "," << static_cast<unsigned long long>(cmd.key_right) << "]";
        }
      }
      if (cmd.batches > 0) {
        oss << " batch_limit=" << cmd.batches;
      } else {
        oss << " batch_limit=unbounded";
      }
    } else {
      oss << "dataloader.command mode=batch-count requested=" << cmd.batches
          << " cursor=continue-from-loader";
    }
    emit_meta_(wave, out, oss.str());
  }

  void emit_meta_(const Wave& wave, Emitter& out, std::string msg) const {
    out.emit_string(wave, OUT_META, std::move(msg));
  }

  void emit_batch_state_meta_(const Wave& wave, Emitter& out) const {
    std::ostringstream oss;
    oss << "dataloader.state"
        << " mode=" << (active_cmd_.has_range ? "range" : "batch")
        << " sampler="
        << (std::is_same_v<Sampler_t, torch::data::samplers::SequentialSampler>
                ? "sequential"
                : "random")
        << " wave(id=" << static_cast<unsigned long long>(wave.cursor.id)
        << ",i=" << static_cast<unsigned long long>(wave.cursor.i)
        << ",episode=" << static_cast<unsigned long long>(wave.cursor.episode)
        << ",batch=" << static_cast<unsigned long long>(wave.cursor.batch) << ")"
        << " emitted=" << static_cast<unsigned long long>(episode_emitted_)
        << " remaining=" << static_cast<unsigned long long>(batch_remaining_)
        << " range_cursor=" << static_cast<unsigned long long>(range_cursor_)
        << "/" << static_cast<unsigned long long>(range_count_)
        << " range_batch_limit=" << static_cast<unsigned long long>(range_batch_limit_)
        << " batch_hint=" << static_cast<long long>(B_hint_)
        << " dims=[C=" << static_cast<long long>(C_)
        << ",T=" << static_cast<long long>(T_)
        << ",D=" << static_cast<long long>(D_) << "]"
        << " dataloader(workers=" << static_cast<unsigned long long>(dataloader_workers_)
        << ",force_rebuild_cache=" << (force_rebuild_cache_ ? "true" : "false")
        << ",range_warn_batches=" << static_cast<unsigned long long>(range_warn_batches_) << ")"
        << " device=" << device_.str();
    emit_meta_(wave, out, oss.str());
  }

  [[nodiscard]] static std::string format_data_analytics_debug_report_(
      const cuwacunu::piaabo::torch_compat::data_source_analytics_report_t& report,
      const cuwacunu::piaabo::torch_compat::data_analytics_options_t& options,
      std::string_view source_label,
      const std::filesystem::path& output_file,
      bool use_color) {
    const char* c_reset = use_color ? "\x1b[0m" : "";
    const char* c_title = use_color ? "\x1b[1;96m" : "";
    const char* c_key = use_color ? "\x1b[90m" : "";
    const char* c_value = use_color ? "\x1b[97m" : "";
    const char* c_note = use_color ? "\x1b[36m" : "";
    const char* c_focus_value = use_color ? "\x1b[95m" : "";

    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(6);

    auto line = [&](std::string_view key, const auto& value, std::string_view note) {
      oss << "\t" << c_key << std::left << std::setw(30) << key << c_reset
          << " : " << c_value << value << c_reset
          << "\t" << c_note << note << c_reset << "\n";
    };

    oss << c_title << "Source Data Analytics Report" << c_reset << "\n";
    if (!source_label.empty()) {
      line("source_label", source_label, "tsi source instance");
    }
    line("schema", report.schema, "report schema id");
    line("file", output_file.string(), "persisted latest report");
    line("valid_sample_count", report.valid_sample_count, "samples used for estimation");
    line("source_effective_feature_count",
         report.source_effective_feature_count,
         "mask-filtered feature count");
    line("source_cov_trace", report.source_cov_trace, "standardized covariance trace");
    line("source_nonzero_eigen_count",
         report.source_nonzero_eigen_count,
         "eigenvalues > standardize_epsilon");
    line("max_samples", options.max_samples, "configured analytics cap");
    oss << "\t" << c_key << std::left << std::setw(30) << "source_entropic_load"
        << c_reset << " : " << c_focus_value << report.source_entropic_load << c_reset
        << "\t" << c_note << "source entropy load" << c_reset << "\n";
    return oss.str();
  }

  [[nodiscard]] static std::string key_to_text_(Key_t key) {
    std::ostringstream oss;
    if constexpr (std::is_integral_v<Key_t>) {
      if constexpr (std::is_signed_v<Key_t>) {
        oss << static_cast<long long>(key);
      } else {
        oss << static_cast<unsigned long long>(key);
      }
    } else {
      oss.setf(std::ios::fixed);
      oss << std::setprecision(12) << static_cast<long double>(key);
    }
    return oss.str();
  }

  [[nodiscard]] bool resolve_analytics_range_(
      const CommandSpec& cmd,
      std::size_t* out_begin_idx,
      std::size_t* out_count,
      Key_t* out_left_key,
      Key_t* out_right_key) const {
    if (!out_begin_idx || !out_count || !out_left_key || !out_right_key) {
      return false;
    }
    *out_begin_idx = 0;
    *out_count = 0;

    if (cmd.has_range) {
      if (!dataset_.compute_index_range_by_keys(
              cmd.key_left, cmd.key_right, out_begin_idx, out_count)) {
        return false;
      }
      *out_left_key = std::min(cmd.key_left, cmd.key_right);
      *out_right_key = std::max(cmd.key_left, cmd.key_right);
      return *out_count > 0;
    }

    const auto total_opt = dataset_.size();
    if (!total_opt.has_value() || *total_opt == 0) return false;

    *out_begin_idx = 0;
    *out_count = *total_opt;
    *out_left_key = dataset_.leftmost_key_value_;
    *out_right_key = dataset_.rightmost_key_value_;
    return true;
  }

  [[nodiscard]] std::string build_data_analytics_signature_(
      Key_t left_key,
      Key_t right_key,
      std::size_t begin_idx,
      std::size_t count) const {
    const auto total_opt = dataset_.size();
    const std::size_t total_records = total_opt.has_value() ? *total_opt : 0;

    std::ostringstream dataset_sig;
    dataset_sig.setf(std::ios::fixed);
    dataset_sig << "records=" << total_records
                << ",left=" << key_to_text_(dataset_.leftmost_key_value_)
                << ",right=" << key_to_text_(dataset_.rightmost_key_value_)
                << ",step=" << key_to_text_(dataset_.key_value_step_)
                << ",max_N_past=" << dataset_.max_N_past_
                << ",max_N_future=" << dataset_.max_N_future_;

    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << "contract_hash=" << contract_hash_
        << "|source=" << instance_name_
        << "|dataset_sig=" << dataset_sig.str()
        << "|left_key=" << key_to_text_(left_key)
        << "|right_key=" << key_to_text_(right_key)
        << "|begin_idx=" << begin_idx
        << "|count=" << count
        << "|max_samples=" << data_analytics_options_.max_samples
        << "|max_features=" << data_analytics_options_.max_features
        << "|mask_epsilon=" << data_analytics_options_.mask_epsilon
        << "|standardize_epsilon=" << data_analytics_options_.standardize_epsilon;
    return oss.str();
  }

  void ensure_data_analytics_report_(const Wave& wave,
                                     const CommandSpec& cmd,
                                     const RuntimeContext& ctx,
                                     Emitter& out) {
    if (!ctx.debug_enabled) return;
    if (contract_hash_.empty()) return;

    std::size_t begin_idx = 0;
    std::size_t count = 0;
    Key_t left_key{};
    Key_t right_key{};
    if (!resolve_analytics_range_(
            cmd, &begin_idx, &count, &left_key, &right_key)) {
      emit_meta_(
          wave,
          out,
          "dataloader.data_analytics.warn reason=empty-effective-range");
      return;
    }

    const std::string signature = build_data_analytics_signature_(
        left_key, right_key, begin_idx, count);
    if (data_analytics_report_ready_ &&
        signature == data_analytics_signature_) {
      return;
    }

    cuwacunu::piaabo::torch_compat::data_source_analytics_accumulator_t acc(
        data_analytics_options_);
    cuwacunu::piaabo::torch_compat::data_symbolic_analytics_accumulator_t
        symbolic_acc(data_symbolic_channel_descriptors_);
    for (std::size_t i = 0; i < count; ++i) {
      const auto sample = dataset_.get(begin_idx + i);
      (void)acc.ingest(sample.features, sample.mask);
      (void)symbolic_acc.ingest(sample.features, sample.mask);
    }
    const auto report = acc.summarize();
    const auto symbolic_report = symbolic_acc.summarize();

    const auto artifact_paths =
        cuwacunu::source::evaluation::
            latest_source_data_analytics_artifact_paths(
                contract_hash_,
                type_name_,
                ctx.source_runtime_cursor);
    const auto& output_file = artifact_paths.numeric_file;
    if (output_file.empty()) return;
    const auto& symbolic_output_file = artifact_paths.symbolic_file;
    if (symbolic_output_file.empty()) return;

    const auto report_identity =
        cuwacunu::source::evaluation::make_source_data_analytics_report_identity(
            ctx.binding_id, ctx.source_runtime_cursor);
    const auto symbolic_report_identity =
        cuwacunu::source::evaluation::
            make_source_data_symbolic_analytics_report_identity(
                ctx.binding_id, ctx.source_runtime_cursor);
    auto report_identity_with_cursor = report_identity;
    auto symbolic_report_identity_with_cursor = symbolic_report_identity;
    std::uint64_t packed_wave_cursor = 0;
    if (cuwacunu::piaabo::latent_lineage_state::pack_runtime_wave_cursor(
            wave.cursor.i,
            wave.cursor.episode,
            wave.cursor.batch,
            &packed_wave_cursor)) {
      report_identity_with_cursor.has_wave_cursor = true;
      report_identity_with_cursor.wave_cursor = packed_wave_cursor;
      symbolic_report_identity_with_cursor = report_identity_with_cursor;
      symbolic_report_identity_with_cursor.semantic_taxon =
          symbolic_report_identity.semantic_taxon;
    }
    std::string write_error;
    if (!cuwacunu::piaabo::torch_compat::write_data_analytics_file(
            report,
            data_analytics_options_,
            output_file,
            instance_name_,
            &write_error,
            report_identity_with_cursor)) {
      emit_meta_(
          wave,
          out,
          std::string("dataloader.data_analytics.warn reason=write-failed detail=") +
              write_error);
      return;
    }
    if (!cuwacunu::piaabo::torch_compat::write_data_symbolic_analytics_file(
            symbolic_report,
            symbolic_output_file,
            instance_name_,
            &write_error,
            symbolic_report_identity_with_cursor)) {
      emit_meta_(
          wave,
          out,
          std::string(
              "dataloader.data_analytics.symbolic.warn reason=write-failed detail=") +
              write_error);
      return;
    }

    latest_data_analytics_file_ = output_file;
    latest_data_analytics_symbolic_file_ = symbolic_output_file;
    data_analytics_signature_ = signature;
    data_analytics_report_ready_ = true;

    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(6)
        << "dataloader.data_analytics"
        << " file=" << output_file.string()
        << " source_entropic_load=" << report.source_entropic_load
        << " valid_sample_count=" << report.valid_sample_count
        << " range=[" << key_to_text_(left_key) << "," << key_to_text_(right_key)
        << "]";
    emit_meta_(wave, out, oss.str());

    std::ostringstream symbolic_meta;
    symbolic_meta.setf(std::ios::fixed);
    symbolic_meta << std::setprecision(6)
                  << "dataloader.data_analytics.symbolic"
                  << " file=" << symbolic_output_file.string()
                  << " eligible_channel_count="
                  << symbolic_report.eligible_channel_count
                  << " lz76_normalized_mean="
                  << symbolic_report.lz76_normalized_mean
                  << " information_density_mean="
                  << symbolic_report.information_density_mean
                  << " compression_ratio_mean="
                  << symbolic_report.compression_ratio_mean
                  << " power_spectrum_entropy_mean="
                  << symbolic_report.power_spectrum_entropy_mean
                  << " hurst_exponent_mean="
                  << symbolic_report.hurst_exponent_mean;
    emit_meta_(wave, out, symbolic_meta.str());

    std::ostringstream pretty;
    pretty << "[tsi.source.dataloader][data_analytics] startup"
           << " source=" << instance_name_ << "\n";
    pretty << format_data_analytics_debug_report_(
        report,
        data_analytics_options_,
        instance_name_,
        output_file,
        /*use_color=*/true);
    pretty << cuwacunu::piaabo::torch_compat::data_symbolic_analytics_to_pretty_text(
        symbolic_report,
        instance_name_,
        symbolic_output_file.string(),
        /*use_color=*/true);
    log_info("%s", pretty.str().c_str());
  }

  [[nodiscard]] static cuwacunu::piaabo::torch_compat::data_analytics_options_t
  data_analytics_options_from_observation_(
      const cuwacunu::camahjucunu::observation_spec_t& observation_instruction) {
    const auto& policy = observation_instruction.data_analytics_policy;
    if (!policy.declared) {
      throw std::runtime_error(
          "observation DATA_ANALYTICS_POLICY is required for tsi.source.dataloader");
    }
    if (policy.max_samples <= 0 || policy.max_features <= 0) {
      throw std::runtime_error(
          "observation DATA_ANALYTICS_POLICY requires MAX_SAMPLES/MAX_FEATURES > 0");
    }
    if (policy.mask_epsilon < 0.0L) {
      throw std::runtime_error(
          "observation DATA_ANALYTICS_POLICY.MASK_EPSILON must be >= 0");
    }
    if (!(policy.standardize_epsilon > 0.0L)) {
      throw std::runtime_error(
          "observation DATA_ANALYTICS_POLICY.STANDARDIZE_EPSILON must be > 0");
    }

    cuwacunu::piaabo::torch_compat::data_analytics_options_t out{};
    out.max_samples = policy.max_samples;
    out.max_features = policy.max_features;
    out.mask_epsilon = static_cast<double>(policy.mask_epsilon);
    out.standardize_epsilon = static_cast<double>(policy.standardize_epsilon);
    return out;
  }

 private:
    TsiId id_{};
    std::string instrument_;
    std::string type_name_;
    std::string instance_name_;
    std::string contract_hash_{};
    std::uint64_t dataloader_workers_{0};
    bool force_rebuild_cache_{true};
    std::uint64_t range_warn_batches_{256};

    torch::Device device_{torch::kCPU};

    // Keep a dataset handle to support exact key-range slicing.
    Dataset_t dataset_;

    // Real loader + iterator state
    Loader_t dl_;
    Iterator_t it_{dl_.end()};
    Iterator_t end_{dl_.end()};
    bool iter_ready_{false};
    cuwacunu::piaabo::torch_compat::data_analytics_options_t data_analytics_options_{};
    std::vector<cuwacunu::piaabo::torch_compat::data_symbolic_channel_descriptor_t>
        data_symbolic_channel_descriptors_{};
    std::string data_analytics_signature_{};
    bool data_analytics_report_ready_{false};
    std::filesystem::path latest_data_analytics_file_{};
    std::filesystem::path latest_data_analytics_symbolic_file_{};

    // Episode cursor state (single-batch stepping + runtime continuation).
    bool episode_active_{false};
    bool continue_requested_{false};
    CommandSpec active_cmd_{};
    std::uint64_t batch_remaining_{0};
    std::uint64_t range_batch_limit_{0};
    std::size_t range_begin_idx_{0};
    std::size_t range_count_{0};
    std::size_t range_cursor_{0};
    std::uint64_t episode_emitted_{0};
    WaveId episode_wave_id_{0};
    std::uint64_t episode_wave_i0_{0};
    std::uint64_t episode_next_wave_i_{0};
    std::uint64_t episode_wave_episode_{0};
    std::uint64_t episode_batch_i0_{0};
    std::uint64_t episode_next_batch_{0};
    bool episode_wave_has_time_span_{false};
    std::int64_t episode_wave_span_begin_ms_{0};
    std::int64_t episode_wave_span_end_ms_{0};

    // Discovered dims
    int64_t B_hint_{0};
    int64_t C_{0}, T_{0}, D_{0};
};

struct source_dataloader_init_record_t : public TsiSourceInitRecord {
  std::size_t instrument_count{0};
  std::size_t input_count{0};
  std::size_t active_input_count{0};
  std::int64_t max_seq_length{0};
  std::int64_t max_future_seq_length{0};
  std::string default_instrument{};
};

using source_dataloader_init_entry_t = TsiSourceInitEntry;
inline constexpr std::string_view kContractSourceDataloaderInitId = "0x0000";
inline constexpr std::string_view kContractSourceDataloaderPath = "<contract-space>";

[[nodiscard]] inline std::filesystem::path source_dataloader_store_root() {
  return std::filesystem::path(kContractSourceDataloaderPath);
}

[[nodiscard]] inline std::filesystem::path source_dataloader_inits_root() {
  return source_dataloader_store_root();
}

[[nodiscard]] inline bool is_valid_source_dataloader_init_id(std::string_view init_id) {
  return init_id == kContractSourceDataloaderInitId;
}

[[nodiscard]] inline std::vector<source_dataloader_init_entry_t> list_source_dataloader_init_entries() {
  source_dataloader_init_entry_t item{};
  item.init_id = std::string(kContractSourceDataloaderInitId);
  item.init_directory = std::filesystem::path(kContractSourceDataloaderPath);
  return {item};
}

[[nodiscard]] inline std::string next_source_dataloader_init_id(const std::filesystem::path& inits_root) {
  (void)inits_root;
  return std::string(kContractSourceDataloaderInitId);
}

inline void fill_source_dataloader_observation_stats(
    const cuwacunu::camahjucunu::observation_spec_t& obs,
    source_dataloader_init_record_t* out) {
  if (!out) return;
  out->instrument_count = obs.source_forms.size();
  out->input_count = obs.channel_forms.size();
  auto obs_stats = obs;
  out->max_seq_length = obs_stats.max_sequence_length();
  out->max_future_seq_length = obs_stats.max_future_sequence_length();
  out->default_instrument.clear();
  out->active_input_count = 0;
  if (!obs.source_forms.empty()) out->default_instrument = obs.source_forms.front().instrument;
  for (const auto& form : obs.channel_forms) {
    if (form.active == "true") ++out->active_input_count;
  }
}

[[nodiscard]] inline source_dataloader_init_record_t build_source_dataloader_init_record(
    const cuwacunu::camahjucunu::observation_spec_t& obs) {
  source_dataloader_init_record_t out{};
  out.ok = true;
  out.init_id = std::string(kContractSourceDataloaderInitId);
  out.store_root = std::filesystem::path(kContractSourceDataloaderPath);
  out.init_directory = std::filesystem::path(kContractSourceDataloaderPath);
  out.metadata_encrypted = false;
  out.metadata_plaintext_fallback = false;
  out.metadata_warning.clear();
  fill_source_dataloader_observation_stats(obs, &out);
  return out;
}

[[nodiscard]] inline source_dataloader_init_record_t persist_source_dataloader_init(
    const cuwacunu::camahjucunu::observation_spec_t& obs) {
  return build_source_dataloader_init_record(obs);
}

[[nodiscard]] inline source_dataloader_init_record_t update_source_dataloader_init(
    const cuwacunu::camahjucunu::observation_spec_t& obs,
    std::string init_id) {
  if (!is_valid_source_dataloader_init_id(init_id)) {
    source_dataloader_init_record_t out{};
    out.error = "invalid dataloader id: " + init_id;
    return out;
  }
  return build_source_dataloader_init_record(obs);
}

[[nodiscard]] inline source_dataloader_init_record_t update_source_dataloader_init_from_config(
    std::string init_id,
    const cuwacunu::iitepi::contract_hash_t& contract_hash) {
  if (contract_hash.empty()) {
    source_dataloader_init_record_t out{};
    out.error = "missing contract hash";
    return out;
  }
  const auto obs =
      cuwacunu::camahjucunu::decode_observation_spec_from_contract(
          contract_hash);
  return update_source_dataloader_init(obs, std::move(init_id));
}

[[nodiscard]] inline bool delete_source_dataloader_init(
    std::string_view init_id,
    std::uintmax_t* removed_count = nullptr,
    std::string* error = nullptr) {
  if (removed_count) *removed_count = 0;
  if (error) error->clear();

  if (!is_valid_source_dataloader_init_id(init_id)) {
    if (error) *error = "invalid dataloader id";
    return false;
  }
  return true;
}

[[nodiscard]] inline source_dataloader_init_record_t invoke_source_dataloader_init_from_config(
    const cuwacunu::iitepi::contract_hash_t& contract_hash) {
  if (contract_hash.empty()) {
    source_dataloader_init_record_t out{};
    out.error = "missing contract hash";
    return out;
  }
  const auto obs =
      cuwacunu::camahjucunu::decode_observation_spec_from_contract(
          contract_hash);
  return persist_source_dataloader_init(obs);
}

} // namespace tsiemene
