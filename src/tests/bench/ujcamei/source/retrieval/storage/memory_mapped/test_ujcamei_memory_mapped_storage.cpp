// SPDX-License-Identifier: MIT
#include "ujcamei/source/registry/types/data.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "ujcamei/source/retrieval/dataloader/edge_dataloader.h"
#include "ujcamei/source/retrieval/dataloader/edge_sample.h"
#include "ujcamei/source/retrieval/storage/memory_mapped/cache_freshness.h"
#include "ujcamei/source/retrieval/storage/memory_mapped/memory_mapped_datafile.h"
#include "ujcamei/source/retrieval/storage/memory_mapped/memory_mapped_dataloader.h"
#include "ujcamei/source/retrieval/storage/memory_mapped/memory_mapped_dataset.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <unistd.h>

namespace dl = cuwacunu::ujcamei::source::retrieval::dataloader;
namespace contract = cuwacunu::ujcamei::source::contract;
namespace graph = cuwacunu::kikijyeba::topology::graph;
namespace mm = cuwacunu::ujcamei::source::retrieval::storage::memory_mapped;
namespace source = cuwacunu::ujcamei::source;
namespace types = cuwacunu::ujcamei::source::registry::types;

namespace {

using Kline = types::kline_t;
using KlineCache = types::kline_cache_t;

void check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[MemoryMappedStorage test] " << message << "\n";
    std::exit(1);
  }
}

void close(double actual, double expected, double tol, const std::string &msg) {
  if (std::fabs(actual - expected) > tol) {
    std::cerr << "[MemoryMappedStorage test] " << msg << " actual=" << actual
              << " expected=" << expected << "\n";
    std::exit(1);
  }
}

double scalar(const torch::Tensor &t) {
  return t.detach().to(torch::kCPU).to(torch::kFloat64).item<double>();
}

bool bscalar(const torch::Tensor &t) {
  return t.detach().to(torch::kCPU).to(torch::kBool).item<bool>();
}

template <typename Fn> void expect_throw(Fn &&fn, const std::string &msg) {
  try {
    fn();
  } catch (const c10::Error &) {
    return;
  } catch (const std::exception &) {
    return;
  }
  std::cerr << "[MemoryMappedStorage test] expected throw: " << msg << "\n";
  std::exit(1);
}

template <typename Fn>
void expect_memory_mapped_error(Fn &&fn, const std::string &msg,
                                const std::string &needle = {}) {
  try {
    fn();
  } catch (const mm::memory_mapped_error_t &ex) {
    if (!needle.empty() &&
        std::string(ex.what()).find(needle) == std::string::npos) {
      std::cerr << "[MemoryMappedStorage test] expected error text '" << needle
                << "' in: " << ex.what() << "\n";
      std::exit(1);
    }
    return;
  } catch (const std::exception &ex) {
    std::cerr << "[MemoryMappedStorage test] expected memory_mapped_error_t "
              << "for " << msg << ", got: " << ex.what() << "\n";
    std::exit(1);
  }
  std::cerr << "[MemoryMappedStorage test] expected memory_mapped_error_t: "
            << msg << "\n";
  std::exit(1);
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  static int counter = 0;
  auto dir = std::filesystem::temp_directory_path() /
             ("cuwacunu_ujcamei_mm_" + label + "_" +
              std::to_string(static_cast<long long>(::getpid())) + "_" +
              std::to_string(counter++));
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

Kline make_kline(types::ms_t close_time, double price, double volume = 10.0,
                 int32_t trades = 3) {
  Kline out{};
  out.open_time = close_time - 1;
  out.open_price = price;
  out.high_price = price + 1.0;
  out.low_price = price - 1.0;
  out.close_price = price;
  out.volume = volume;
  out.close_time = close_time;
  out.quote_asset_volume = volume * price;
  out.number_of_trades = trades;
  out.taker_buy_base_volume = volume * 0.4;
  out.taker_buy_quote_volume = volume * price * 0.4;
  return out;
}

KlineCache make_cache(types::ms_t close_time, double price, bool valid = true) {
  if (!valid) {
    return KlineCache::null_instance(close_time);
  }
  return KlineCache::from_raw(make_kline(close_time, price));
}

template <typename KlineRecord>
double kline_field_value(const KlineRecord &record,
                         types::kline_feature_e feature) {
  switch (feature) {
  case types::kline_feature_e::open:
    return record.open_price;
  case types::kline_feature_e::high:
    return record.high_price;
  case types::kline_feature_e::low:
    return record.low_price;
  case types::kline_feature_e::close:
    return record.close_price;
  case types::kline_feature_e::volume:
    return record.volume;
  case types::kline_feature_e::quote_volume:
    return record.quote_asset_volume;
  case types::kline_feature_e::trades:
    return static_cast<double>(record.number_of_trades);
  case types::kline_feature_e::taker_buy_base:
    return record.taker_buy_base_volume;
  case types::kline_feature_e::taker_buy_quote:
    return record.taker_buy_quote_volume;
  }
  return 0.0;
}

void write_cache_bin(const std::filesystem::path &path,
                     const std::vector<KlineCache> &records) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  check(out.is_open(), "failed to open binary output");
  out.write(reinterpret_cast<const char *>(records.data()),
            static_cast<std::streamsize>(records.size() * sizeof(KlineCache)));
  check(static_cast<bool>(out), "failed to write binary records");
}

