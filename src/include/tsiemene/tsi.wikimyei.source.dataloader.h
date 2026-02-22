// ./include/tsiemene/tsi.wikimyei.source.dataloader.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <ctime>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "tsiemene/utils/tsi.h"

// ---- Real dataloader hook -------------------------------------------------
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/data/observation_sample.h"

namespace tsiemene {

// NOTE:
//  - Datatype_t is your record struct type (e.g. exchange::kline_t).
//  - Sampler_t controls determinism/order (SequentialSampler vs RandomSampler).
template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::RandomSampler>
class TsiDataloaderInstrument final : public Tsi {
 public:
  static constexpr DirectiveId IN_PAYLOAD  = directive_id::Payload;
  static constexpr DirectiveId OUT_PAYLOAD = directive_id::Payload;
  static constexpr DirectiveId OUT_META    = directive_id::Meta;

  using Dataset_t    = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
  using Sample_t     = cuwacunu::camahjucunu::data::observation_sample_t;
  using Loader_t     = cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Sample_t, Datatype_t, Sampler_t>;
  using Iterator_t   = decltype(std::declval<Loader_t&>().begin());

  TsiDataloaderInstrument(TsiId id,
                          std::string instrument,
                          torch::Device device = torch::kCPU)
      : id_(id),
        instrument_(std::move(instrument)),
        type_name_("tsi.wikimyei.source.dataloader"),
        instance_name_(type_name_ + "." + instrument_),
        device_(device),
        dataset_(make_dataset_(instrument_)),
        dl_(make_loader_(dataset_)) {
    // Introspect shape from the real dataset:
    C_ = dl_.C_;
    T_ = dl_.T_;
    D_ = dl_.D_;

    // Batch size from DataLoaderOptions (used only for metadata / expectations).
    // NOTE: last batch may be smaller.
    B_hint_ = static_cast<int64_t>(dl_.inner().options().batch_size);

    // Start first epoch iterator.
    it_  = dl_.begin();
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
      directive(IN_PAYLOAD, DirectiveDir::In, KindSpec::String(),
                "command for this wave (e.g. \"batches=10\" or \"BTCUSDT[01.01.2009,31.12.2009]\")"),
      directive(OUT_PAYLOAD, DirectiveDir::Out, KindSpec::Tensor(),
                "packed [B,C,T,D+1] (last slot is mask 0/1; B may be <= batch_size on last batch)"),
      directive(OUT_META, DirectiveDir::Out, KindSpec::String(), "runtime trace/meta stream"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 3);
  }

  [[nodiscard]] Determinism determinism() const noexcept override {
    if constexpr (std::is_same_v<Sampler_t, torch::data::samplers::SequentialSampler>) {
      return Determinism::Deterministic;
    } else {
      return Determinism::SeededStochastic;
    }
  }

  void step(const Wave& wave, Ingress in, TsiContext&, Emitter& out) override {
    if (in.directive != IN_PAYLOAD) return;
    if (in.signal.kind != PayloadKind::String) return;

    const CommandSpec cmd = parse_command_(in.signal.text);
    emit_command_meta_(wave, cmd, out);

    // Date-range mode: emit collated batches from [start,end].
    if (cmd.has_range) {
      emit_range_batches_(wave, cmd, out);
      return;
    }

    // Plain batch-count mode.
    if (cmd.batches == 0) {
      emit_meta_(wave, out, "dataloader.batch-mode noop requested=0 wave_i=<none>");
      return;
    }

    std::uint64_t emitted = 0;
    for (std::uint64_t k = 0; k < cmd.batches; ++k) {
      torch::Tensor packed = next_packed_batch_();
      if (!packed.defined()) {
        // No data available (empty dataset or hard exhaustion).
        // For now, stop emitting further batches.
        std::ostringstream oss;
        oss << "dataloader.batch-mode exhausted emitted=" << emitted
            << " requested=" << cmd.batches;
        emit_meta_(wave, out, oss.str());
        break;
      }

      Wave witem{.id = wave.id, .i = wave.i + k};
      out.emit_tensor(witem, OUT_PAYLOAD, std::move(packed));
      ++emitted;
    }

    std::ostringstream oss;
    oss << "dataloader.batch-mode done emitted=" << emitted
        << " requested=" << cmd.batches;
    if (emitted > 0) {
      oss << " wave_i=[" << wave.i << "," << (wave.i + emitted - 1) << "]";
    } else {
      oss << " wave_i=<none>";
    }
    emit_meta_(wave, out, oss.str());
  }

 private:
  using Key_t = typename Datatype_t::key_type_t;

  struct CommandSpec {
    std::uint64_t batches{0};
    bool has_range{false};
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
    if (*d < 1 || *d > 31 || *m < 1 || *m > 12 || *y < 1970) return std::nullopt;

    std::tm tm{};
    tm.tm_mday = *d;
    tm.tm_mon = *m - 1;
    tm.tm_year = *y - 1900;
    tm.tm_hour = end_of_day ? 23 : 0;
    tm.tm_min = end_of_day ? 59 : 0;
    tm.tm_sec = end_of_day ? 59 : 0;

    const std::time_t t = std::mktime(&tm);
    if (t < 0) return std::nullopt;

    return static_cast<std::int64_t>(t) * 1000 + (end_of_day ? 999 : 0);
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

