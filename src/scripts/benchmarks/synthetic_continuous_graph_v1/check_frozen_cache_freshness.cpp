// Read-only cache-chain guard intended for pre-ledger use by future runners.
#include "ujcamei/source/retrieval/storage/memory_mapped/cache_freshness.h"

#include <cstddef>
#include <filesystem>
#include <iostream>

namespace mm = cuwacunu::ujcamei::source::retrieval::storage::memory_mapped;

namespace {

template <typename TimePoint>
auto ticks(const TimePoint &time) -> decltype(time.time_since_epoch().count()) {
  return time.time_since_epoch().count();
}

void print_usage(const char *program) {
  std::cerr << "usage: " << program
            << " CSV RAW_CACHE NORMALIZED_CACHE"
               " [CSV RAW_CACHE NORMALIZED_CACHE ...]\n";
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 4 || ((argc - 1) % 3) != 0) {
    print_usage(argv[0]);
    return 2;
  }

  const auto chain_count = static_cast<std::size_t>((argc - 1) / 3);
  for (std::size_t chain = 0; chain < chain_count; ++chain) {
    const auto offset = static_cast<int>(1 + chain * 3);
    const std::filesystem::path csv{argv[offset]};
    const std::filesystem::path raw{argv[offset + 1]};
    const std::filesystem::path normalized{argv[offset + 2]};
    const auto inspection =
        mm::inspect_cache_chain_freshness(csv, raw, normalized);
    if (!inspection.ready()) {
      std::cerr << "schema_id=synthetic_frozen_cache_freshness_guard.v1\n"
                << "status=fail\n"
                << "chain_index=" << chain << "\n"
                << "reason="
                << mm::cache_chain_freshness_status_name(inspection.status)
                << "\n"
                << "csv=" << csv.string() << "\n"
                << "raw_cache=" << raw.string() << "\n"
                << "normalized_cache=" << normalized.string() << "\n";
      if (inspection.status ==
              mm::cache_chain_freshness_status_t::raw_not_strictly_newer ||
          inspection.status == mm::cache_chain_freshness_status_t::
                                   normalized_not_strictly_newer) {
        std::cerr << "csv_mtime_ticks=" << ticks(inspection.csv_time) << "\n"
                  << "raw_mtime_ticks=" << ticks(inspection.raw_time) << "\n"
                  << "normalized_mtime_ticks="
                  << ticks(inspection.normalized_time) << "\n";
      }
      if (inspection.error) {
        std::cerr << "filesystem_error=" << inspection.error.message() << "\n";
      }
      return 1;
    }
  }

  std::cout << "schema_id=synthetic_frozen_cache_freshness_guard.v1\n"
            << "status=pass\n"
            << "chain_count=" << chain_count << "\n"
            << "writes_performed=false\n";
  return 0;
}