void write_bytes(const std::filesystem::path &path,
                 const std::vector<unsigned char> &bytes) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  check(out.is_open(), "failed to open byte output");
  out.write(reinterpret_cast<const char *>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  check(static_cast<bool>(out), "failed to write byte records");
}

std::vector<unsigned char> read_bytes(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary);
  check(in.is_open(), "failed to open byte input");
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(in),
                                    std::istreambuf_iterator<char>());
}

void test_frozen_cache_freshness_guard() {
  const auto dir = make_tmp_dir("cache_freshness");
  const auto csv = dir / "series.csv";
  const auto raw = dir / "series.cache.bin";
  const auto normalized = dir / "series.cache.norm.log_returns.bin";
  write_bytes(csv, {1, 2, 3});
  write_bytes(raw, {4, 5, 6});
  write_bytes(normalized, {7, 8, 9});

  const auto base_time =
      std::filesystem::file_time_type::clock::now() - std::chrono::hours(24);
  const auto requested_raw_time = base_time + std::chrono::seconds(2);
  std::filesystem::last_write_time(csv, base_time);
  std::filesystem::last_write_time(raw, requested_raw_time);
  std::filesystem::last_write_time(normalized, requested_raw_time);
  const auto observed_csv_time = std::filesystem::last_write_time(csv);
  const auto observed_raw_time = std::filesystem::last_write_time(raw);
  const auto observed_normalized_time =
      std::filesystem::last_write_time(normalized);
  check(mm::cache_time_is_strictly_newer(observed_raw_time, observed_csv_time),
        "fixture raw cache is strictly newer than CSV");
  check(observed_normalized_time == observed_raw_time,
        "fixture raw and normalized mtimes are equal");
  for (const auto &path : {csv, raw, normalized}) {
    std::filesystem::permissions(path,
                                 std::filesystem::perms::owner_write |
                                     std::filesystem::perms::group_write |
                                     std::filesystem::perms::others_write,
                                 std::filesystem::perm_options::remove);
  }

  const auto csv_size = std::filesystem::file_size(csv);
  const auto raw_size = std::filesystem::file_size(raw);
  const auto normalized_size = std::filesystem::file_size(normalized);
  const auto csv_bytes = read_bytes(csv);
  const auto raw_bytes = read_bytes(raw);
  const auto normalized_bytes = read_bytes(normalized);
  const auto csv_permissions = std::filesystem::status(csv).permissions();
  const auto raw_permissions = std::filesystem::status(raw).permissions();
  const auto normalized_permissions =
      std::filesystem::status(normalized).permissions();

  const auto equal_time =
      mm::inspect_cache_chain_freshness(csv, raw, normalized);
  check(equal_time.status ==
            mm::cache_chain_freshness_status_t::normalized_not_strictly_newer,
        "equal raw/normalized mtimes rejected");
  check(mm::cache_file_is_strictly_newer(raw, csv),
        "loader freshness predicate accepts raw newer than CSV");
  check(!mm::cache_file_is_strictly_newer(normalized, raw),
        "loader freshness predicate rejects equal normalized/raw mtimes");
  check(std::filesystem::file_size(csv) == csv_size &&
            std::filesystem::file_size(raw) == raw_size &&
            std::filesystem::file_size(normalized) == normalized_size,
        "cache freshness inspection does not change file sizes");
  check(read_bytes(csv) == csv_bytes && read_bytes(raw) == raw_bytes &&
            read_bytes(normalized) == normalized_bytes,
        "cache freshness inspection does not change file bytes");
  check(std::filesystem::last_write_time(csv) == observed_csv_time &&
            std::filesystem::last_write_time(raw) == observed_raw_time &&
            std::filesystem::last_write_time(normalized) ==
                observed_normalized_time,
        "cache freshness inspection does not change mtimes");
  check(std::filesystem::status(csv).permissions() == csv_permissions &&
            std::filesystem::status(raw).permissions() == raw_permissions &&
            std::filesystem::status(normalized).permissions() ==
                normalized_permissions,
        "cache freshness inspection does not change permissions");

  std::filesystem::last_write_time(normalized,
                                   observed_raw_time + std::chrono::seconds(2));
  const auto ready_normalized_time =
      std::filesystem::last_write_time(normalized);
  const auto ready = mm::inspect_cache_chain_freshness(csv, raw, normalized);
  check(ready.ready(), "strict csv < raw < normalized chain accepted");
  check(mm::cache_file_is_strictly_newer(normalized, raw),
        "loader freshness predicate accepts normalized newer than raw");

  const auto selected = mm::sanitize_csv_into_binary_file<Kline>(
      csv.string(), "log_returns", false, 2);
  check(selected == normalized.string(),
        "no-force sanitizer selects the fresh normalized cache");
  check(read_bytes(csv) == csv_bytes && read_bytes(raw) == raw_bytes &&
            read_bytes(normalized) == normalized_bytes,
        "no-force sanitizer does not change fresh read-only cache bytes");
  check(std::filesystem::last_write_time(csv) == observed_csv_time &&
            std::filesystem::last_write_time(raw) == observed_raw_time &&
            std::filesystem::last_write_time(normalized) ==
                ready_normalized_time,
        "no-force sanitizer does not change fresh cache mtimes");
  check(std::filesystem::status(csv).permissions() == csv_permissions &&
            std::filesystem::status(raw).permissions() == raw_permissions &&
            std::filesystem::status(normalized).permissions() ==
                normalized_permissions,
        "no-force sanitizer does not change fresh cache permissions");

  std::filesystem::remove(normalized);
  const auto missing = mm::inspect_cache_chain_freshness(csv, raw, normalized);
  check(missing.status ==
            mm::cache_chain_freshness_status_t::normalized_missing,
        "missing normalized cache rejected");

  std::filesystem::create_symlink(raw, normalized);
  const auto symlink = mm::inspect_cache_chain_freshness(csv, raw, normalized);
  check(symlink.status ==
            mm::cache_chain_freshness_status_t::normalized_symlink,
        "symlinked normalized cache rejected");
  std::filesystem::remove_all(dir);
}

