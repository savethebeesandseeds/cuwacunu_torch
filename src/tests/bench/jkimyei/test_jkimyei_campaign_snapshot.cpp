#include <cassert>
#include <filesystem>
#include <string>

#include "hero/jkimyei_hero/jkimyei_campaign_runtime.h"

int main() {
  namespace fs = std::filesystem;

  const fs::path campaigns_root =
      fs::temp_directory_path() / "jkimyei_campaign_snapshot_smoke";
  std::error_code ec{};
  fs::remove_all(campaigns_root, ec);
  ec.clear();

  const std::string campaign_cursor =
      cuwacunu::hero::jkimyei::make_campaign_cursor(campaigns_root);
  std::string source_campaign_path{};
  std::string snapshot_path{};
  std::string error{};

  bool ok = cuwacunu::hero::jkimyei::prepare_campaign_dsl_snapshot(
      campaigns_root,
      campaign_cursor,
      "/cuwacunu/src/config",
      &source_campaign_path,
      &snapshot_path,
      &error);
  assert(ok);
  assert(error.empty());
  assert(!source_campaign_path.empty());
  assert(!snapshot_path.empty());
  assert(fs::exists(snapshot_path));

  std::string snapshot_text{};
  ok = cuwacunu::hero::runtime::read_text_file(snapshot_path,
                                               &snapshot_text,
                                               &error);
  assert(ok);
  assert(error.empty());
  assert(snapshot_text.find(
             "/cuwacunu/src/config/instructions/default.iitepi.contract.dsl") !=
         std::string::npos);
  assert(snapshot_text.find(
             "/cuwacunu/src/config/instructions/default.iitepi.wave.dsl") !=
         std::string::npos);

  fs::remove_all(campaigns_root, ec);
  return 0;
}
