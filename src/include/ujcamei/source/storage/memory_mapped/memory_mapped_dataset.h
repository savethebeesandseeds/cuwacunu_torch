/* memory_mapped_dataset.h */
#pragma once

#include <torch/torch.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstddef> // std::byte, offsetof
#include <cstring> // std::memcpy
#include <execution>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "piaabo/core/utils.h"
#include "piaabo/io/files.h"
#include "piaabo/tensor/torch/torch_utils.h"

#include "ujcamei/source/types/types_data.h"
#include "ujcamei/source/types/types_enums.h"
#include "ujcamei/source/types/types_utils.h"

#include "ujcamei/source/contract/contract.h"
#include "ujcamei/source/dataloader/edge_sample.h"
#include "ujcamei/source/storage/memory_mapped/memory_mapped_datafile.h"
#include "ujcamei/source/storage/memory_mapped/memory_mapped_error.h"

/* ============================================================
 *  Edge dataset grid policy (compile-time)
 *  - MIN => densest grid (smallest step across datasets)  [default]
 *  - MAX => coarsest grid (largest step across datasets)
 *  To override:
 *    -DCUWACUNU_EDGE_DATASET_GRID_STEP_POLICY=CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY_MAX
 * ============================================================ */
#ifndef CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY_MIN
#define CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY_MIN 0
#endif
#ifndef CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY_MAX
#define CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY_MAX 1
#endif
#ifndef CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY
#define CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY                                 \
  CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY_MIN
#endif

