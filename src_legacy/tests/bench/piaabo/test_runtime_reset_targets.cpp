#include <algorithm>
#include <cassert>
#include <filesystem>
#include <string>
#include <vector>

#include "hero/runtime_dev_loop.h"

namespace {

bool has_path(const std::vector<std::filesystem::path>& paths,
              const std::filesystem::path& expected) {
  const std::filesystem::path normalized = expected.lexically_normal();
  return std::find(paths.begin(), paths.end(), normalized) != paths.end();
}

}  // namespace

int main() {
  using cuwacunu::hero::runtime_dev::resolve_runtime_reset_targets_from_global_config;
  using cuwacunu::hero::runtime_dev::runtime_reset_targets_t;
  using cuwacunu::hero::runtime_dev::strip_control_plane_store_roots;

  runtime_reset_targets_t targets{};
  std::string error{};
  assert(resolve_runtime_reset_targets_from_global_config(
      "/cuwacunu/src/config/.config", &targets, &error));
  assert(error.empty());

  const std::filesystem::path hashimyei_root =
      cuwacunu::hero::runtime_dev::detail::derive_hashimyei_store_root(
          targets.runtime_root);

  assert(has_path(targets.store_roots, hashimyei_root));
  assert(has_path(targets.store_roots, targets.runtime_campaigns_root));
  assert(has_path(targets.store_roots, targets.marshal_root));
  assert(has_path(targets.store_roots, targets.human_root));

  assert(strip_control_plane_store_roots(&targets));

  assert(has_path(targets.store_roots, hashimyei_root));
  assert(has_path(targets.store_roots, targets.runtime_campaigns_root));
  assert(!has_path(targets.store_roots, targets.marshal_root));
  assert(!has_path(targets.store_roots, targets.human_root));

  return 0;
}