void write_kline_csv(const std::filesystem::path &path,
                     const std::vector<Kline> &records) {
  std::ofstream out(path, std::ios::trunc);
  check(out.is_open(), "failed to open csv output");
  for (const auto &record : records) {
    record.to_csv(out, ',');
    out << '\n';
  }
}

source::registry::instrument_signature_t btc_usdt_signature() {
  return source::registry::instrument_signature_t{
      .symbol = "BTCUSDT",
      .record_type = "kline",
      .market_type = "spot",
      .venue = "binance",
      .base_asset = "BTC",
      .quote_asset = "USDT",
  };
}

contract::source_spec_t make_source_spec(const std::filesystem::path &csv,
                                         std::size_t input_length,
                                         std::size_t future_length) {
  contract::source_spec_t spec{};
  spec.source_forms.push_back(contract::source_form_t{
      .instrument = "BTCUSDT",
      .interval = types::interval_type_e::interval_1m,
      .record_type = "kline",
      .market_type = "spot",
      .venue = "binance",
      .base_asset = "BTC",
      .quote_asset = "USDT",
      .source = csv.string(),
  });
  spec.channel_forms.push_back(contract::channel_form_t{
      .interval = types::interval_type_e::interval_1m,
      .active = "true",
      .record_type = "kline",
      .input_length = std::to_string(input_length),
      .future_length = std::to_string(future_length),
      .channel_weight = "1.0",
      .normalization_policy = "none",
  });
  return spec;
}