/* Floating-point alignment tolerance used for snapping to the grid */
#ifndef CUWACUNU_EDGE_DATASET_ALIGN_TOL
#define CUWACUNU_EDGE_DATASET_ALIGN_TOL 1e-9
#endif
#ifndef CUWACUNU_EDGE_DATASET_ALIGN_REL_TOL
#define CUWACUNU_EDGE_DATASET_ALIGN_REL_TOL 1e-12
#endif

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace storage {
namespace memory_mapped {

using ::cuwacunu::ujcamei::source::dataloader::edge_sample_t;

struct source_materialization_request_t {
  std::vector<cuwacunu::ujcamei::source::contract::source_form_t>
      source_forms{};
  std::vector<cuwacunu::ujcamei::source::contract::channel_form_t>
      channel_forms{};
  std::size_t csv_bootstrap_deltas{64};
  long double csv_step_abs_tol{1e-8L};
  long double csv_step_rel_tol{1e-10L};
};

[[nodiscard]] inline source_materialization_request_t
make_source_materialization_request(
    const cuwacunu::ujcamei::source::contract::source_universe_t
        &source_universe,
    std::vector<cuwacunu::ujcamei::source::contract::channel_form_t>
        channel_forms) {
  source_materialization_request_t out{};
  out.source_forms = source_universe.source_forms;
  out.channel_forms = std::move(channel_forms);
  out.csv_bootstrap_deltas = source_universe.csv_bootstrap_deltas;
  out.csv_step_abs_tol = source_universe.csv_step_abs_tol;
  out.csv_step_rel_tol = source_universe.csv_step_rel_tol;
  return out;
}

[[nodiscard]] inline source_materialization_request_t
make_source_materialization_request(
    const cuwacunu::ujcamei::source::contract::source_spec_t &source_spec) {
  return make_source_materialization_request(
      cuwacunu::ujcamei::source::contract::make_source_universe_from_compat(
          source_spec),
      source_spec.channel_forms);
}

[[nodiscard]] inline std::vector<
    cuwacunu::ujcamei::source::contract::source_form_t>
filter_source_materialization_forms(
    const source_materialization_request_t &request,
    const cuwacunu::ujcamei::source::instrument_signature_t &target_signature,
    cuwacunu::ujcamei::source::types::interval_type_e target_interval) {
  std::vector<cuwacunu::ujcamei::source::contract::source_form_t> result;
  for (const auto &form : request.source_forms) {
    if (form.interval != target_interval) {
      continue;
    }
    if (cuwacunu::ujcamei::source::instrument_signature_compact_string(
            form.instrument_signature()) ==
        cuwacunu::ujcamei::source::instrument_signature_compact_string(
            target_signature)) {
      result.push_back(form);
    }
  }
  return result;
}

/**
 * @brief Reads a specific value from a memory-mapped structure.
 *
 * @tparam Datatype_t The struct type, which defines `key_type_t`.
 */
template <typename Datatype_t>
typename Datatype_t::key_type_t
read_memory_value(const void *data_ptr, std::size_t index, std::size_t offset) {
  typename Datatype_t::key_type_t value{};
  const std::byte *base = static_cast<const std::byte *>(data_ptr);
  const std::byte *target = base + index * sizeof(Datatype_t) + offset;
  std::memcpy(&value, target, sizeof(typename Datatype_t::key_type_t));
  return value;
}

/**
 * @brief Reads a full record from the memory-mapped data.
 */
template <typename Datatype_t>
Datatype_t read_memory_struct(const void *data_ptr, std::size_t index) {
  Datatype_t record{};
  const std::byte *base = static_cast<const std::byte *>(data_ptr);
  const std::byte *target = base + index * sizeof(Datatype_t);
  std::memcpy(&record, target, sizeof(Datatype_t));
  return record;
}

/**
 * @brief Reads multiple records from the memory-mapped data.
 */
template <typename Datatype_t>
std::vector<Datatype_t> read_memory_structs(const void *data_ptr,
                                            std::size_t index,
                                            std::size_t count) {
  if (count == 0)
    return {};
  std::vector<Datatype_t> records(
      count); // Allocate memory for `count` records.
  const std::byte *base = static_cast<const std::byte *>(data_ptr);
  const std::byte *target = base + index * sizeof(Datatype_t);
  std::memcpy(records.data(), target, count * sizeof(Datatype_t));
  return records;
}

/*
 * Like absolute difference but stable for signed/unsigned integrals.
 */
template <typename T> constexpr auto absolute_difference(T a, T b) {
  if constexpr (std::is_floating_point_v<T>) {
    return std::abs(a - b);
  } else {
    using U = std::make_unsigned_t<T>;
    return (a >= b) ? U(a - b) : U(b - a);
  }
}

/* ------------------------------------------------------------
 * Helpers for grid alignment and step counting (integral/float)
 * ------------------------------------------------------------ */
namespace detail {

template <typename T> inline std::string value_to_string(const T &value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

template <typename K, typename = std::enable_if_t<std::is_integral_v<K>>>
inline K align_up_to_grid(K x, K step, K base) {
  // returns smallest y >= x s.t. (y - base) % step == 0
  if (step <= 0)
    return x;
  K diff = x - base;
  K r = diff % step;
  if (r < 0)
    r += step; // make remainder positive
  if (r == 0)
    return x;
  return static_cast<K>(x + (step - r));
}

template <typename K, typename = std::enable_if_t<std::is_integral_v<K>>>
inline K align_down_to_grid(K x, K step, K base) {
  // returns largest y <= x s.t. (y - base) % step == 0
  if (step <= 0)
    return x;
  K diff = x - base;
  K r = diff % step;
  if (r < 0)
    r += step; // make remainder positive
  return static_cast<K>(x - r);
}

template <typename K, typename = std::enable_if_t<std::is_integral_v<K>>>
inline std::size_t steps_between_inclusive(K left_aligned, K right_aligned,
                                           K step) {
  // pre: left_aligned and right_aligned are both on the grid, step > 0
  if (right_aligned < left_aligned)
    return 0;
  return static_cast<std::size_t>((right_aligned - left_aligned) / step) + 1;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline K effective_fp_tol(K a, K b) {
  const K abs_tol = static_cast<K>(CUWACUNU_EDGE_DATASET_ALIGN_TOL);
  const K rel_tol = static_cast<K>(CUWACUNU_EDGE_DATASET_ALIGN_REL_TOL);
  const K scale = std::max(K(1), std::max(std::fabs(a), std::fabs(b)));
  return abs_tol + rel_tol * scale;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline bool fp_less_with_tol(K a, K b) {
  return (a + effective_fp_tol(a, b)) < b;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline bool fp_le_with_tol(K a, K b) {
  return !fp_less_with_tol<K>(b, a);
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline K align_up_to_grid_fp(K x, K step, K base) {
  if (step <= K(0))
    return x;
  const K q = (x - base) / step;
  const K eps = effective_fp_tol<K>(q, K(0));
  K k = std::ceil(q - eps);
  return base + k * step;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline K align_down_to_grid_fp(K x, K step, K base) {
  if (step <= K(0))
    return x;
  const K q = (x - base) / step;
  const K eps = effective_fp_tol<K>(q, K(0));
  K k = std::floor(q + eps);
  return base + k * step;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline std::size_t steps_between_inclusive_fp(K left_aligned, K right_aligned,
                                              K step) {
  if (step <= K(0))
    return 0;
  if (fp_less_with_tol<K>(right_aligned, left_aligned))
    return 0;
  const K span = (right_aligned - left_aligned) / step;
  const K eps = effective_fp_tol<K>(span, K(0));
  K k = std::floor(span + eps);
  if (k < 0)
    return 0;
  return static_cast<std::size_t>(k) + 1;
}

template <typename T>
inline constexpr const char *record_type_name_for_datatype() {
  if constexpr (std::is_same_v<T, cuwacunu::ujcamei::source::types::kline_t>) {
    return "kline";
  } else if constexpr (std::is_same_v<
                           T,
                           cuwacunu::ujcamei::source::types::kline_cache_t>) {
    return "kline";
  } else if constexpr (std::is_same_v<
                           T, cuwacunu::ujcamei::source::types::trade_t>) {
    return "trade";
  } else if constexpr (std::is_same_v<
                           T,
                           cuwacunu::ujcamei::source::types::trade_cache_t>) {
    return "trade";
  } else if constexpr (std::is_same_v<
                           T, cuwacunu::ujcamei::source::types::basic_t>) {
    return "basic";
  } else if constexpr (std::is_same_v<
                           T,
                           cuwacunu::ujcamei::source::types::basic_cache_t>) {
    return "basic";
  } else {
    return "";
  }
}

inline std::size_t parse_required_size(const std::string &raw,
                                       const char *field,
                                       const std::string &channel_label) {
  try {
    std::size_t consumed = 0;
    const auto value = std::stoull(raw, &consumed);
    if (consumed != raw.size()) {
      throw std::invalid_argument("trailing characters");
    }
    return static_cast<std::size_t>(value);
  } catch (const std::exception &ex) {
    throw_memory_mapped_error({.where = "create_memory_mapped_edge_dataset",
                               .reason = std::string("invalid ") + field +
                                         "='" + raw + "' for " + channel_label +
                                         ": " + ex.what()});
  }
}

} // namespace detail

/**
 * @brief A memory-mapped dataset for PyTorch-based data loading.
 *
 * Anchor-indexed edge sample semantics:
 *   - input_length_: number of input frames returned (last row is time t)
 *   - future_length_: number of future frames returned (first row is t+1)
 *   - stride is 1 (anchor moves by one raw record)
 *
 * @tparam Datatype_t The struct type of the records stored in the binary file.
 */
template <typename Datatype_t>
class MemoryMappedDataset
    : public torch::data::datasets::Dataset<MemoryMappedDataset<Datatype_t>,
                                            edge_sample_t> {
  using BinaryDatatype_t =
      cuwacunu::ujcamei::source::storage::memory_mapped::binary_record_type_t<
          Datatype_t>;

private:
  struct MappedData {
    int fd_;                /* File descriptor for the binary file */
    void *data_ptr_;        /* Pointer to the memory-mapped data */
    std::size_t file_size_; /* Size of the binary file in bytes */

    explicit MappedData(const std::string &bin_filename)
        : fd_(-1), data_ptr_(nullptr), file_size_(0) {

      fd_ = open(bin_filename.c_str(), O_RDONLY);
      if (fd_ == -1) {
        const int saved_errno = errno;
        throw_memory_mapped_errno_error(
            {.where = "MemoryMappedDataset::MappedData::open",
             .file = bin_filename,
             .reason = "could not open binary file"},
            saved_errno);
      }

      struct stat st {};
      if (fstat(fd_, &st) == -1) {
        const int saved_errno = errno;
        close(fd_);
        fd_ = -1;
        throw_memory_mapped_errno_error(
            {.where = "MemoryMappedDataset::MappedData::fstat",
             .file = bin_filename,
             .reason = "failed to determine file size"},
            saved_errno);
      }
      file_size_ = static_cast<std::size_t>(st.st_size);

      if (file_size_ == 0) {
        close(fd_);
        fd_ = -1;
        throw_memory_mapped_error({.where = "MemoryMappedDataset::MappedData",
                                   .file = bin_filename,
                                   .reason = "binary file is empty"});
      }

      data_ptr_ = mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
      if (data_ptr_ == MAP_FAILED) {
        const int saved_errno = errno;
        data_ptr_ = nullptr;
        close(fd_);
        fd_ = -1;
        throw_memory_mapped_errno_error(
            {.where = "MemoryMappedDataset::MappedData::mmap",
             .file = bin_filename,
             .reason = "failed to memory-map binary file"},
            saved_errno);
      }
    }

    ~MappedData() {
      if (data_ptr_ && data_ptr_ != MAP_FAILED) {
        munmap(data_ptr_, file_size_);
      }
      if (fd_ != -1) {
        close(fd_);
      }
    }
  };

private:
  std::string bin_filename_; /* Path to the binary file */
  std::unique_ptr<MappedData>
      mapped_data_;         /* Unique pointer to memory-mapped data */
  std::size_t num_records_; /* Raw number of records (rows) in the dataset */

public:
  /* Variables for key-value boundaries. */
  std::size_t
      key_value_offset_; /* Offset (in bytes) of the key in each record */
  typename Datatype_t::key_type_t
      leftmost_key_value_; /* Smallest key value in the dataset */
  typename Datatype_t::key_type_t
      rightmost_key_value_; /* Largest key value in the dataset */
  typename Datatype_t::key_type_t key_value_span_; /* rightmost - leftmost */
  typename Datatype_t::key_type_t
      key_value_step_; /* regular increment in key_value between consecutive
                          records */

  /* Anchor-indexed input/future configuration */
  std::size_t input_length_{1};
  std::size_t future_length_{1};
  std::size_t anchor_count_{0}; /* Number of valid anchor positions. */
  bool normalized_records_{
      false}; /* Whether the mapped file is a normalized cache */

private:
  // Build a 1D tensor of keys from a mutable vector (key_value() is non-const).
  static inline torch::Tensor
  keys_from_records_1d(std::vector<BinaryDatatype_t> &recs) {
    using KeyT = typename BinaryDatatype_t::key_type_t;
    const long n = static_cast<long>(recs.size());
    if constexpr (std::is_integral_v<KeyT>) {
      if (n == 0)
        return torch::empty({0}, torch::TensorOptions().dtype(torch::kInt64));
    } else {
      if (n == 0)
        return torch::empty({0}, torch::TensorOptions().dtype(torch::kFloat64));
    }
    if constexpr (std::is_integral_v<KeyT>) {
      std::vector<long long> v(recs.size());
      for (size_t i = 0; i < recs.size(); ++i)
        v[i] = static_cast<long long>(recs[i].key_value());
      return torch::from_blob(v.data(), {n},
                              torch::TensorOptions().dtype(torch::kInt64))
          .clone();
    } else {
      std::vector<double> v(recs.size());
      for (size_t i = 0; i < recs.size(); ++i)
        v[i] = static_cast<double>(recs[i].key_value());
      return torch::from_blob(v.data(), {n},
                              torch::TensorOptions().dtype(torch::kFloat64))
          .clone();
    }
  }

public:
  explicit MemoryMappedDataset(const std::string &bin_filename,
                               std::size_t input_length = 1,
                               std::size_t future_length = 1)
      : bin_filename_(bin_filename),
        mapped_data_(std::make_unique<MappedData>(bin_filename)),
        num_records_(mapped_data_->file_size_ / sizeof(BinaryDatatype_t)),
        key_value_offset_(BinaryDatatype_t::key_offset()),
        input_length_(input_length), future_length_(future_length) {

    if (mapped_data_->file_size_ % sizeof(BinaryDatatype_t) != 0) {
      throw_memory_mapped_error(
          {.where = "MemoryMappedDataset::MemoryMappedDataset",
           .file = bin_filename_,
           .reason = "binary file size is not a multiple of record size"});
    }
    if (num_records_ == 0) {
      throw_memory_mapped_error(
          {.where = "MemoryMappedDataset::MemoryMappedDataset",
           .file = bin_filename_,
           .reason = "binary dataset has zero records"});
    }
    if (input_length_ == 0) {
      throw_memory_mapped_error(
          {.where = "MemoryMappedDataset::MemoryMappedDataset",
           .file = bin_filename_,
           .reason = "input_length must be >= 1"});
    }

    normalized_records_ = is_bin_filename_normalized(bin_filename_);

    leftmost_key_value_ = read_memory_value<BinaryDatatype_t>(
        mapped_data_->data_ptr_, 0, key_value_offset_);
    rightmost_key_value_ = read_memory_value<BinaryDatatype_t>(
        mapped_data_->data_ptr_, num_records_ - 1, key_value_offset_);
    key_value_span_ = rightmost_key_value_ - leftmost_key_value_;

    if (num_records_ > 1 && !(leftmost_key_value_ < rightmost_key_value_)) {
      throw_memory_mapped_error(
          {.where = "MemoryMappedDataset::MemoryMappedDataset",
           .file = bin_filename_,
           .reason = "binary dataset is not sorted correctly"});
    }

    static_assert(std::is_same_v<decltype(std::declval<BinaryDatatype_t>()
                                              .tensor_features()),
                                 std::vector<double>>,
                  "[MemoryMappedDataset] Error: Template argument must provide "
                  "tensor_features() returning std::vector<double>");
    static_assert(std::is_standard_layout<BinaryDatatype_t>::value,
                  "[MemoryMappedDataset] Error: Template argument must be a "
                  "standard layout type");
    static_assert(std::is_trivially_copyable<BinaryDatatype_t>::value,
                  "[MemoryMappedDataset] Error: Template argument must be "
                  "trivially copyable");
    static_assert(
        std::is_same_v<typename Datatype_t::key_type_t,
                       typename BinaryDatatype_t::key_type_t>,
        "[MemoryMappedDataset] Error: raw and binary key types must match");

    if (num_records_ == 1) {
      if constexpr (std::is_integral_v<typename Datatype_t::key_type_t>) {
        key_value_step_ = static_cast<typename Datatype_t::key_type_t>(1);
      } else {
        key_value_step_ = static_cast<typename Datatype_t::key_type_t>(1.0);
      }
    } else {
      // Infer regular step (warn on irregularities)
      typename Datatype_t::key_type_t prev =
          read_memory_value<BinaryDatatype_t>(mapped_data_->data_ptr_, 0,
                                              key_value_offset_);
      typename Datatype_t::key_type_t curr =
          read_memory_value<BinaryDatatype_t>(mapped_data_->data_ptr_, 1,
                                              key_value_offset_);
      key_value_step_ = curr - prev;

      if (key_value_step_ <= 0) {
        throw_memory_mapped_error(
            {.where = "MemoryMappedDataset::MemoryMappedDataset",
             .file = bin_filename_,
             .reason = "negative or zero key_value_step"});
      }

      for (std::size_t idx = 1; idx < num_records_; ++idx) {
        curr = read_memory_value<BinaryDatatype_t>(mapped_data_->data_ptr_, idx,
                                                   key_value_offset_);
        if (curr < prev) {
          throw_memory_mapped_error(
              {.where = "MemoryMappedDataset::MemoryMappedDataset",
               .file = bin_filename_,
               .dataset_index = idx,
               .reason = "binary dataset is not sequential and increasing"});
        }
        if constexpr (std::is_floating_point_v<
                          typename Datatype_t::key_type_t>) {
          const auto delta = curr - prev;
          const auto tol =
              detail::effective_fp_tol<decltype(delta)>(delta, key_value_step_);
          if (std::abs(delta - key_value_step_) > tol) {
            log_warn("[MemoryMappedDataset] record on file [%s] irregular key "
                     "delta at index [%zu]: (curr - prev): %f != step: %f\n",
                     bin_filename_.c_str(), idx, static_cast<double>(delta),
                     static_cast<double>(key_value_step_));
          }
        } else {
          if (((curr - prev) != key_value_step_)) {
            log_warn(
                "[MemoryMappedDataset] record on file [%s] irregular key delta "
                "at index [%zu]: (curr - prev): %lld != step: %lld\n",
                bin_filename_.c_str(), idx, static_cast<long long>(curr - prev),
                static_cast<long long>(key_value_step_));
          }
        }
        prev = curr;
      }
    }

    // Compute valid anchor count (stride = 1).
    if (num_records_ >= (input_length_ + future_length_)) {
      anchor_count_ = num_records_ - (input_length_ + future_length_) + 1;
    } else {
      anchor_count_ = 0;
    }
  }

  /**
   * @brief Expose raw row count for external validation.
   */
  std::size_t raw_records() const noexcept { return num_records_; }
  bool is_normalized() const noexcept { return normalized_records_; }

  [[nodiscard]] bool can_get_edge_sample_at_anchor_key(
      typename Datatype_t::key_type_t target_key_value,
      std::size_t input_length, std::size_t future_length,
      std::string *error = nullptr) const {
    auto fail = [&](const std::string &reason) {
      if (error != nullptr) {
        *error = reason;
      }
      return false;
    };
    if (input_length == 0) {
      return fail("input_length must be >= 1");
    }
    if (num_records_ == 0) {
      return fail("dataset has zero records");
    }

    std::size_t i = 0;
    try {
      i = find_closest_index(target_key_value);
    } catch (const std::exception &ex) {
      return fail(ex.what());
    }

    if (future_length > 0) {
      const std::size_t future_available = num_records_ - 1 - i;
      if (future_length > future_available) {
        return fail("future field exceeds dataset size at key " +
                    detail::value_to_string(target_key_value));
      }
    }
    if (i + 1 < input_length) {
      return fail("input field exceeds dataset start at key " +
                  detail::value_to_string(target_key_value));
    }
    return true;
  }

  /**
   * @brief Retrieves one edge sample around an anchor key.
   *
   * The input field ends at the anchor. The future field begins at the first
   * record after the anchor.
   */
  edge_sample_t get_edge_sample_at_anchor_key(
      typename Datatype_t::key_type_t target_key_value,
      std::size_t input_length, std::size_t future_length) {
    if (input_length == 0) {
      throw std::invalid_argument(
          "[MemoryMappedDataset] input_length must be >= 1 in "
          "get_edge_sample_at_anchor_key");
    }
    std::size_t i = find_closest_index(target_key_value);

    // Bounds: need [i-(input_length-1) ... i] and [i+1 ... i+future_length]
    if (future_length > 0 && i + future_length >= num_records_) {
      throw std::out_of_range(
          "[MemoryMappedDataset] future field exceeds dataset size at key " +
          std::to_string(static_cast<long long>(target_key_value)));
    }
    if (i + 1 < input_length) {
      throw std::out_of_range(
          "[MemoryMappedDataset] input field exceeds dataset start at key " +
          std::to_string(static_cast<long long>(target_key_value)));
    }

    const std::size_t input_start = i - (input_length - 1);
    auto input_records = read_memory_structs<BinaryDatatype_t>(
        mapped_data_->data_ptr_, input_start, input_length);
    if (input_records.empty()) {
      throw std::runtime_error("[MemoryMappedDataset] empty input field in "
                               "get_edge_sample_at_anchor_key");
    }
    const std::size_t D = input_records[0].tensor_features().size();

    torch::Tensor input_features =
        torch::empty({static_cast<long>(input_length), static_cast<long>(D)},
                     torch::kFloat32);
    torch::Tensor input_mask =
        torch::empty({static_cast<long>(input_length)}, torch::kBool);
    {
      float *x = input_features.data_ptr<float>();
      bool *m = input_mask.data_ptr<bool>();
      for (std::size_t k = 0; k < input_length; ++k) {
        const auto &v = input_records[k].tensor_features();
        std::copy(v.begin(), v.end(), x + k * D);
        m[k] = input_records[k].is_valid();
      }
    }
    torch::Tensor past_keys = keys_from_records_1d(input_records);

    const std::size_t future_start = i + 1;
    auto future_records = read_memory_structs<BinaryDatatype_t>(
        mapped_data_->data_ptr_, future_start, future_length);

    torch::Tensor future_features =
        torch::empty({static_cast<long>(future_length), static_cast<long>(D)},
                     torch::kFloat32);
    torch::Tensor future_mask =
        torch::empty({static_cast<long>(future_length)}, torch::kBool);
    {
      float *x = future_features.data_ptr<float>();
      bool *m = future_mask.data_ptr<bool>();
      for (std::size_t k = 0; k < future_length; ++k) {
        const auto &v = future_records[k].tensor_features();
        std::copy(v.begin(), v.end(), x + k * D);
        m[k] = future_records[k].is_valid();
      }
    }
    torch::Tensor fut_keys = keys_from_records_1d(future_records);

    edge_sample_t s{input_features, input_mask, future_features, future_mask,
                    torch::Tensor()};
    s.past_keys = past_keys;
    s.future_keys = fut_keys;
    s.normalized = normalized_records_;
    return s;
  }

  /**
   * @brief Anchor-indexed get.
   *
   * Returns {input[input_length_], future[future_length_]} for the anchor at
   * raw index (input_length_ - 1) + index.
   */
  edge_sample_t get(std::size_t index) override {
    if (index >= anchor_count_) {
      throw std::out_of_range("[MemoryMappedDataset] index " +
                              std::to_string(index) + " out of range [0, " +
                              std::to_string(anchor_count_) +
                              ") file=" + bin_filename_);
    }

    const std::size_t a = (input_length_ > 0 ? (input_length_ - 1) : 0) + index;
    const std::size_t input_start =
        (input_length_ > 0 ? (a - (input_length_ - 1)) : a);
    const std::size_t future_start = a + 1;

    auto input_records = read_memory_structs<BinaryDatatype_t>(
        mapped_data_->data_ptr_, input_start, input_length_);
    auto future_records = read_memory_structs<BinaryDatatype_t>(
        mapped_data_->data_ptr_, future_start, future_length_);

    if (input_records.empty()) {
      throw_memory_mapped_error({.where = "MemoryMappedDataset::get",
                                 .file = bin_filename_,
                                 .dataset_index = index,
                                 .reason = "empty input field"});
    }
    const std::size_t D = input_records[0].tensor_features().size();

    torch::Tensor input_features =
        torch::empty({static_cast<long>(input_length_), static_cast<long>(D)},
                     torch::kFloat32);
    torch::Tensor input_mask =
        torch::empty({static_cast<long>(input_length_)}, torch::kBool);
    {
      float *x = input_features.data_ptr<float>();
      bool *m = input_mask.data_ptr<bool>();
      for (std::size_t k = 0; k < input_length_; ++k) {
        const auto &v = input_records[k].tensor_features();
        std::copy(v.begin(), v.end(), x + k * D);
        m[k] = input_records[k].is_valid();
      }
    }
    torch::Tensor past_keys = keys_from_records_1d(input_records);

    torch::Tensor future_features =
        torch::empty({static_cast<long>(future_length_), static_cast<long>(D)},
                     torch::kFloat32);
    torch::Tensor future_mask =
        torch::empty({static_cast<long>(future_length_)}, torch::kBool);
    {
      float *x = future_features.data_ptr<float>();
      bool *m = future_mask.data_ptr<bool>();
      for (std::size_t k = 0; k < future_length_; ++k) {
        const auto &v = future_records[k].tensor_features();
        std::copy(v.begin(), v.end(), x + k * D);
        m[k] = future_records[k].is_valid();
      }
    }
    torch::Tensor fut_keys = keys_from_records_1d(future_records);

    edge_sample_t s{input_features, input_mask, future_features, future_mask,
                    torch::Tensor()};
    s.past_keys = past_keys;
    s.future_keys = fut_keys;
    s.normalized = normalized_records_;
    return s;
  }

  /* size() is the number of valid anchor positions. */
  torch::optional<std::size_t> size() const override { return anchor_count_; }

  /* Common samplers */
  torch::data::samplers::SequentialSampler SequentialSampler() const {
    return torch::data::samplers::SequentialSampler(anchor_count_);
  }
  torch::data::DataLoaderOptions
  SequentialSampler_options(std::size_t batch_size = 64,
                            std::size_t workers = 0) const {
    return torch::data::DataLoaderOptions()
        .batch_size(batch_size)
        .workers(workers);
  }
  torch::data::samplers::RandomSampler RandomSampler() const {
    return torch::data::samplers::RandomSampler(anchor_count_);
  }
  torch::data::DataLoaderOptions
  RandomSampler_options(std::size_t batch_size = 64,
                        std::size_t workers = 0) const {
    return torch::data::DataLoaderOptions()
        .batch_size(batch_size)
        .workers(workers);
  }

  /**
   * @brief Finds the closest index for a given key value using a safe
   * interpolation strategy. Returns the last index whose key <=
   * target_key_value.
   */
  std::size_t
  find_closest_index(typename Datatype_t::key_type_t target_key_value) const {
    if (num_records_ == 0) {
      throw std::out_of_range("[MemoryMappedDataset] dataset is empty: " +
                              bin_filename_);
    }
    if (target_key_value <= leftmost_key_value_)
      return 0;
    if (target_key_value >= rightmost_key_value_)
      return num_records_ - 1;

    std::size_t left = 0;
    std::size_t right = num_records_ - 1;
    std::size_t best_index = 0;
    using key_t = typename Datatype_t::key_type_t;
    using diff_t = decltype(absolute_difference(std::declval<key_t>(),
                                                std::declval<key_t>()));
    diff_t best_diff = std::numeric_limits<diff_t>::max();
    auto left_key_value = leftmost_key_value_;
    auto right_key_value = rightmost_key_value_;

    while (left <= right) {
      if (left_key_value == right_key_value)
        break;

      const double num = static_cast<double>(target_key_value - left_key_value);
      const double den = static_cast<double>(right_key_value - left_key_value);
      double r = num / den;
      if (r < 0.0)
        r = 0.0;
      if (r > 1.0)
        r = 1.0;

      std::size_t mid = left + static_cast<std::size_t>(r * (right - left));
      if (mid >= num_records_)
        mid = num_records_ - 1;

      auto mid_key_value = read_memory_value<BinaryDatatype_t>(
          mapped_data_->data_ptr_, mid, key_value_offset_);

      if (mid_key_value <= target_key_value) {
        const auto diff = absolute_difference(mid_key_value, target_key_value);
        if (diff < best_diff) {
          best_diff = diff;
          best_index = mid;
        }
        if (mid == right)
          break;
        left = mid + 1;
        left_key_value = read_memory_value<BinaryDatatype_t>(
            mapped_data_->data_ptr_, left, key_value_offset_);
      } else {
        if (mid == 0)
          break;
        right = mid - 1;
        right_key_value = read_memory_value<BinaryDatatype_t>(
            mapped_data_->data_ptr_, right, key_value_offset_);
      }
    }
    return best_index;
  }

  /**
   * ====================== (A) anchor-range slicing ========================
   * Return edge samples whose anchor key is within [key_left, key_right].
   * No clamping/padding beyond the natural grid. Vector<edge_sample_t>.
   */
  std::vector<edge_sample_t>
  range_edge_samples_by_anchor_keys(typename Datatype_t::key_type_t key_left,
                                    typename Datatype_t::key_type_t key_right) {
    using key_t = typename Datatype_t::key_type_t;
    std::vector<edge_sample_t> out;
    if (num_records_ == 0 || anchor_count_ == 0)
      return out;
    if (key_right < key_left)
      std::swap(key_left, key_right);

    // compute raw anchor bounds [a_min, a_max] such that key[a] in [left,
    // right] first raw index >= left
    std::size_t idx_left = find_closest_index(key_left);
    key_t key_at_left = read_memory_value<BinaryDatatype_t>(
        mapped_data_->data_ptr_, idx_left, key_value_offset_);
    if (key_at_left < key_left && idx_left + 1 < num_records_)
      ++idx_left;

    // last raw index <= right
    std::size_t idx_right = find_closest_index(key_right);
    key_t key_at_right = read_memory_value<BinaryDatatype_t>(
        mapped_data_->data_ptr_, idx_right, key_value_offset_);
    if (key_at_right > key_right && idx_right > 0)
      --idx_right;

    // Translate to valid anchor range respecting input/future lengths.
    const std::size_t a_min_natural =
        (input_length_ > 0 ? (input_length_ - 1) : 0);
    const std::size_t a_max_natural = num_records_ - 1 - future_length_;
    if (idx_left > a_max_natural || idx_right < a_min_natural ||
        idx_left > idx_right) {
      return out; // empty
    }
    std::size_t a_min = std::max(idx_left, a_min_natural);
    std::size_t a_max = std::min(idx_right, a_max_natural);
    if (a_min > a_max)
      return out;

    // iterate natural anchors and re-use get()
    out.reserve(a_max - a_min + 1);
    for (std::size_t a = a_min; a <= a_max; ++a) {
      const std::size_t anchor_idx =
          a - (input_length_ > 0 ? (input_length_ - 1) : 0);
      out.emplace_back(get(anchor_idx));
    }
    return out;
  }
};

/**
 * @brief A memory-mapped edge dataset for efficient data access across
 * multiple sources.
 *
 * Each source can specify (input_length, future_length). The dataset pads to
 * (max_input_length_, max_future_length_). Sampling domain is the intersection
 * of valid target positions across all sources.
 *
 * The global sampling grid (step/anchors) is chosen by compile-time policy:
 *   - MIN: densest grid (smallest step across datasets)
 *   - MAX: coarsest grid (largest step across datasets)
 * The final left/right bounds are aligned to the chosen grid to avoid drift.
 */
template <typename Datatype_t>
class MemoryMappedEdgeDataset
    : public torch::data::datasets::Dataset<MemoryMappedEdgeDataset<Datatype_t>,
                                            edge_sample_t> {
private:
  std::vector<std::shared_ptr<MemoryMappedDataset<Datatype_t>>> datasets_;
  std::vector<std::string> file_names_;
  std::vector<std::size_t> input_length_;
  std::vector<std::size_t> future_length_;

  // Cache of per-dataset valid range (in key space) after (input_length,
  // future_length)
  std::vector<typename Datatype_t::key_type_t>
      valid_left_; // leftmost key where a target index is valid
  std::vector<typename Datatype_t::key_type_t>
      valid_right_; // rightmost key where a target index is valid

  // Index of dataset that defines the global grid (based on policy)
  std::size_t grid_ref_idx_{static_cast<std::size_t>(-1)};

public:
  std::size_t max_input_length_{0};
  std::size_t max_future_length_{0};

  std::size_t num_records_{0}; /* number of valid target positions across
                                  intersection (on the chosen grid) */
  typename Datatype_t::key_type_t
      leftmost_key_value_{}; /* intersection lower bound aligned to grid */
  typename Datatype_t::key_type_t
      rightmost_key_value_{}; /* intersection upper bound aligned to grid */
  typename Datatype_t::key_type_t key_value_span_{}; /* rightmost - leftmost */
  typename Datatype_t::key_type_t
      key_value_step_{}; /* global step used by the edge dataset (per
                            policy) */

public:
  MemoryMappedEdgeDataset() = default;

  edge_sample_t get(std::size_t index) override {
    if (index >= num_records_) {
      throw std::out_of_range(
          "[MemoryMappedEdgeDataset] index " + std::to_string(index) +
          " out of range [0, " + std::to_string(num_records_) +
          ") left=" + detail::value_to_string(leftmost_key_value_) +
          " right=" + detail::value_to_string(rightmost_key_value_) +
          " step=" + detail::value_to_string(key_value_step_));
    }
    const auto target_key_value =
        leftmost_key_value_ +
        static_cast<typename Datatype_t::key_type_t>(index) * key_value_step_;
    return get_by_anchor_key(target_key_value);
  }

  torch::optional<std::size_t> size() const override { return num_records_; }

  /* Common samplers */
  torch::data::samplers::SequentialSampler SequentialSampler() const {
    return torch::data::samplers::SequentialSampler(num_records_);
  }
  torch::data::DataLoaderOptions
  SequentialSampler_options(std::size_t batch_size = 64,
                            std::size_t workers = 0) const {
    return torch::data::DataLoaderOptions()
        .batch_size(batch_size)
        .workers(workers);
  }
  torch::data::samplers::RandomSampler RandomSampler() const {
    return torch::data::samplers::RandomSampler(num_records_);
  }
  torch::data::DataLoaderOptions
  RandomSampler_options(std::size_t batch_size = 64,
                        std::size_t workers = 0) const {
    return torch::data::DataLoaderOptions()
        .batch_size(batch_size)
        .workers(workers);
  }

  /**
   * @brief Cheap construction-time probe for graph-edge assembly.
   *
   * This intentionally checks the valid anchor domain instead of materializing
   * tensors. Non-reference edges may still use the sequence synchronizer's
   * closest-terminal-key <= anchor behavior when get_by_anchor_key() is called.
   */
  [[nodiscard]] bool can_get_by_anchor_key(
      typename Datatype_t::key_type_t target_key_value) const {
    return num_records_ > 0 && leftmost_key_value_ <= target_key_value &&
           target_key_value <= rightmost_key_value_;
  }

  [[nodiscard]] bool
  can_get_by_anchor_key_strict(typename Datatype_t::key_type_t target_key_value,
                               std::string *error = nullptr) const {
    auto fail = [&](const std::string &reason) {
      if (error != nullptr) {
        *error = reason;
      }
      return false;
    };
    if (!can_get_by_anchor_key(target_key_value)) {
      return fail("anchor key outside edge dataset domain");
    }
    if (!(datasets_.size() == input_length_.size() &&
          datasets_.size() == future_length_.size())) {
      return fail("internal channel container sizes mismatch");
    }
    for (std::size_t i = 0; i < datasets_.size(); ++i) {
      std::string child_error{};
      if (!datasets_[i]->can_get_edge_sample_at_anchor_key(
              target_key_value, input_length_[i], future_length_[i],
              &child_error)) {
        std::ostringstream oss;
        oss << "channel=" << i;
        if (i < file_names_.size()) {
          oss << " file=" << file_names_[i];
        }
        if (!child_error.empty()) {
          oss << " reason=" << child_error;
        }
        return fail(oss.str());
      }
    }
    return true;
  }

  /**
   * @brief Compute edge-dataset anchor index range for keys within [key_left,
   * key_right]. Returns false when the range is empty.
   */
  bool
  compute_anchor_index_range_by_keys(typename Datatype_t::key_type_t key_left,
                                     typename Datatype_t::key_type_t key_right,
                                     std::size_t *out_begin_idx,
                                     std::size_t *out_count) const {
    using key_t = typename Datatype_t::key_type_t;
    auto set_empty = [&](bool ret = false) {
      if (out_begin_idx)
        *out_begin_idx = 0;
      if (out_count)
        *out_count = 0;
      return ret;
    };

    if (num_records_ == 0)
      return set_empty(false);
    if (key_right < key_left)
      std::swap(key_left, key_right);

    // Clamp to intersection domain first.
    if (key_right < leftmost_key_value_ || key_left > rightmost_key_value_) {
      return set_empty(false);
    }

    key_t left =
        key_left < leftmost_key_value_ ? leftmost_key_value_ : key_left;
    key_t right =
        key_right > rightmost_key_value_ ? rightmost_key_value_ : key_right;
    const key_t base = valid_left_[grid_ref_idx_];

    std::size_t begin_idx = 0;
    std::size_t count = 0;
    if constexpr (std::is_integral_v<key_t>) {
      const key_t left_aligned =
          detail::align_up_to_grid<key_t>(left, key_value_step_, base);
      if (!(left_aligned <= right))
        return set_empty(false);
      const key_t right_aligned =
          detail::align_down_to_grid<key_t>(right, key_value_step_, base);
      if (!(left_aligned <= right_aligned))
        return set_empty(false);
      count = detail::steps_between_inclusive<key_t>(
          left_aligned, right_aligned, key_value_step_);
      begin_idx = static_cast<std::size_t>(
          (left_aligned - leftmost_key_value_) / key_value_step_);
    } else {
      const key_t left_aligned =
          detail::align_up_to_grid_fp<key_t>(left, key_value_step_, base);
      if (detail::fp_less_with_tol<key_t>(right, left_aligned))
        return set_empty(false);
      const key_t right_aligned =
          detail::align_down_to_grid_fp<key_t>(right, key_value_step_, base);
      if (detail::fp_less_with_tol<key_t>(right_aligned, left_aligned))
        return set_empty(false);
      count = detail::steps_between_inclusive_fp<key_t>(
          left_aligned, right_aligned, key_value_step_);
      const double j0d = static_cast<double>(
          (left_aligned - leftmost_key_value_) / key_value_step_);
      begin_idx = static_cast<std::size_t>(std::llround(j0d));
    }

    if (begin_idx >= num_records_ || count == 0)
      return set_empty(false);
    if (count > num_records_ - begin_idx)
      count = num_records_ - begin_idx;
    if (count == 0)
      return set_empty(false);

    if (out_begin_idx)
      *out_begin_idx = begin_idx;
    if (out_count)
      *out_count = count;
    return true;
  }

  /**
   * @brief Retrieve stacked and padded edge sample by anchor key across
   * sources.
   * - Input is left-padded to max_input_length_.
   * - Future is right-padded to max_future_length_.
   * (C) stacks past_keys/future_keys across channels.
   */
  edge_sample_t
  get_by_anchor_key(typename Datatype_t::key_type_t target_key_value) {
    std::string availability_error{};
    if (!can_get_by_anchor_key_strict(target_key_value, &availability_error)) {
      throw_memory_mapped_error(
          {.where = "MemoryMappedEdgeDataset::get_by_anchor_key",
           .anchor_key = detail::value_to_string(target_key_value),
           .reason = availability_error.empty()
                         ? "anchor key outside edge dataset domain"
                         : availability_error});
    }

    const size_t C = datasets_.size();
    std::vector<torch::Tensor> feats(C), masks(C), fut_feats(C), fut_masks(C);
    std::vector<torch::Tensor> keys_past(C), keys_future(C);
    int64_t expected_D = -1;
    bool all_normalized = !datasets_.empty();

    // key tensor dtype to use for padding
    auto key_opts = torch::TensorOptions().dtype(
        std::is_integral_v<typename Datatype_t::key_type_t> ? torch::kInt64
                                                            : torch::kFloat64);

    for (size_t i = 0; i < C; ++i) {
      auto &d = datasets_[i];
      const auto np = input_length_[i];
      const auto nf = future_length_[i];
      all_normalized = all_normalized && d->is_normalized();

      edge_sample_t s;
      try {
        s = d->get_edge_sample_at_anchor_key(target_key_value, np, nf);
      } catch (const std::exception &ex) {
        throw_memory_mapped_error(
            {.where = "MemoryMappedEdgeDataset::get_by_anchor_key",
             .file = i < file_names_.size() ? file_names_[i] : std::string{},
             .channel = i,
             .anchor_key = detail::value_to_string(target_key_value),
             .reason = ex.what()});
      }
      const int64_t input_D = (s.features.defined() && s.features.dim() >= 2)
                                  ? s.features.size(1)
                                  : -1;
      const int64_t future_D =
          (s.future_features.defined() && s.future_features.dim() >= 2)
              ? s.future_features.size(1)
              : -1;
      if (expected_D < 0) {
        expected_D = (input_D >= 0) ? input_D : future_D;
      }
      if (input_D >= 0 && input_D != expected_D) {
        std::ostringstream reason;
        reason << "feature dimension mismatch across datasets: expected D="
               << expected_D << " got D=" << input_D;
        throw_memory_mapped_error(
            {.where = "MemoryMappedEdgeDataset::get_by_anchor_key",
             .file = i < file_names_.size() ? file_names_[i] : std::string{},
             .channel = i,
             .anchor_key = detail::value_to_string(target_key_value),
             .reason = reason.str()});
      }
      if (future_D >= 0 && future_D != expected_D) {
        std::ostringstream reason;
        reason << "future feature dimension mismatch across datasets: "
                  "expected D="
               << expected_D << " got D=" << future_D;
        throw_memory_mapped_error(
            {.where = "MemoryMappedEdgeDataset::get_by_anchor_key",
             .file = i < file_names_.size() ? file_names_[i] : std::string{},
             .channel = i,
             .anchor_key = detail::value_to_string(target_key_value),
             .reason = reason.str()});
      }

      // pad past at the front (so last row is time t)
      if (np < max_input_length_) {
        const int64_t pad = static_cast<int64_t>(max_input_length_ - np);
        feats[i] = torch::cat(
            {torch::zeros({pad, s.features.size(1)}, s.features.options()),
             s.features},
            0);
        masks[i] =
            torch::cat({torch::zeros({pad}, s.mask.options()), s.mask}, 0);
        keys_past[i] =
            torch::cat({torch::zeros({pad}, key_opts), s.past_keys}, 0);
      } else {
        feats[i] = s.features;
        masks[i] = s.mask;
        keys_past[i] = s.past_keys;
      }

      // pad future at the end (so first row is t+1)
      if (nf < max_future_length_) {
        const int64_t pad = static_cast<int64_t>(max_future_length_ - nf);
        fut_feats[i] = torch::cat(
            {s.future_features, torch::zeros({pad, s.future_features.size(1)},
                                             s.future_features.options())},
            0);
        fut_masks[i] = torch::cat(
            {s.future_mask, torch::zeros({pad}, s.future_mask.options())}, 0);
        keys_future[i] =
            torch::cat({s.future_keys, torch::zeros({pad}, key_opts)}, 0);
      } else {
        fut_feats[i] = s.future_features;
        fut_masks[i] = s.future_mask;
        keys_future[i] = s.future_keys;
      }
    }

    edge_sample_t out{
        torch::stack(feats, 0),     // [C, max_input_length, D]
        torch::stack(masks, 0),     // [C, max_input_length]
        torch::stack(fut_feats, 0), // [C, max_future_length, D]
        torch::stack(fut_masks, 0), // [C, max_future_length]
        torch::Tensor()             // encoding
    };
    // (C) keys
    out.past_keys = torch::stack(keys_past, 0);     // [C,max_input_length]
    out.future_keys = torch::stack(keys_future, 0); // [C,max_future_length]
    out.normalized = all_normalized;
    return out;
  }

  /**
   * ====================== (A) anchor-range slicing ========================
   * Return edge samples whose anchor key is within [key_left, key_right].
   * Uses the edge dataset grid (already regular).
   */
  std::vector<edge_sample_t>
  range_edge_samples_by_anchor_keys(typename Datatype_t::key_type_t key_left,
                                    typename Datatype_t::key_type_t key_right) {
    std::vector<edge_sample_t> out;
    std::size_t begin_idx = 0;
    std::size_t count = 0;
    if (!compute_anchor_index_range_by_keys(key_left, key_right, &begin_idx,
                                            &count))
      return out;
    out.reserve(count);
    for (std::size_t j = 0; j < count; ++j)
      out.emplace_back(get(begin_idx + j));
    return out;
  }

  /**
   * @brief Adds a dataset with per-source input/future lengths and updates the
   * intersection domain.
   */
  void add_dataset(const std::string csv_filename, std::size_t input_length,
                   std::size_t future_length,
                   std::string normalization_policy = "none",
                   bool force_rebuild_cache = false, size_t buffer_size = 1024,
                   char delimiter = ',',
                   const detail::csv_step_policy_t &csv_step_policy =
                       detail::csv_step_policy_t{}) {
    if (input_length == 0) {
      throw_memory_mapped_error(
          {.where = "MemoryMappedEdgeDataset::add_dataset",
           .csv_file = csv_filename,
           .reason = "input_length must be >= 1"});
    }

    /* --- prepare the file: CSV → binary --- */
    std::string bin_filename = sanitize_csv_into_binary_file<Datatype_t>(
        csv_filename, normalization_policy, force_rebuild_cache, buffer_size,
        delimiter, csv_step_policy);

    if (std::find(file_names_.begin(), file_names_.end(), bin_filename) !=
        file_names_.end()) {
      throw_memory_mapped_error(
          {.where = "MemoryMappedEdgeDataset::add_dataset",
           .file = bin_filename,
           .csv_file = csv_filename,
           .reason = "duplicated csv/bin file"});
    }

    auto dataset =
        std::make_shared<MemoryMappedDataset<Datatype_t>>(bin_filename);
    if (dataset->raw_records() < (input_length + future_length)) {
      std::ostringstream reason;
      reason << "dataset too small: rows=" << dataset->raw_records()
             << " input_length+future_length="
             << (input_length + future_length);
      throw_memory_mapped_error(
          {.where = "MemoryMappedEdgeDataset::add_dataset",
           .file = bin_filename,
           .csv_file = csv_filename,
           .reason = reason.str()});
    }

    file_names_.push_back(bin_filename);
    datasets_.push_back(std::move(dataset));
    input_length_.push_back(input_length);
    future_length_.push_back(future_length);

    /* --- extend valid range caches to keep them aligned with datasets_ --- */
    valid_left_.resize(datasets_.size());
    valid_right_.resize(datasets_.size());

    /* --- recompute global state after adding this dataset --- */
    try {
      recompute_global_state_();
    } catch (...) {
      file_names_.pop_back();
      datasets_.pop_back();
      input_length_.pop_back();
      future_length_.pop_back();
      valid_left_.resize(datasets_.size());
      valid_right_.resize(datasets_.size());
      if (!datasets_.empty()) {
        try {
          recompute_global_state_();
        } catch (...) {
        }
      } else {
        max_input_length_ = max_future_length_ = num_records_ = 0;
        leftmost_key_value_ = rightmost_key_value_ = key_value_span_ =
            key_value_step_ = {};
        grid_ref_idx_ = static_cast<std::size_t>(-1);
      }
      throw;
    }
  }

private:
  void recompute_global_state_() {
    using key_t = typename Datatype_t::key_type_t;

    const std::size_t channel_count = datasets_.size();
    if (channel_count == 0) {
      max_input_length_ = max_future_length_ = num_records_ = 0;
      return;
    }

    /* 1) maxima of (input_length, future_length) for padding */
    max_input_length_ =
        *std::max_element(input_length_.begin(), input_length_.end());
    max_future_length_ =
        *std::max_element(future_length_.begin(), future_length_.end());

    /* 2) compute per-dataset valid ranges in key space
          valid_left  = leftmost + (input_length-1) * step
          valid_right = rightmost -  future_length   * step     */
    key_t inter_left{};
    key_t inter_right{};
    for (std::size_t i = 0; i < channel_count; ++i) {
      auto &d = datasets_[i];
      const auto np = input_length_[i];
      const auto nf = future_length_[i];

      const key_t vleft =
          d->leftmost_key_value_ +
          static_cast<key_t>((np > 0 ? (np - 1) : 0)) * d->key_value_step_;
      const key_t vright =
          d->rightmost_key_value_ - static_cast<key_t>(nf) * d->key_value_step_;

      if (vright < vleft) {
        std::ostringstream reason;
        reason << "empty per-dataset valid range after "
                  "(input_length,future_length): input_length="
               << np << " future_length=" << nf
               << " valid_left=" << detail::value_to_string(vleft)
               << " valid_right=" << detail::value_to_string(vright);
        throw_memory_mapped_error(
            {.where = "MemoryMappedEdgeDataset::recompute_global_state",
             .file = i < file_names_.size() ? file_names_[i] : std::string{},
             .dataset_index = i,
             .reason = reason.str()});
      }

      valid_left_[i] = vleft;
      valid_right_[i] = vright;

      if (i == 0) {
        inter_left = vleft;
        inter_right = vright;
      } else {
        inter_left = std::max(inter_left, vleft);
        inter_right = std::min(inter_right, vright);
      }
    }

    if (inter_right < inter_left) {
      std::ostringstream reason;
      reason << "empty intersection across datasets after applying "
                "(input_length,future_length): intersection_left="
             << detail::value_to_string(inter_left)
             << " intersection_right=" << detail::value_to_string(inter_right);
      throw_memory_mapped_error(
          {.where = "MemoryMappedEdgeDataset::recompute_global_state",
           .reason = reason.str()});
    }

    /* 3) choose grid step and reference dataset per policy */
#if (CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY ==                                 \
     CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY_MIN)
    {
      std::size_t sel_idx = 0;
      key_t sel_step = datasets_[0]->key_value_step_;
      for (std::size_t i = 1; i < channel_count; ++i) {
        const key_t st = datasets_[i]->key_value_step_;
        if (st < sel_step) {
          sel_step = st;
          sel_idx = i;
        }
      }
      key_value_step_ = sel_step;
      grid_ref_idx_ = sel_idx;
    }
#elif (CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY ==                               \
       CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY_MAX)
    {
      std::size_t sel_idx = 0;
      key_t sel_step = datasets_[0]->key_value_step_;
      for (std::size_t i = 1; i < channel_count; ++i) {
        const key_t st = datasets_[i]->key_value_step_;
        if (st > sel_step) {
          sel_step = st;
          sel_idx = i;
        }
      }
      key_value_step_ = sel_step;
      grid_ref_idx_ = sel_idx;
    }
#else
#error "Unknown CUWACUNU_EDGE_DATASET_GRID_STEP_POLICY"
#endif

    /* 4) align the intersection [inter_left, inter_right] to the chosen grid.
          Use the reference dataset's valid_left as the base for congruence. */
    const key_t base = valid_left_[grid_ref_idx_];

    if constexpr (std::is_integral_v<key_t>) {
      leftmost_key_value_ =
          detail::align_up_to_grid<key_t>(inter_left, key_value_step_, base);
      if (!(leftmost_key_value_ <= inter_right)) {
        std::ostringstream reason;
        reason << "aligned left bound exceeds intersection right bound: "
               << "aligned_left="
               << detail::value_to_string(leftmost_key_value_)
               << " intersection_right=" << detail::value_to_string(inter_right)
               << " step=" << detail::value_to_string(key_value_step_)
               << " grid_ref_idx=" << grid_ref_idx_;
        throw_memory_mapped_error(
            {.where = "MemoryMappedEdgeDataset::recompute_global_state",
             .reason = reason.str()});
      }
      rightmost_key_value_ =
          detail::align_down_to_grid<key_t>(inter_right, key_value_step_, base);

      if (!(leftmost_key_value_ <= rightmost_key_value_)) {
        std::ostringstream reason;
        reason << "empty grid after alignment (integral keys): left="
               << detail::value_to_string(leftmost_key_value_)
               << " right=" << detail::value_to_string(rightmost_key_value_)
               << " step=" << detail::value_to_string(key_value_step_)
               << " grid_ref_idx=" << grid_ref_idx_;
        throw_memory_mapped_error(
            {.where = "MemoryMappedEdgeDataset::recompute_global_state",
             .reason = reason.str()});
      }

      num_records_ = detail::steps_between_inclusive<key_t>(
          leftmost_key_value_, rightmost_key_value_, key_value_step_);
      key_value_span_ = rightmost_key_value_ - leftmost_key_value_;
    } else {
      leftmost_key_value_ =
          detail::align_up_to_grid_fp<key_t>(inter_left, key_value_step_, base);
      if (!(leftmost_key_value_ <= inter_right)) {
        std::ostringstream reason;
        reason << "aligned left bound exceeds intersection right bound "
                  "(float): aligned_left="
               << detail::value_to_string(leftmost_key_value_)
               << " intersection_right=" << detail::value_to_string(inter_right)
               << " step=" << detail::value_to_string(key_value_step_)
               << " grid_ref_idx=" << grid_ref_idx_;
        throw_memory_mapped_error(
            {.where = "MemoryMappedEdgeDataset::recompute_global_state",
             .reason = reason.str()});
      }
      rightmost_key_value_ = detail::align_down_to_grid_fp<key_t>(
          inter_right, key_value_step_, base);

      if (!(leftmost_key_value_ <= rightmost_key_value_)) {
        std::ostringstream reason;
        reason << "empty grid after alignment (floating keys): left="
               << detail::value_to_string(leftmost_key_value_)
               << " right=" << detail::value_to_string(rightmost_key_value_)
               << " step=" << detail::value_to_string(key_value_step_)
               << " grid_ref_idx=" << grid_ref_idx_;
        throw_memory_mapped_error(
            {.where = "MemoryMappedEdgeDataset::recompute_global_state",
             .reason = reason.str()});
      }

      num_records_ = detail::steps_between_inclusive_fp<key_t>(
          leftmost_key_value_, rightmost_key_value_, key_value_step_);
      key_value_span_ = rightmost_key_value_ - leftmost_key_value_;
    }

    if (num_records_ == 0) {
      throw_memory_mapped_error(
          {.where = "MemoryMappedEdgeDataset::recompute_global_state",
           .reason = "no records after alignment to global grid"});
    }
  }
};

template <typename Datatype_t>
MemoryMappedEdgeDataset<Datatype_t> create_memory_mapped_edge_dataset(
    const cuwacunu::ujcamei::source::instrument_signature_t &edge_signature,
    const source_materialization_request_t &request,
    bool force_rebuild_cache = false) {

  char delimiter = ',';
  size_t buffer_size = 1024;
  detail::csv_step_policy_t csv_step_policy{};
  csv_step_policy.bootstrap_deltas =
      std::max<std::size_t>(2, request.csv_bootstrap_deltas);
  csv_step_policy.abs_tol =
      (request.csv_step_abs_tol > 0.0L) ? request.csv_step_abs_tol : 1e-8L;
  csv_step_policy.rel_tol =
      (request.csv_step_rel_tol >= 0.0L) ? request.csv_step_rel_tol : 1e-10L;

  MemoryMappedEdgeDataset<Datatype_t> edge_dataset;
  const std::string expected_record_type =
      detail::record_type_name_for_datatype<Datatype_t>();
  if (expected_record_type.empty()) {
    throw_memory_mapped_error(
        {.where = "create_memory_mapped_edge_dataset",
         .reason = "unsupported Datatype_t for source spec record_type "
                   "matching"});
  }
  std::string signature_error{};
  if (!cuwacunu::ujcamei::source::instrument_signature_validate(
          edge_signature, /*allow_any=*/false, "edge instrument",
          &signature_error)) {
    throw_memory_mapped_error(
        {.where = "create_memory_mapped_edge_dataset",
         .edge_signature =
             cuwacunu::ujcamei::source::instrument_signature_compact_string(
                 edge_signature),
         .reason = "invalid edge instrument signature: " + signature_error});
  }
  if (edge_signature.record_type != expected_record_type) {
    throw_memory_mapped_error(
        {.where = "create_memory_mapped_edge_dataset",
         .edge_signature =
             cuwacunu::ujcamei::source::instrument_signature_compact_string(
                 edge_signature),
         .reason =
             "edge instrument record_type=" + edge_signature.record_type +
             " does not match Datatype_t record_type=" + expected_record_type});
  }

  std::size_t matched_sources = 0;
  const std::string edge_signature_label =
      cuwacunu::ujcamei::source::instrument_signature_compact_string(
          edge_signature);

  for (const auto &in_form : request.channel_forms) {
    if (in_form.active == "true") {
      if (in_form.record_type != expected_record_type) {
        log_warn("[create_memory_mapped_edge_dataset] Skipping active "
                 "channel_form with record_type=%s for Datatype_t=%s\n",
                 in_form.record_type.c_str(), expected_record_type.c_str());
        continue;
      }
      const auto matching_sources = filter_source_materialization_forms(
          request, edge_signature, in_form.interval);
      if (matching_sources.empty()) {
        throw_memory_mapped_error(
            {.where = "create_memory_mapped_edge_dataset",
             .edge_signature = edge_signature_label,
             .reason = "no source row matched edge instrument for interval=" +
                       cuwacunu::ujcamei::source::types::enum_to_string(
                           in_form.interval)});
      }
      for (const auto &instr_form : matching_sources) {

        const std::string channel_label =
            "interval=" +
            cuwacunu::ujcamei::source::types::enum_to_string(in_form.interval) +
            " record_type=" + in_form.record_type;
        const std::size_t input_length = detail::parse_required_size(
            in_form.input_length, "input_length", channel_label);
        const std::size_t future_length = detail::parse_required_size(
            in_form.future_length, "future_length", channel_label);
        const std::string normalization_policy =
            in_form.normalization_policy.empty() ? std::string("none")
                                                 : in_form.normalization_policy;
        if (input_length == 0) {
          throw_memory_mapped_error(
              {.where = "create_memory_mapped_edge_dataset",
               .edge_signature = edge_signature_label,
               .reason = "input_length must be >= 1 for " + channel_label});
        }
        if (!detail::try_parse_normalization_policy(normalization_policy)
                 .has_value()) {
          throw_memory_mapped_error(
              {.where = "create_memory_mapped_edge_dataset",
               .csv_file = instr_form.source,
               .edge_signature = edge_signature_label,
               .reason = "unsupported normalization policy '" +
                         normalization_policy + "' for " + channel_label});
        }

        edge_dataset.add_dataset(
            /* csv file */ instr_form.source,
            /* input_length */ input_length,
            /* future_length */ future_length,
            /* normalization_policy */ normalization_policy,
            /* force_rebuild_cache */ force_rebuild_cache,
            /* buffer_size */ buffer_size,
            /* delimiter */ delimiter,
            /* csv_step_policy */ csv_step_policy);
        ++matched_sources;
      }
    }
  }

  if (matched_sources == 0) {
    throw_memory_mapped_error(
        {.where = "create_memory_mapped_edge_dataset",
         .edge_signature = edge_signature_label,
         .reason = "no datasets matched edge instrument and Datatype_t "
                   "record_type=" +
                   std::string(expected_record_type)});
  }

  return edge_dataset;
}

/**
 * @brief Compatibility overload for older callers that still carry a merged
 * source_spec_t. New code should pass source_materialization_request_t.
 */
template <typename Datatype_t>
MemoryMappedEdgeDataset<Datatype_t> create_memory_mapped_edge_dataset(
    const cuwacunu::ujcamei::source::instrument_signature_t &edge_signature,
    const cuwacunu::ujcamei::source::contract::source_spec_t &source_spec,
    bool force_rebuild_cache = false) {
  return create_memory_mapped_edge_dataset<Datatype_t>(
      edge_signature, make_source_materialization_request(source_spec),
      force_rebuild_cache);
}

} /* namespace memory_mapped */
} /* namespace storage */
} /* namespace source */
} /* namespace ujcamei */
} /* namespace cuwacunu */
