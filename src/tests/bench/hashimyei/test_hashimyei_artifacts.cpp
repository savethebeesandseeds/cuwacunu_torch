#include "hero/hashimyei_hero/hashimyei_artifacts.h"
#include "hero/hashimyei_hero/hashimyei_driver.h"
#include "hero/hashimyei_hero/hashimyei_identity.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;

static void require_impl(bool ok, const char* expr, const char* file, int line) {
  if (!ok) {
    std::cerr << "[TEST FAIL] " << file << ":" << line << "  (" << expr << ")\n";
    std::exit(1);
  }
}
#define REQUIRE(x) require_impl((x), #x, __FILE__, __LINE__)

static std::string random_suffix() {
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dist;
  const uint64_t v = dist(gen);
  return std::to_string(static_cast<unsigned long long>(getpid())) + "_" +
         std::to_string(static_cast<unsigned long long>(v));
}

struct temp_dir_t {
  fs::path dir{};
  temp_dir_t() {
    dir = fs::temp_directory_path() / ("test_hashimyei_artifacts_" + random_suffix());
    fs::create_directories(dir);
  }
  ~temp_dir_t() {
    std::error_code ec;
    fs::remove_all(dir, ec);
  }
};

struct env_guard_t {
  std::string key{};
  std::optional<std::string> previous{};

  explicit env_guard_t(std::string key_in) : key(std::move(key_in)) {
    const char* current = std::getenv(key.c_str());
    if (current != nullptr) previous = std::string(current);
  }

  ~env_guard_t() {
    if (previous.has_value()) {
      (void)setenv(key.c_str(), previous->c_str(), 1);
    } else {
      (void)unsetenv(key.c_str());
    }
  }

  void set(std::string_view value) {
    REQUIRE(setenv(key.c_str(), std::string(value).c_str(), 1) == 0);
  }

  void unset() {
    REQUIRE(unsetenv(key.c_str()) == 0);
  }
};

static void write_text_file(const fs::path& path, const std::string& payload) {
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  REQUIRE(static_cast<bool>(out));
  out << payload;
  REQUIRE(static_cast<bool>(out));
}