std::vector<KlineCache> read_cache_bin(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary);
  check(in.is_open(), "failed to open binary input");
  in.seekg(0, std::ios::end);
  const std::streamoff bytes = in.tellg();
  check(bytes >= 0, "failed to measure binary input");
  check((static_cast<std::size_t>(bytes) % sizeof(KlineCache)) == 0,
        "binary size is not record-aligned");
  in.seekg(0, std::ios::beg);
  std::vector<KlineCache> records(static_cast<std::size_t>(bytes) /
                                  sizeof(KlineCache));
  in.read(reinterpret_cast<char *>(records.data()),
          static_cast<std::streamsize>(records.size() * sizeof(KlineCache)));
  check(static_cast<bool>(in) || in.eof(), "failed to read binary records");
  return records;
}

void test_kline_feature_registry_contract() {
  check(types::kline_feature_registry_is_complete(),
        "kline registry covers every coordinate");
  check(types::kKlineFeatureRegistry.size() ==
            static_cast<std::size_t>(types::kKlineFeatureWidth),
        "kline registry width");

  std::vector<bool> seen(static_cast<std::size_t>(types::kKlineFeatureWidth),
                         false);
  std::size_t price_count = 0;
  std::size_t activity_count = 0;
  std::size_t price_coord_index = 0;
  std::size_t activity_coord_index = 0;

  for (const auto &entry : types::kKlineFeatureRegistry) {
    const auto coord = static_cast<std::size_t>(entry.coord);
    check(entry.coord >= 0 && entry.coord < types::kKlineFeatureWidth,
          "kline registry coordinate range");
    check(!seen[coord], "kline registry coordinate uniqueness");
    seen[coord] = true;
    check(entry.coord == types::kline_feature_index(entry.feature),
          "kline feature enum/index agreement");
    check(types::kline_feature_name(entry.feature) == entry.name,
          "kline feature name lookup");

    const auto *by_feature =
        types::find_kline_feature_descriptor(entry.feature);
    check(by_feature != nullptr && by_feature->coord == entry.coord,
          "kline descriptor lookup by feature");
    const auto *by_coord = types::find_kline_feature_descriptor_at(entry.coord);
    check(by_coord != nullptr && by_coord->feature == entry.feature,
          "kline descriptor lookup by coord");

    switch (entry.group) {
    case types::kline_feature_group_e::price:
      check(entry.normalization_rule ==
                types::kline_normalization_rule_e::log_return_to_previous_close,
            "price feature normalization rule");
      check(types::kKlinePriceFeatureCoords[price_coord_index++] == entry.coord,
            "price coordinate selector derives from registry order");
      ++price_count;
      break;
    case types::kline_feature_group_e::activity:
      check(entry.normalization_rule ==
                types::kline_normalization_rule_e::log1p,
            "activity feature normalization rule");
      check(types::kKlineActivityFeatureCoords[activity_coord_index++] ==
                entry.coord,
            "activity coordinate selector derives from registry order");
      ++activity_count;
      break;
    }
  }

  check(price_count == types::kKlinePriceFeatureCount,
        "price feature count derives from registry");
  check(activity_count == types::kKlineActivityFeatureCount,
        "activity feature count derives from registry");
}

void test_kline_csv_canonicalizes_microsecond_timestamps() {
  const std::string row = "1740960000000000,94269.99000000,94416.46000000,"
                          "80000.00000000,80734.37000000,284471.65101000,"
                          "1741564799999999,24921880717.48637600,53091837,"
                          "138994.28437000,12185490816.00301640,0";
  const auto parsed = Kline::from_csv(row, ',', 1);
  check(parsed.open_time == 1740960000000,
        "kline open_time microseconds canonicalized to milliseconds");
  check(parsed.close_time == 1741564799999,
        "kline close_time microseconds canonicalized to milliseconds");
}