  static std::uint64_t parse_batches_compat_(std::string_view s) noexcept {
    // Compatibility convenience: bare first digit run (only for non-range commands).
    std::size_t pos = 0;
    while (pos < s.size() && (s[pos] < '0' || s[pos] > '9')) ++pos;
    if (pos == s.size()) return 0;

    std::size_t end = pos;
    while (end < s.size() && s[end] >= '0' && s[end] <= '9') ++end;
    if (auto v = parse_u64_(s.substr(pos, end - pos)); v.has_value()) return *v;
    return 0;
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

  [[nodiscard]] CommandSpec parse_command_(std::string_view s) const noexcept {
    CommandSpec cmd{};
    cmd.has_range = parse_range_keys_(s, cmd.key_left, cmd.key_right);
    if (cmd.has_range) {
      // In range mode, do not infer batches from date digits.
      cmd.batches = parse_batches_explicit_(s).value_or(0);
    } else {
      const auto explicit_batches = parse_batches_explicit_(s);
      cmd.batches = explicit_batches.value_or(parse_batches_compat_(s));
    }
    return cmd;
  }

  static Dataset_t make_dataset_(std::string_view instrument) {
    const bool force_bin = cuwacunu::piaabo::dconfig::config_space_t::get<bool>(
        "DATA_LOADER", "dataloader_force_binarization");

    std::string inst{instrument};
    auto obs = cuwacunu::camahjucunu::BNF::observationPipeline().decode(
        cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction());

    return cuwacunu::camahjucunu::data::create_memory_mapped_concat_dataset<Datatype_t>(
        inst, std::move(obs), force_bin);
  }

  static Loader_t make_loader_(Dataset_t& dataset) {
    const int batch_size = cuwacunu::piaabo::dconfig::config_space_t::get<int>(
        "DATA_LOADER", "dataloader_batch_size");
    const int workers = cuwacunu::piaabo::dconfig::config_space_t::get<int>(
        "DATA_LOADER", "dataloader_workers");

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

  void emit_range_batches_(const Wave& wave, const CommandSpec& cmd, Emitter& out) {
    std::vector<Sample_t> samples = dataset_.range_samples_by_keys(cmd.key_left, cmd.key_right);
    const std::size_t batch_size = (B_hint_ > 0) ? static_cast<std::size_t>(B_hint_) : std::size_t(64);
    const std::uint64_t max_batches = (cmd.batches > 0)
        ? cmd.batches
        : std::numeric_limits<std::uint64_t>::max();

    {
      std::ostringstream oss;
      oss << "dataloader.range-mode setup samples=" << samples.size()
          << " batch_size=" << batch_size;
      if (cmd.batches > 0) {
        oss << " max_batches=" << max_batches;
      } else {
        oss << " max_batches=unbounded";
      }
      emit_meta_(wave, out, oss.str());
    }

    if (samples.empty()) {
      emit_meta_(wave, out, "dataloader.range-mode noop reason=no-samples wave_i=<none>");
      return;
    }

    std::uint64_t emitted = 0;
    for (std::size_t i = 0; i < samples.size() && emitted < max_batches; i += batch_size) {
      const std::size_t e = std::min(i + batch_size, samples.size());
      std::vector<Sample_t> chunk;
      chunk.reserve(e - i);
      for (std::size_t j = i; j < e; ++j) chunk.push_back(std::move(samples[j]));

      torch::Tensor packed = pack_batch_(std::move(chunk));
      if (!packed.defined()) continue;

      Wave witem{.id = wave.id, .i = wave.i + emitted};
      out.emit_tensor(witem, OUT_PAYLOAD, std::move(packed));
      ++emitted;
    }

    std::ostringstream oss;
    oss << "dataloader.range-mode done emitted=" << emitted
        << " sample_count=" << samples.size();
    if (emitted > 0) {
      oss << " wave_i=[" << wave.i << "," << (wave.i + emitted - 1) << "]";
    } else {
      oss << " wave_i=<none>";
    }
    emit_meta_(wave, out, oss.str());
  }

  torch::Tensor pack_batch_(std::vector<Sample_t>&& sample_batch) const {
    if (sample_batch.empty()) return torch::Tensor();

    // Collate to [B,C,T,D] and [B,C,T]
    Sample_t coll = Sample_t::collate_fn(sample_batch);

    torch::Tensor data = coll.features;                // float32 [B,C,T,D]
    torch::Tensor mask = coll.mask.to(torch::kFloat32); // 0/1    [B,C,T]

    if (!data.defined() || !mask.defined()) return torch::Tensor();

    if (device_ != torch::kCPU) {
      data = data.to(device_);
      mask = mask.to(device_);
    }

    // packed: [B,C,T,D+1] where last slot is mask (0/1 float)
    return torch::cat({data, mask.unsqueeze(-1)}, /*dim=*/3);
  }

  torch::Tensor next_packed_batch_() {
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
      if (it_ == end_) return torch::Tensor(); // empty loader
    }

    // Move the produced batch out of the iterator to avoid copying.
    auto& batch_ref = *it_;
    std::vector<Sample_t> batch = std::move(batch_ref);
    ++it_;

    return pack_batch_(std::move(batch));
  }

  void emit_command_meta_(const Wave& wave, const CommandSpec& cmd, Emitter& out) const {
    std::ostringstream oss;
    if (cmd.has_range) {
      oss << "dataloader.command mode=range";
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
      oss << "dataloader.command mode=batch-count requested=" << cmd.batches;
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

    // Discovered dims
    int64_t B_hint_{0};
    int64_t C_{0}, T_{0}, D_{0};
};

} // namespace tsiemene