static std::string read_text_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  REQUIRE(static_cast<bool>(in));
  std::string out((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  return out;
}

static void test_manifest_roundtrip(const fs::path& store_root) {
  const fs::path artifact_dir =
      store_root / "tsi.wikimyei" / "representation" / "vicreg" / "0x00ab";

  cuwacunu::hashimyei::artifact_manifest_t manifest{};
  manifest.canonical_type = "tsi.wikimyei.representation.vicreg";
  manifest.family = "representation";
  manifest.model = "vicreg";
  manifest.artifact_id = "0x00ab";
  manifest.files.push_back({"weights.init.pt", 100});
  manifest.files.push_back({"metadata.enc", 7});
  manifest.files.push_back({"weights.init.pt", 88});
  manifest.files.push_back({"weights.init.network_analytics.lls", 25});

  std::string error;
  REQUIRE(cuwacunu::hashimyei::write_artifact_manifest(artifact_dir, manifest, &error));
  REQUIRE(cuwacunu::hashimyei::artifact_manifest_exists(artifact_dir));

  const fs::path manifest_path = cuwacunu::hashimyei::artifact_manifest_path(artifact_dir);
  REQUIRE(manifest_path.filename() == "manifest.v2.kv");

  const std::string payload = read_text_file(manifest_path);
  REQUIRE(payload.find("schema=hashimyei.artifact.manifest.v2\n") != std::string::npos);
  REQUIRE(payload.find("file_count=3\n") != std::string::npos);
  REQUIRE(payload.find("file_0000_path=metadata.enc\n") != std::string::npos);
  REQUIRE(payload.find("file_0001_path=weights.init.network_analytics.lls\n") != std::string::npos);
  REQUIRE(payload.find("file_0002_path=weights.init.pt\n") != std::string::npos);
  REQUIRE(payload.find("file_0002_size=100\n") != std::string::npos);

  cuwacunu::hashimyei::artifact_manifest_t parsed{};
  REQUIRE(cuwacunu::hashimyei::read_artifact_manifest(artifact_dir, &parsed, &error));
  REQUIRE(parsed.schema == "hashimyei.artifact.manifest.v2");
  REQUIRE(parsed.canonical_type == manifest.canonical_type);
  REQUIRE(parsed.family == manifest.family);
  REQUIRE(parsed.model == manifest.model);
  REQUIRE(parsed.artifact_id == manifest.artifact_id);
  REQUIRE(parsed.files.size() == 3);
  REQUIRE(parsed.files[0].path == "metadata.enc");
  REQUIRE(parsed.files[1].path == "weights.init.network_analytics.lls");
  REQUIRE(parsed.files[2].path == "weights.init.pt");
  REQUIRE(parsed.files[2].size == 100);
  REQUIRE(cuwacunu::hashimyei::artifact_manifest_has_file(parsed, "weights.init.pt"));
}

static void test_store_root_and_catalog_defaults(const fs::path& store_root) {
  env_guard_t root_env("CUWACUNU_HASHIMYEI_STORE_ROOT");
  env_guard_t secret_env("CUWACUNU_HASHIMYEI_META_SECRET");

  root_env.set(store_root.string());
  secret_env.set("hashimyei-secret-test");

  REQUIRE(cuwacunu::hashimyei::store_root() == store_root);
  REQUIRE(cuwacunu::hashimyei::catalog_db_path() ==
          (store_root / "catalog" / "hashimyei_catalog.idydb"));
  REQUIRE(cuwacunu::hashimyei::catalog_db_path(store_root) ==
          (store_root / "catalog" / "hashimyei_catalog.idydb"));
  REQUIRE(cuwacunu::hashimyei::metadata_secret() == "hashimyei-secret-test");

  secret_env.unset();
  (void)cuwacunu::hashimyei::metadata_secret();
}

static void test_discovery_rules(const fs::path& store_root) {
  env_guard_t root_env("CUWACUNU_HASHIMYEI_STORE_ROOT");
  root_env.set(store_root.string());

  const fs::path base =
      store_root / "tsi.wikimyei" / "representation" / "vicreg";
  std::error_code ec;
  fs::create_directories(base, ec);

  const auto create_manifest = [&](const fs::path& artifact_dir, std::string_view hash) {
    cuwacunu::hashimyei::artifact_manifest_t manifest{};
    manifest.canonical_type = "tsi.wikimyei.representation.vicreg";
    manifest.family = "representation";
    manifest.model = "vicreg";
    manifest.artifact_id = std::string(hash);
    std::string error;
    REQUIRE(cuwacunu::hashimyei::write_artifact_manifest(artifact_dir, manifest, &error));
  };

  const fs::path manifest_only = base / "0x00aa";
  fs::create_directories(manifest_only, ec);
  create_manifest(manifest_only, "0x00aa");

  const fs::path weights_only = base / "0x00ab";
  fs::create_directories(weights_only, ec);
  write_text_file(weights_only / "weights.init.pt", "w");

  const fs::path manifest_and_weights = base / "0x00ac";
  fs::create_directories(manifest_and_weights, ec);
  create_manifest(manifest_and_weights, "0x00ac");
  write_text_file(manifest_and_weights / "weights.init.pt", "w2");

  const fs::path invalid_atom = base / "legacy_name";
  fs::create_directories(invalid_atom, ec);
  write_text_file(invalid_atom / "weights.init.pt", "ignored");

  const fs::path invalid_hex = base / "0xzz12";
  fs::create_directories(invalid_hex, ec);
  write_text_file(invalid_hex / "weights.init.pt", "ignored");

  const auto discovered =
      cuwacunu::hashimyei::discover_created_artifacts_for("representation", "vicreg");
  REQUIRE(discovered.size() == 3);
  REQUIRE(discovered[0].hashimyei == "0x00aa");
  REQUIRE(discovered[1].hashimyei == "0x00ab");
  REQUIRE(discovered[2].hashimyei == "0x00ac");

  REQUIRE(discovered[0].weight_files.empty());
  REQUIRE(discovered[1].weight_files.size() == 1);
  REQUIRE(discovered[2].weight_files.size() == 1);
  REQUIRE(discovered[0].canonical_base ==
          "tsi.wikimyei.representation.vicreg.0x00aa");
}

static void test_driver_registry_ordering() {
  const std::string suffix = random_suffix();
  const std::string a = "tsi.test.driver.a." + suffix;
  const std::string b = "tsi.test.driver.b." + suffix;
  const std::string c = "tsi.test.driver.c." + suffix;

  const auto cb = [](const cuwacunu::hashimyei::artifact_action_context_t&,
                     std::string*) { return true; };
  std::string error;

  REQUIRE(cuwacunu::hashimyei::register_artifact_driver(
      {.canonical_type = c, .save = cb}, &error));
  REQUIRE(cuwacunu::hashimyei::register_artifact_driver(
      {.canonical_type = a, .save = cb}, &error));
  REQUIRE(cuwacunu::hashimyei::register_artifact_driver(
      {.canonical_type = b, .save = cb}, &error));

  const auto types = cuwacunu::hashimyei::registered_artifact_driver_types();
  REQUIRE(std::is_sorted(types.begin(), types.end()));
  REQUIRE(std::find(types.begin(), types.end(), a) != types.end());
  REQUIRE(std::find(types.begin(), types.end(), b) != types.end());
  REQUIRE(std::find(types.begin(), types.end(), c) != types.end());
}

static void test_identity_helper_regressions() {
  using cuwacunu::hashimyei::hashimyei_kind_e;
  using cuwacunu::hashimyei::hashimyei_t;

  std::uint64_t ordinal = 0;
  REQUIRE(cuwacunu::hashimyei::parse_hex_hash_name_ordinal("0x000f", &ordinal));
  REQUIRE(ordinal == 15);

  hashimyei_t ok = cuwacunu::hashimyei::make_identity(
      hashimyei_kind_e::WAVE, 9, std::string(64, 'a'));
  std::string error;
  REQUIRE(cuwacunu::hashimyei::validate_hashimyei(ok, &error));

  hashimyei_t bad = ok;
  bad.hash_sha256_hex.clear();
  REQUIRE(!cuwacunu::hashimyei::validate_hashimyei(bad, &error));
}

int main() {
  temp_dir_t tmp{};
  const fs::path store_root = tmp.dir / ".hashimyei";

  test_manifest_roundtrip(store_root);
  test_store_root_and_catalog_defaults(store_root);
  test_discovery_rules(store_root);
  test_driver_registry_ordering();
  test_identity_helper_regressions();

  std::cout << "[PASS] test_hashimyei_artifacts\n";
  return 0;
}