void test_kline_tensor_and_normalization_follow_registry() {
  Kline previous{};
  previous.open_time = 999;
  previous.open_price = 91.0;
  previous.high_price = 109.0;
  previous.low_price = 89.0;
  previous.close_price = 100.0;
  previous.volume = 7.0;
  previous.close_time = 1000;
  previous.quote_asset_volume = 701.0;
  previous.number_of_trades = 5;
  previous.taker_buy_base_volume = 3.0;
  previous.taker_buy_quote_volume = 300.0;

  Kline current{};
  current.open_time = 1000;
  current.open_price = 101.0;
  current.high_price = 107.0;
  current.low_price = 97.0;
  current.close_price = 103.0;
  current.volume = 11.0;
  current.close_time = 1001;
  current.quote_asset_volume = 13.0;
  current.number_of_trades = 17;
  current.taker_buy_base_volume = 19.0;
  current.taker_buy_quote_volume = 23.0;

  const auto current_cache = KlineCache::from_raw(current);
  const auto previous_cache = KlineCache::from_raw(previous);
  const auto normalized =
      KlineCache::normalize_log_returns(current_cache, &previous_cache);

  const auto raw_features = current.tensor_features();
  const auto cache_features = current_cache.tensor_features();
  const auto normalized_features = normalized.tensor_features();
  check(raw_features.size() ==
            static_cast<std::size_t>(types::kKlineFeatureWidth),
        "raw kline feature width");
  check(cache_features.size() ==
            static_cast<std::size_t>(types::kKlineFeatureWidth),
        "cache kline feature width");
  check(normalized_features.size() ==
            static_cast<std::size_t>(types::kKlineFeatureWidth),
        "normalized kline feature width");

  for (const auto &entry : types::kKlineFeatureRegistry) {
    const auto coord = static_cast<std::size_t>(entry.coord);
    const std::string feature_name(entry.name);
    const double raw_value = kline_field_value(current, entry.feature);
    close(raw_features[coord], raw_value, 1e-12,
          "raw tensor feature map " + feature_name);
    close(cache_features[coord], raw_value, 1e-12,
          "cache tensor feature map " + feature_name);

    double expected_normalized = 0.0;
    switch (entry.normalization_rule) {
    case types::kline_normalization_rule_e::log_return_to_previous_close:
      expected_normalized = std::log(raw_value / previous.close_price);
      break;
    case types::kline_normalization_rule_e::log1p:
      expected_normalized = std::log1p(raw_value);
      break;
    }
    close(normalized_features[coord], expected_normalized, 1e-12,
          "normalized tensor feature map " + feature_name);
  }
}

void test_single_edge_dataset_anchor_fields() {
  const auto dir = make_tmp_dir("single");
  const auto bin = dir / "single.cache.bin";
  write_cache_bin(bin, {
                           make_cache(1000, 10.0),
                           make_cache(1001, 11.0),
                           make_cache(1002, 12.0),
                           make_cache(1003, 13.0),
                           make_cache(1004, 14.0),
                           make_cache(1005, 0.0, false),
                           make_cache(1006, 16.0),
                       });

  mm::MemoryMappedDataset<Kline> dataset(bin.string(), 3, 2);
  check(dataset.size().value() == 3, "single dataset anchor count");
  check(dataset.key_value_step_ == 1, "single dataset step");

  auto sample = dataset.get_edge_sample_at_anchor_key(1003, 3, 2);
  check(sample.features.sizes() == torch::IntArrayRef({3, 9}),
        "single sample features shape");
  check(sample.future_features.sizes() == torch::IntArrayRef({2, 9}),
        "single sample future shape");
  close(scalar(sample.past_keys.index({0})), 1001.0, 1e-6, "past first key");
  close(scalar(sample.past_keys.index({2})), 1003.0, 1e-6, "past terminal key");
  close(scalar(sample.future_keys.index({0})), 1004.0, 1e-6,
        "future first key");
  close(scalar(sample.future_keys.index({1})), 1005.0, 1e-6,
        "future second key");
  check(bscalar(sample.mask.index({0})), "past valid row");
  check(!bscalar(sample.future_mask.index({1})),
        "invalid future row mask is false");

  const auto before = sample.features.clone();
  sample.feature_mean = torch::zeros({9}, torch::kFloat32);
  sample.feature_std = torch::ones({9}, torch::kFloat32);
  sample.normalize_inplace();
  check(sample.normalized, "sample normalized flag set");
  close(scalar((sample.features - before).abs().sum()), 0.0, 1e-8,
        "zero-mean unit-std normalization preserves values");
  sample.denormalize_inplace();
  check(!sample.normalized, "sample denormalized flag cleared");

  const auto range = dataset.range_edge_samples_by_anchor_keys(1002, 1004);
  check(range.size() == 3, "single dataset anchor range count");

  std::string probe_error{};
  check(!dataset.can_get_edge_sample_at_anchor_key(1000, 3, 2, &probe_error),
        "strict probe rejects insufficient past");
  check(probe_error.find("input field exceeds dataset start") !=
            std::string::npos,
        "strict probe reports insufficient past");
  probe_error.clear();
  check(!dataset.can_get_edge_sample_at_anchor_key(1005, 3, 2, &probe_error),
        "strict probe rejects insufficient future");
  check(probe_error.find("future field exceeds dataset size") !=
            std::string::npos,
        "strict probe reports insufficient future");
}

