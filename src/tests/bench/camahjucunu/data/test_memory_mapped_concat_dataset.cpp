/* test_memory_mapped_concat_dataset.cpp */
#include <torch/torch.h>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <vector>
#include <string>

#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dfiles.h"

#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

using cuwacunu::camahjucunu::data::MemoryMappedConcatDataset;

/* helper for float/integer “alignment” to a grid base + n*step */
template <typename K>
static inline bool aligned_to_grid(K x, K base, K step, K tol = static_cast<K>(1e-9)) {
  if constexpr (std::is_floating_point_v<K>) {
    if (step == K(0)) return true;
    const K q = (x - base) / step;
    return std::abs(q - std::round(q)) <= tol;
  } else {
    if (step == K(0)) return true;
    auto diff = x - base;
    return (diff % step) == 0;
  }
}

/* size recomputation consistent with concat’s implementation */
template <typename K>
static inline std::size_t expected_count(K left, K right, K step, K tol = static_cast<K>(1e-9)) {
  if constexpr (std::is_floating_point_v<K>) {
    if (step <= K(0) || right + tol < left) return 0;
    K k = std::floor(((right - left) / step) + tol);
    if (k < 0) return 0;
    return static_cast<std::size_t>(k) + 1;
  } else {
    if (step <= 0 || right < left) return 0;
    return static_cast<std::size_t>((right - left) / step) + 1;
  }
}

int main() {
  /* read the config */
  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  /* choose instrument / instruction */
  // using T = cuwacunu::camahjucunu::exchange::kline_t;
  using T = cuwacunu::camahjucunu::exchange::basic_t;  // keep as in your example
  std::string INSTRUMENT = "UTILITIES";
  std::string instruction = cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction();

  /* create the observation pipeline and concat dataset */
  auto obsPipe = cuwacunu::camahjucunu::BNF::observationPipeline();
  cuwacunu::camahjucunu::observation_instruction_t decoded = obsPipe.decode(instruction);

  MemoryMappedConcatDataset<T> cds =
      cuwacunu::camahjucunu::data::create_memory_mapped_concat_dataset<T>(
          INSTRUMENT, decoded, /*force_binarization*/ false);

  /* sanity props */
  assert(cds.size().has_value());
  const std::size_t N = cds.size().value();
  assert(cds.max_N_past_   > 0);
  assert(cds.max_N_future_ > 0);

  using key_t = typename T::key_type_t;
  const key_t left  = cds.leftmost_key_value_;
  const key_t right = cds.rightmost_key_value_;
  const key_t step  = cds.key_value_step_;
  assert(step > 0);
  assert(left <= right);

  /* size formula matches */
  const std::size_t Nexp = expected_count(left, right, step);
  assert(N == Nexp && "cds.size must equal (right-left)/step + 1");

  /* build targets: on-grid and outside */
  std::vector<key_t> targets;
  targets.push_back(left);
  if (N >= 3) targets.push_back(static_cast<key_t>(left + (N/2)*step));
  targets.push_back(right);
  // outside both ends
  targets.push_back(static_cast<key_t>(left  - step));
  targets.push_back(static_cast<key_t>(right + step));

  /* --- get_by_key_value smoke / shapes / exceptions --- */
  log_info("-- -- -- get_by_key_value -- -- --\n");
  for (auto target_key : targets) {
    bool ok = true;
    cuwacunu::camahjucunu::data::observation_sample_t s;
    try {
      TICK(GET_BY_KEY_);
      s = cds.get_by_key_value(target_key);
      PRINT_TOCK_ns(GET_BY_KEY_);
    } catch (const std::exception& e) {
      ok = false;
      std::cout << "get_by_key_value threw for key=" << std::setprecision(17) << target_key
                << " (" << e.what() << ")\n";
    }

    const bool inside = (target_key >= left && target_key <= right);
    if (inside) {
      // In our implementation, concat uses exact anchors for indexing but
      // still serves arbitrary keys via per-dataset closest<=target.
      // So inside the range we expect success.
      assert(ok && "Expected success for in-range key");
      assert(s.features.dim()        == 3);
      assert(s.future_features.dim() == 3);
      assert(s.mask.dim()            == 2);
      assert(s.future_mask.dim()     == 2);
      assert(s.features.size(1)      == (long)cds.max_N_past_);
      assert(s.future_features.size(1)== (long)cds.max_N_future_);
      assert(s.mask.size(1)          == (long)cds.max_N_past_);
      assert(s.future_mask.size(1)   == (long)cds.max_N_future_);
    } else {
      assert(!ok && "Out-of-range key should throw");
    }
  }

  /* --- index/key equivalence: get(i) == get_by_key_value(left + i*step) --- */
  log_info("-- -- -- index/key equivalence -- -- --\n");
  if (N > 0) {
    std::vector<std::size_t> idxs{0};
    if (N >= 3) idxs.push_back(N/2);
    if (N > 1)  idxs.push_back(N-1);

    for (auto i : idxs) {
      const key_t key_i = static_cast<key_t>(left + i * step);

      TICK(GET_BY_INDEX_);
      auto a = cds.get(i);
      PRINT_TOCK_ns(GET_BY_INDEX_);

      TICK(GET_BY_KEY_EQ_);
      auto b = cds.get_by_key_value(key_i);
      PRINT_TOCK_ns(GET_BY_KEY_EQ_);

      // shapes
      assert(a.features.sizes()        == b.features.sizes());
      assert(a.future_features.sizes() == b.future_features.sizes());
      assert(a.mask.sizes()            == b.mask.sizes());
      assert(a.future_mask.sizes()     == b.future_mask.sizes());

      // content checks: last past row (time t) & first future row (t+1)
      for (int src = 0; src < a.features.size(0); ++src) {
        auto a_p = a.features.index({src, a.features.size(1)-1}).to(torch::kFloat32);
        auto b_p = b.features.index({src, b.features.size(1)-1}).to(torch::kFloat32);
        auto a_f = a.future_features.index({src, 0}).to(torch::kFloat32);
        auto b_f = b.future_features.index({src, 0}).to(torch::kFloat32);
        assert(torch::allclose(a_p, b_p, 1e-5, 1e-5));
        assert(torch::allclose(a_f, b_f, 1e-5, 1e-5));
      }
    }
  }

  /* --- “aligned on grid” sanity (no modulo for doubles) --- */
  log_info("-- -- -- on-grid alignment sanity -- -- --\n");
  {
    // pick three anchors strictly on the grid
    std::vector<key_t> on_grid;
    on_grid.push_back(left);
    if (N >= 3) on_grid.push_back(static_cast<key_t>(left + (N/2)*step));
    on_grid.push_back(right);

    for (auto k : on_grid) {
      assert(aligned_to_grid<key_t>(k, left, step) && "constructed keys must be on grid");
    }
  }

  std::cout << "[OK] memory_mapped_concat_dataset tests passed.\n";
  return 0;
}
