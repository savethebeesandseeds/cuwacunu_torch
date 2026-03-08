#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "HERO/hashimyei/hashimyei_catalog.h"
#include "hashimyei/hashimyei_artifacts.h"
#include "iitepi/config_space_t.h"

namespace {

constexpr const char* kDefaultPassEnv = "CUWACUNU_HASHIMYEI_META_SECRET";

[[nodiscard]] std::string read_optional_config(const char* section, const char* key) {
  try {
    return cuwacunu::iitepi::config_space_t::get<std::string>(section, key, std::string{});
  } catch (...) {
    return {};
  }
}

void print_help(const char* argv0) {
  std::cout << "Usage: " << argv0 << " [options]\n"
            << "Options:\n"
            << "  --store-root <path>      Hashimyei root (default: env/config/fallback policy)\n"
            << "  --catalog <path>         Catalog DB path (default: <store-root>/catalog/hashimyei_catalog.idydb)\n"
            << "  --passphrase <value>     Encryption passphrase override\n"
            << "  --passphrase-env <name>  Read passphrase from env var (default: CUWACUNU_HASHIMYEI_META_SECRET)\n"
            << "  --unencrypted            Open plaintext catalog (for local debugging only)\n"
            << "  --no-backfill            Disable legacy run-id backfill\n"
            << "  --help                   Show this help\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::filesystem::path store_root = cuwacunu::hashimyei::store_root();

  std::filesystem::path catalog_path{};
  std::string passphrase{};
  std::string passphrase_env = kDefaultPassEnv;
  bool encrypted = true;
  bool backfill_legacy = true;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--store-root" && i + 1 < argc) {
      store_root = argv[++i];
      continue;
    }
    if (arg == "--catalog" && i + 1 < argc) {
      catalog_path = argv[++i];
      continue;
    }
    if (arg == "--passphrase" && i + 1 < argc) {
      passphrase = argv[++i];
      continue;
    }
    if (arg == "--passphrase-env" && i + 1 < argc) {
      passphrase_env = argv[++i];
      continue;
    }
    if (arg == "--no-backfill") {
      backfill_legacy = false;
      continue;
    }
    if (arg == "--unencrypted") {
      encrypted = false;
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      print_help(argv[0]);
      return 0;
    }

    std::cerr << "Unknown argument: " << arg << "\n";
    print_help(argv[0]);
    return 2;
  }

  if (catalog_path.empty()) {
    catalog_path = cuwacunu::hashimyei::catalog_db_path(store_root);
  }

  if (passphrase.empty()) {
    if (!passphrase_env.empty()) {
      if (const char* env = std::getenv(passphrase_env.c_str()); env && env[0] != '\0') {
        passphrase = env;
      }
    }
  }
  if (passphrase.empty()) {
    passphrase = read_optional_config("GENERAL", "hashimyei_metadata_secret");
  }

  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t::options_t options{};
  options.catalog_path = catalog_path;
  options.encrypted = encrypted;
  options.passphrase = passphrase;

  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t catalog{};
  std::string error;
  if (!catalog.open(options, &error)) {
    std::cerr << "error: open catalog failed: " << error << "\n";
    return 1;
  }

  if (!catalog.ingest_filesystem(store_root, backfill_legacy, &error)) {
    std::cerr << "error: ingest failed: " << error << "\n";
    return 1;
  }

  std::vector<cuwacunu::hero::hashimyei::run_manifest_t> runs{};
  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> artifacts{};
  if (!catalog.list_runs_by_binding("", "", "", &runs, &error)) {
    std::cerr << "error: run index read failed: " << error << "\n";
    return 1;
  }
  if (!catalog.list_artifacts("", "", 0, 0, true, &artifacts, &error)) {
    std::cerr << "error: artifact index read failed: " << error << "\n";
    return 1;
  }

  std::cout << "ok\n"
            << "store_root=" << store_root.string() << "\n"
            << "catalog_path=" << catalog_path.string() << "\n"
            << "encrypted=" << (encrypted ? "true" : "false") << "\n"
            << "run_count=" << runs.size() << "\n"
            << "artifact_count=" << artifacts.size() << "\n";
  return 0;
}