void test_storage_errors_are_catchable() {
  const auto dir = make_tmp_dir("errors");
  const auto missing = dir / "missing.cache.bin";
  expect_memory_mapped_error(
      [&] { mm::MemoryMappedDataset<Kline> dataset(missing.string(), 1, 0); },
      "missing binary throws", "could not open binary file");

  const auto malformed = dir / "malformed.cache.bin";
  write_bytes(malformed, {0x01, 0x02, 0x03});
  expect_memory_mapped_error(
      [&] { mm::MemoryMappedDataset<Kline> dataset(malformed.string(), 1, 0); },
      "malformed binary throws", "multiple of record size");
}

void test_zero_future_horizon_and_validation() {
  const auto dir = make_tmp_dir("zerofuture");
  const auto bin = dir / "single.cache.bin";
  write_cache_bin(bin, {make_cache(2000, 20.0)});

  mm::MemoryMappedDataset<Kline> dataset(bin.string(), 1, 0);
  check(dataset.size().value() == 1, "zero future dataset has one anchor");
  auto sample = dataset.get(0);
  check(sample.future_features.sizes() == torch::IntArrayRef({0, 9}),
        "zero future features shape");
  check(sample.future_mask.sizes() == torch::IntArrayRef({0}),
        "zero future mask shape");
  check(!sample.has_future_values(), "zero future sample has no future values");

  auto by_key = dataset.get_edge_sample_at_anchor_key(2000, 1, 0);
  check(by_key.features.sizes() == torch::IntArrayRef({1, 9}),
        "zero future by-key lookup succeeds");

  expect_throw([&] { mm::MemoryMappedDataset<Kline> bad(bin.string(), 0, 0); },
               "input_length=0 rejected");
}

void test_edge_dataset_channel_padding_and_ranges() {
  const auto dir = make_tmp_dir("edge");
  const auto csv0 = dir / "edge0.csv";
  const auto csv1 = dir / "edge1.csv";
  std::vector<Kline> rows0;
  std::vector<Kline> rows1;
  for (int i = 0; i < 6; ++i) {
    rows0.push_back(make_kline(3000 + i, 30.0 + i));
    rows1.push_back(make_kline(3000 + i, 40.0 + i));
  }
  write_kline_csv(csv0, rows0);
  write_kline_csv(csv1, rows1);

  mm::MemoryMappedEdgeDataset<Kline> dataset;
  dataset.add_dataset(csv0.string(), 3, 2, "none", true);
  dataset.add_dataset(csv1.string(), 2, 1, "none", true);

  check(dataset.max_input_length_ == 3, "max input length");
  check(dataset.max_future_length_ == 2, "max future length");
  check(dataset.size().value() == 2, "edge dataset anchor count");
  check(dataset.leftmost_key_value_ == 3002, "edge dataset left key");
  check(dataset.rightmost_key_value_ == 3003, "edge dataset right key");

  auto sample = dataset.get_by_anchor_key(3002);
  check(sample.features.sizes() == torch::IntArrayRef({2, 3, 9}),
        "edge sample features shape");
  check(sample.future_features.sizes() == torch::IntArrayRef({2, 2, 9}),
        "edge sample future shape");
  close(scalar(sample.past_keys.index({0, 0})), 3000.0, 1e-6,
        "channel 0 past first key");
  close(scalar(sample.past_keys.index({1, 0})), 0.0, 1e-6,
        "channel 1 left padding key");
  check(!bscalar(sample.mask.index({1, 0})), "channel 1 left padding mask");
  close(scalar(sample.past_keys.index({1, 2})), 3002.0, 1e-6,
        "channel 1 terminal key");
  close(scalar(sample.future_keys.index({1, 0})), 3003.0, 1e-6,
        "channel 1 first future key");
  check(!bscalar(sample.future_mask.index({1, 1})),
        "channel 1 right future padding mask");

  std::size_t begin = 0;
  std::size_t count = 0;
  check(dataset.compute_anchor_index_range_by_keys(3002, 3003, &begin, &count),
        "edge range non-empty");
  check(begin == 0 && count == 2, "edge range begin/count");
  const auto range = dataset.range_edge_samples_by_anchor_keys(3002, 3003);
  check(range.size() == 2, "edge range sample count");

  auto sampler = dataset.SequentialSampler();
  auto loader_options = dataset.SequentialSampler_options(2, 0);
  mm::MemoryMappedDataLoader<mm::MemoryMappedEdgeDataset<Kline>,
                             dl::edge_sample_t, Kline>
      loader(dataset, sampler, loader_options);
  check(loader.C_ == 2, "loader channel count");
  check(loader.Hx_ == 3, "loader input horizon");
  check(loader.D_ == 9, "loader feature width");
  std::size_t loader_batches = 0;
  for (auto &batch : loader) {
    (void)batch;
    ++loader_batches;
  }
  check(loader_batches == 1, "loader sequential iteration batch count");

  const auto csv_single = dir / "one_anchor.csv";
  write_kline_csv(csv_single, {
                                  make_kline(4000, 40.0),
                                  make_kline(4001, 41.0),
                                  make_kline(4002, 42.0),
                              });
  mm::MemoryMappedEdgeDataset<Kline> one_anchor;
  one_anchor.add_dataset(csv_single.string(), 2, 1, "none", true);
  check(one_anchor.size().value() == 1, "one-anchor edge dataset size");
  std::string strict_error{};
  check(!one_anchor.can_get_by_anchor_key_strict(4000, &strict_error),
        "edge strict probe rejects insufficient past anchor");
  check(strict_error.find("outside edge dataset domain") != std::string::npos,
        "edge strict probe reports domain reason");
  close(scalar(one_anchor.get(0).past_keys.index({0, 1})), 4001.0, 1e-6,
        "one-anchor terminal key");
}

