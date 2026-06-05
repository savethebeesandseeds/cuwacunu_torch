#include "hero/lattice_hero/lattice/exposure/exposure_ledger.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace exposure = cuwacunu::hero::lattice::exposure;
namespace fs = std::filesystem;

namespace {

[[nodiscard]] fs::path benchmark_runtime_root() {
  if (const char *raw = std::getenv("CUWACUNU_LATTICE_BENCHMARK_RUNTIME_ROOT");
      raw != nullptr && std::string(raw).size() > 0) {
    return fs::path(raw);
  }
  return fs::path("/cuwacunu/.runtime/cuwacunu_exec");
}

template <typename Fn> [[nodiscard]] double mean_millis(Fn &&fn, int runs) {
  double total = 0.0;
  for (int i = 0; i < runs; ++i) {
    const auto begin = std::chrono::steady_clock::now();
    fn();
    const auto end = std::chrono::steady_clock::now();
    total += std::chrono::duration<double, std::milli>(end - begin).count();
  }
  return total / static_cast<double>(runs);
}

void require(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void print_row(const std::string &benchmark_id, const std::string &timing_layer,
               const std::string &proof_mode, double mean_ms) {
  std::cout << "BENCHMARK benchmark_id=" << benchmark_id
            << " timing_layer=" << timing_layer << " proof_mode=" << proof_mode
            << " mean_ms=" << mean_ms << "\n";
}

} // namespace

int main() {
  const auto runtime_root = benchmark_runtime_root();
  if (!fs::is_directory(runtime_root)) {
    std::cout << "SKIP runtime_root_missing=" << runtime_root.string() << "\n";
    return 0;
  }

  const auto index_path =
      runtime_root / "indexes" / "lattice_runtime_index.v1.lls";
  const auto live_cache = exposure::build_runtime_index_cache(runtime_root);
  fs::create_directories(index_path.parent_path());
  exposure::write_runtime_index_cache(index_path, live_cache);
  const auto cached = exposure::read_runtime_index_cache(index_path);

  exposure::lattice_runtime_index_query_t query{};
  query.relation = "checkpoint";

  const auto header_validation = exposure::validate_runtime_index_cache(
      cached, runtime_root,
      exposure::lattice_runtime_index_validation_strength_t::header_only);
  const auto header_query =
      exposure::query_runtime_index_cache(cached, query, 16);
  require(header_validation.cache_valid,
          "header_only validation should accept the freshly written cache");
  require(!header_validation.metadata_checked &&
              !header_validation.watched_metadata_checked,
          "header_only validation must not compute metadata digests");
  require(header_validation.row_set_checked &&
              header_validation.relation_counts_checked,
          "header_only validation must still check internal cache integrity");
  require(header_query.matching_row_count > 0,
          "checkpoint relation should be queryable from the runtime index");

  const auto watched_validation = exposure::validate_runtime_index_cache(
      cached, runtime_root,
      exposure::lattice_runtime_index_validation_strength_t::
          watched_file_manifest);
  require(watched_validation.cache_valid &&
              watched_validation.watched_metadata_checked &&
              !watched_validation.metadata_checked,
          "watched_file_manifest validation must avoid full metadata digest");

  constexpr int kFastRuns = 10;
  constexpr int kProofRuns = 3;
  print_row("library_read_runtime_index_cache", "library_function",
            "header_only",
            mean_millis(
                [&]() { (void)exposure::read_runtime_index_cache(index_path); },
                kFastRuns));
  print_row(
      "library_header_only_index_query", "library_function", "header_only",
      mean_millis(
          [&]() {
            const auto read = exposure::read_runtime_index_cache(index_path);
            const auto validation = exposure::validate_runtime_index_cache(
                read, runtime_root,
                exposure::lattice_runtime_index_validation_strength_t::
                    header_only);
            require(validation.cache_valid,
                    "header_only cache validation regressed");
            require(!validation.metadata_checked &&
                        !validation.watched_metadata_checked,
                    "header_only cache validation triggered metadata");
            (void)exposure::query_runtime_index_cache(read, query, 16);
          },
          kFastRuns));
  print_row(
      "library_watched_manifest_index_query", "library_function",
      "watched_file_manifest",
      mean_millis(
          [&]() {
            const auto read = exposure::read_runtime_index_cache(index_path);
            const auto validation = exposure::validate_runtime_index_cache(
                read, runtime_root,
                exposure::lattice_runtime_index_validation_strength_t::
                    watched_file_manifest);
            require(validation.cache_valid,
                    "watched-file cache validation regressed");
            require(validation.watched_metadata_checked &&
                        !validation.metadata_checked,
                    "watched-file validation triggered full metadata");
            (void)exposure::query_runtime_index_cache(read, query, 16);
          },
          kFastRuns));
  print_row(
      "library_full_metadata_index_status", "library_function",
      "full_runtime_metadata_digest",
      mean_millis(
          [&]() {
            const auto read = exposure::read_runtime_index_cache(index_path);
            const auto validation = exposure::validate_runtime_index_cache(
                read, runtime_root,
                exposure::lattice_runtime_index_validation_strength_t::
                    full_runtime_metadata_digest);
            require(validation.metadata_checked,
                    "full metadata validation did not check metadata");
          },
          kProofRuns));
  print_row(
      "library_live_scan_index_build", "library_function", "live_scan",
      mean_millis(
          [&]() { (void)exposure::build_runtime_index_cache(runtime_root); },
          kProofRuns));

  std::cout << "SUMMARY runtime_root=" << runtime_root.string()
            << " row_count=" << cached.rows.size()
            << " relation_count=" << cached.relation_counts.size()
            << " row_set_digest=" << cached.row_set_digest
            << " watched_file_metadata_digest="
            << cached.watched_file_metadata_digest << "\n";
  return 0;
}
