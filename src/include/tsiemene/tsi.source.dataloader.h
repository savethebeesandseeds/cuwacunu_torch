// ./include/tsiemene/tsi.source.dataloader.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "tsiemene/tsi.source.h"

// ---- Real dataloader hook -------------------------------------------------
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/data/observation_sample.h"
#include "piaabo/dconfig.h"

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
  static constexpr DirectiveId OUT_FUTURE  = directive_id::Future;
  static constexpr DirectiveId OUT_META    = directive_id::Meta;

  using Dataset_t    = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
  using Sample_t     = cuwacunu::camahjucunu::data::observation_sample_t;
  using Loader_t     = cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Sample_t, Datatype_t, Sampler_t>;
  using Iterator_t   = decltype(std::declval<Loader_t&>().begin());

  TsiSourceDataloader(TsiId id,
                      std::string instrument,
                      cuwacunu::camahjucunu::observation_spec_t observation_instruction,
                      torch::Device device = torch::kCPU,
                      std::size_t batch_size_override = 0)
      : id_(id),
        instrument_(std::move(instrument)),
        type_name_("tsi.source.dataloader"),
        instance_name_(type_name_ + "." + instrument_),
        device_(device),
        dataset_(make_dataset_(instrument_, std::move(observation_instruction))),
        dl_(make_loader_(dataset_, batch_size_override)) {
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

  [[nodiscard]] std::span<const DirectiveSpec> directives() const noexcept override {
    static constexpr DirectiveSpec kDirectives[] = {
      directive(IN_STEP, DirectiveDir::In, KindSpec::String(),
                "episode command (e.g. \"batches=10\" or \"BTCUSDT[01.01.2009,31.12.2009]\"); empty means continue active episode; wave time-span can define range"),
      directive(OUT_PAYLOAD, DirectiveDir::Out, KindSpec::Tensor(),
                "past packed [B,C,T,D+1] (last slot is mask 0/1; B may be <= batch_size on last batch; keys/stats stay in raw sample)"),
      directive(OUT_FUTURE, DirectiveDir::Out, KindSpec::Tensor(),
                "future packed [B,C,Tf,D+1] (last slot is mask 0/1); emitted when future data is available; keys/stats stay in raw sample"),
      directive(OUT_META, DirectiveDir::Out, KindSpec::String(), "runtime trace/meta stream"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 4);
  }
  [[nodiscard]] bool supports_init_artifacts() const noexcept override { return true; }
  [[nodiscard]] std::string_view init_artifact_schema() const noexcept override {
    return "tsi.source.dataloader.init.v1";
  }

  [[nodiscard]] Determinism determinism() const noexcept override {
    if constexpr (std::is_same_v<Sampler_t, torch::data::samplers::SequentialSampler>) {
      return Determinism::Deterministic;
    } else {
      return Determinism::SeededStochastic;
    }
  }

  [[nodiscard]] bool requests_runtime_continuation() const noexcept override {
    return continue_requested_;
  }

  [[nodiscard]] Ingress runtime_continuation_ingress() const override {
    return Ingress{
        .directive = IN_STEP,
        .signal = string_signal(""),
    };
  }

  void step(const Wave& wave, Ingress in, BoardContext&, Emitter& out) override {
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
    if (!pb.past.defined()) {
      std::ostringstream oss;
      oss << "dataloader.episode done emitted=" << episode_emitted_;
      if (!active_cmd_.has_range && batch_remaining_ > 0) {
        oss << " reason=exhausted";
      }
      finish_episode_(out, oss.str());
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
    out.emit_tensor(witem, OUT_PAYLOAD, std::move(pb.past));
    if (pb.future.defined()) out.emit_tensor(witem, OUT_FUTURE, std::move(pb.future));
    ++episode_next_wave_i_;
    ++episode_next_batch_;
    ++episode_emitted_;

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
      finish_episode_(out, oss.str());
    }
  }

  void reset(BoardContext&) override {
    clear_episode_state_();
    it_ = dl_.begin();
    end_ = dl_.end();
    iter_ready_ = true;
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

  static Dataset_t make_dataset_(
      std::string_view instrument,
      cuwacunu::camahjucunu::observation_spec_t observation_instruction) {
    const bool force_rebuild_cache =
        cuwacunu::iitepi::config_space_t::get<bool>(
            "DATA_LOADER", "dataloader_force_rebuild_cache");

    return cuwacunu::camahjucunu::data::create_memory_mapped_concat_dataset<Datatype_t>(
        std::string(instrument), std::move(observation_instruction),
        force_rebuild_cache);
  }

  static Loader_t make_loader_(Dataset_t& dataset, std::size_t batch_size_override) {
    const int configured_workers = cuwacunu::iitepi::config_space_t::get<int>(
        "DATA_LOADER", "dataloader_workers");

    constexpr std::size_t kDefaultBatchSize = 64;
    std::size_t batch_size = kDefaultBatchSize;
    if (batch_size_override > 0) {
      batch_size = batch_size_override;
    }
    const std::size_t workers =
        (configured_workers > 0) ? static_cast<std::size_t>(configured_workers) : 0;

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

  [[nodiscard]] static std::uint64_t range_warn_batches_threshold_() {
    const int configured = cuwacunu::iitepi::config_space_t::get<int>(
        "DATA_LOADER", "dataloader_range_warn_batches", std::optional<int>{256});
    return static_cast<std::uint64_t>(std::max(1, configured));
  }

  struct PackedBatch {
    torch::Tensor past{};
    torch::Tensor future{};
  };

  [[nodiscard]] torch::Tensor pack_feature_mask_(torch::Tensor data, torch::Tensor mask) const {
    if (!data.defined() || !mask.defined()) return torch::Tensor();
    if (mask.dtype() != torch::kFloat32) mask = mask.to(torch::kFloat32);

    if (device_ != torch::kCPU) {
      data = data.to(device_);
      mask = mask.to(device_);
    }

    return torch::cat({data, mask.unsqueeze(-1)}, /*dim=*/3);
  }

  [[nodiscard]] PackedBatch pack_batch_(std::vector<Sample_t>&& sample_batch) const {
    if (sample_batch.empty()) return PackedBatch{};

    Sample_t coll = Sample_t::collate_fn(sample_batch);
    PackedBatch out{};
    out.past = pack_feature_mask_(coll.features, coll.mask);

    if (coll.future_features.defined() && coll.future_mask.defined() &&
        coll.future_features.dim() == 4 && coll.future_mask.dim() == 3) {
      torch::Tensor future = pack_feature_mask_(coll.future_features, coll.future_mask);
      if (future.defined() && future.numel() > 0) out.future = std::move(future);
    }
    return out;
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

  [[nodiscard]] bool start_episode_(const Wave& wave, std::string_view cmd_text, Emitter& out) {
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
    if (!out.past.defined()) return PackedBatch{};
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

  void finish_episode_(Emitter& out, std::string msg) {
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

 private:
    TsiId id_{};
    std::string instrument_;
    std::string type_name_;
    std::string instance_name_;

    torch::Device device_{torch::kCPU};

    // Keep a dataset handle to support exact key-range slicing.
    Dataset_t dataset_;

    // Real loader + iterator state
    Loader_t dl_;
    Iterator_t it_{dl_.end()};
    Iterator_t end_{dl_.end()};
    bool iter_ready_{false};

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