void test_csv_sanitization_and_normalized_masks() {
  const auto dir = make_tmp_dir("sanitize");
  const auto csv = dir / "duplicate_gap.csv";
  write_kline_csv(csv, {
                           make_kline(5000, 10.0),
                           make_kline(5001, 11.0),
                           make_kline(5001, 12.0),
                           make_kline(5003, 13.0),
                       });
  const auto raw_bin =
      mm::sanitize_csv_into_binary_file<Kline>(csv.string(), "none", true, 2);
  const auto raw = read_cache_bin(raw_bin);
  check(raw.size() == 4, "duplicate/gap raw cache size");
  close(raw[1].close_price, 12.0, 1e-9, "duplicate keeps latest row");
  check(raw[2].close_time == 5002, "gap inserted at expected key");
  check(!raw[2].is_valid(), "gap row is masked invalid");

  const auto norm_csv = dir / "constant.csv";
  write_kline_csv(norm_csv, {
                                make_kline(6000, 10.0, 5.0),
                                make_kline(6001, 10.0, 5.0),
                                make_kline(6002, 10.0, 5.0),
                            });
  const auto norm_bin = mm::sanitize_csv_into_binary_file<Kline>(
      norm_csv.string(), "log_returns", true, 2);
  check(mm::is_bin_filename_normalized(norm_bin),
        "normalized filename recognized");
  const auto norm = read_cache_bin(norm_bin);
  check(!norm[0].is_valid(), "first normalized row is masked");
  check(norm[1].is_valid(), "second normalized row valid");
  close(norm[1].close_price, 0.0, 1e-8,
        "constant close price has zero log return");

  mm::MemoryMappedDataset<Kline> normalized_dataset(norm_bin, 2, 1);
  check(normalized_dataset.is_normalized(), "dataset reports normalized cache");
  auto sample = normalized_dataset.get(0);
  check(!bscalar(sample.mask.index({0})),
        "masked first normalized row propagates to sample");
  check(bscalar(sample.mask.index({1})), "valid normalized row mask");

  const auto bad_csv = dir / "bad_step.csv";
  write_kline_csv(bad_csv, {
                               make_kline(7000, 70.0),
                               make_kline(7002, 72.0),
                               make_kline(7005, 75.0),
                           });
  expect_memory_mapped_error(
      [&] {
        (void)mm::sanitize_csv_into_binary_file<Kline>(bad_csv.string(), "none",
                                                       true, 2);
      },
      "non-integer step ratio rejected", "non-integer step ratio");

  expect_memory_mapped_error(
      [&] {
        (void)mm::sanitize_csv_into_binary_file<Kline>(
            (dir / "missing.csv").string(), "none", true, 2);
      },
      "missing CSV rejected", "could not open CSV");

  expect_memory_mapped_error(
      [&] {
        (void)mm::sanitize_csv_into_binary_file<Kline>(
            norm_csv.string(), "unsupported_policy", true, 2);
      },
      "unsupported normalization rejected", "unsupported normalization policy");
}

