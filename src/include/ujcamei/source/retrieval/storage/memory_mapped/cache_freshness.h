/* cache_freshness.h */
#pragma once

#include <filesystem>
#include <system_error>

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace retrieval {
namespace storage {
namespace memory_mapped {

enum class cache_chain_freshness_status_t {
  ready,
  csv_missing,
  csv_symlink,
  csv_not_regular,
  raw_missing,
  raw_symlink,
  raw_not_regular,
  normalized_missing,
  normalized_symlink,
  normalized_not_regular,
  timestamp_error,
  raw_not_strictly_newer,
  normalized_not_strictly_newer,
};

struct cache_chain_freshness_t {
  cache_chain_freshness_status_t status{
      cache_chain_freshness_status_t::timestamp_error};
  std::filesystem::file_time_type csv_time{};
  std::filesystem::file_time_type raw_time{};
  std::filesystem::file_time_type normalized_time{};
  std::error_code error{};

  [[nodiscard]] bool ready() const noexcept {
    return status == cache_chain_freshness_status_t::ready;
  }
};

inline const char *
cache_chain_freshness_status_name(cache_chain_freshness_status_t status) {
  switch (status) {
  case cache_chain_freshness_status_t::ready:
    return "ready";
  case cache_chain_freshness_status_t::csv_missing:
    return "csv_missing";
  case cache_chain_freshness_status_t::csv_symlink:
    return "csv_symlink";
  case cache_chain_freshness_status_t::csv_not_regular:
    return "csv_not_regular";
  case cache_chain_freshness_status_t::raw_missing:
    return "raw_missing";
  case cache_chain_freshness_status_t::raw_symlink:
    return "raw_symlink";
  case cache_chain_freshness_status_t::raw_not_regular:
    return "raw_not_regular";
  case cache_chain_freshness_status_t::normalized_missing:
    return "normalized_missing";
  case cache_chain_freshness_status_t::normalized_symlink:
    return "normalized_symlink";
  case cache_chain_freshness_status_t::normalized_not_regular:
    return "normalized_not_regular";
  case cache_chain_freshness_status_t::timestamp_error:
    return "timestamp_error";
  case cache_chain_freshness_status_t::raw_not_strictly_newer:
    return "raw_not_strictly_newer";
  case cache_chain_freshness_status_t::normalized_not_strictly_newer:
    return "normalized_not_strictly_newer";
  }
  return "unknown";
}

inline bool
cache_time_is_strictly_newer(std::filesystem::file_time_type derived,
                             std::filesystem::file_time_type source) noexcept {
  return derived > source;
}

inline bool
cache_file_is_strictly_newer(const std::filesystem::path &derived,
                             const std::filesystem::path &source) noexcept {
  std::error_code derived_error;
  std::error_code source_error;
  if (!std::filesystem::exists(derived, derived_error) || derived_error ||
      !std::filesystem::exists(source, source_error) || source_error) {
    return false;
  }
  const auto derived_time =
      std::filesystem::last_write_time(derived, derived_error);
  const auto source_time =
      std::filesystem::last_write_time(source, source_error);
  return !derived_error && !source_error &&
         cache_time_is_strictly_newer(derived_time, source_time);
}

inline cache_chain_freshness_t inspect_cache_chain_freshness(
    const std::filesystem::path &csv, const std::filesystem::path &raw,
    const std::filesystem::path &normalized) noexcept {
  cache_chain_freshness_t result{};

  const auto inspect_file = [&](const std::filesystem::path &path,
                                cache_chain_freshness_status_t missing,
                                cache_chain_freshness_status_t symlink,
                                cache_chain_freshness_status_t not_regular) {
    std::error_code error;
    const auto status = std::filesystem::symlink_status(path, error);
    if (error) {
      if (error == std::errc::no_such_file_or_directory) {
        result.status = missing;
      } else {
        result.status = cache_chain_freshness_status_t::timestamp_error;
        result.error = error;
      }
      return false;
    }
    if (!std::filesystem::exists(status)) {
      result.status = missing;
      return false;
    }
    if (std::filesystem::is_symlink(status)) {
      result.status = symlink;
      return false;
    }
    if (!std::filesystem::is_regular_file(status)) {
      result.status = not_regular;
      return false;
    }
    return true;
  };

  if (!inspect_file(csv, cache_chain_freshness_status_t::csv_missing,
                    cache_chain_freshness_status_t::csv_symlink,
                    cache_chain_freshness_status_t::csv_not_regular) ||
      !inspect_file(raw, cache_chain_freshness_status_t::raw_missing,
                    cache_chain_freshness_status_t::raw_symlink,
                    cache_chain_freshness_status_t::raw_not_regular) ||
      !inspect_file(normalized,
                    cache_chain_freshness_status_t::normalized_missing,
                    cache_chain_freshness_status_t::normalized_symlink,
                    cache_chain_freshness_status_t::normalized_not_regular)) {
    return result;
  }

  std::error_code error;
  result.csv_time = std::filesystem::last_write_time(csv, error);
  if (error) {
    result.status = cache_chain_freshness_status_t::timestamp_error;
    result.error = error;
    return result;
  }
  result.raw_time = std::filesystem::last_write_time(raw, error);
  if (error) {
    result.status = cache_chain_freshness_status_t::timestamp_error;
    result.error = error;
    return result;
  }
  result.normalized_time = std::filesystem::last_write_time(normalized, error);
  if (error) {
    result.status = cache_chain_freshness_status_t::timestamp_error;
    result.error = error;
    return result;
  }

  if (!cache_time_is_strictly_newer(result.raw_time, result.csv_time)) {
    result.status = cache_chain_freshness_status_t::raw_not_strictly_newer;
    return result;
  }
  if (!cache_time_is_strictly_newer(result.normalized_time, result.raw_time)) {
    result.status =
        cache_chain_freshness_status_t::normalized_not_strictly_newer;
    return result;
  }
  result.status = cache_chain_freshness_status_t::ready;
  return result;
}

} // namespace memory_mapped
} // namespace storage
} // namespace retrieval
} // namespace source
} // namespace ujcamei
} // namespace cuwacunu