void test_edge_sample_collate_roundtrip() {
  auto make_sample = [](double value, int64_t key_base) {
    dl::edge_sample_t sample{};
    sample.features = torch::full({2, 3, 9}, value, torch::kFloat32);
    sample.mask = torch::ones({2, 3}, torch::kBool);
    sample.future_features =
        torch::full({2, 2, 9}, value + 10.0, torch::kFloat32);
    sample.future_mask = torch::ones({2, 2}, torch::kBool);
    auto key_opts = torch::TensorOptions().dtype(torch::kInt64);
    sample.past_keys = torch::tensor({{key_base, key_base + 1, key_base + 2},
                                      {key_base, key_base + 1, key_base + 2}},
                                     key_opts);
    sample.future_keys = torch::tensor(
        {{key_base + 3, key_base + 4}, {key_base + 3, key_base + 4}}, key_opts);
    sample.feature_mean = torch::zeros({9}, torch::kFloat32);
    sample.feature_std = torch::ones({9}, torch::kFloat32);
    sample.normalized = true;
    return sample;
  };

  std::vector<dl::edge_sample_t> samples{make_sample(1.0, 8000),
                                         make_sample(2.0, 9000)};
  auto batch = dl::edge_sample_t::collate_fn(samples);
  check(batch.features.sizes() == torch::IntArrayRef({2, 2, 3, 9}),
        "collated features shape");
  check(batch.future_features.sizes() == torch::IntArrayRef({2, 2, 2, 9}),
        "collated future shape");
  check(batch.past_keys.sizes() == torch::IntArrayRef({2, 2, 3}),
        "collated past keys shape");
  check(batch.normalized, "collated normalized flag");

  const auto decollated = dl::edge_sample_t::decollate_fn(batch);
  check(decollated.size() == 2, "decollated count");
  close(scalar(decollated[1].features.index({0, 0, 0})), 2.0, 1e-6,
        "decollated feature value");
  close(scalar(decollated[1].past_keys.index({0, 2})), 9002.0, 1e-6,
        "decollated key value");
  check(decollated[1].normalized, "decollated normalized flag");
}

void test_root_edge_dataloader_wrapper() {
  const auto dir = make_tmp_dir("root_loader");
  const auto csv = dir / "btc_usdt.csv";
  write_kline_csv(csv, {
                           make_kline(10000, 100.0),
                           make_kline(10001, 101.0),
                           make_kline(10002, 102.0),
                           make_kline(10003, 103.0),
                       });

  dl::edge_dataloader_options_t options{};
  options.force_rebuild_cache = true;
  options.batch_size = 1;
  options.workers = 0;

  const auto spec = make_source_spec(csv, 2, 1);
  const auto materialization_request =
      mm::make_source_materialization_request(spec);
  auto loader = dl::make_sequential_edge_dataloader<Kline>(
      btc_usdt_signature(), materialization_request, options);
  check(loader.size().value() == 2, "root edge dataloader size");
  check(loader.channel_count() == 1, "root edge dataloader channels");
  check(loader.input_horizon() == 2, "root edge dataloader input horizon");
  check(loader.feature_width() == 9, "root edge dataloader feature width");
  check(loader.options().backend ==
            dl::edge_dataloader_backend_t::memory_mapped,
        "root edge dataloader backend");

  std::size_t batches = 0;
  for (auto &batch : loader) {
    (void)batch;
    ++batches;
  }
  check(batches == 2, "root edge dataloader sequential batches");

  auto direct_dataset = mm::create_memory_mapped_edge_dataset<Kline>(
      btc_usdt_signature(), materialization_request,
      /*force_rebuild_cache=*/true);
  dl::edge_dataloader_t<Kline> direct_loader(std::move(direct_dataset),
                                             options);
  check(direct_loader.size().value() == 2,
        "root edge dataloader direct dataset wrapper size");
}

} // namespace

int main() {
  test_frozen_cache_freshness_guard();
  test_kline_feature_registry_contract();
  test_kline_csv_canonicalizes_microsecond_timestamps();
  test_kline_tensor_and_normalization_follow_registry();
  test_single_edge_dataset_anchor_fields();
  test_storage_errors_are_catchable();
  test_zero_future_horizon_and_validation();
  test_edge_dataset_channel_padding_and_ranges();
  test_csv_sanitization_and_normalized_masks();
  test_edge_sample_collate_roundtrip();
  test_root_edge_dataloader_wrapper();
  std::cout << "[MemoryMappedStorage test] ok\n";
  return 0;
}
