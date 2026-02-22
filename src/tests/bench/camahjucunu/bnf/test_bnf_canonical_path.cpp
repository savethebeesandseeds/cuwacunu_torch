// test_bnf_canonical_path.cpp
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/implementations/canonical_path/canonical_path.h"

namespace {

using cuwacunu::camahjucunu::canonical_path_kind_e;

bool expect_ok(const std::string& raw,
               canonical_path_kind_e expected_kind,
               const std::string& expected_canonical,
               std::set<std::string>* hashes,
               cuwacunu::camahjucunu::canonical_path_t* out_decoded = nullptr) {
  auto decoded = cuwacunu::camahjucunu::decode_canonical_path(raw);
  if (!decoded.ok) {
    std::cerr << "[FAIL] expected ok for: " << raw
              << " error=\"" << decoded.error << "\"\n";
    return false;
  }
  if (decoded.path_kind != expected_kind) {
    std::cerr << "[FAIL] kind mismatch for: " << raw << "\n";
    return false;
  }
  if (!expected_canonical.empty() && decoded.canonical != expected_canonical) {
    std::cerr << "[FAIL] canonical mismatch for: " << raw << "\n";
    std::cerr << "       got=\"" << decoded.canonical
              << "\" expected=\"" << expected_canonical << "\"\n";
    return false;
  }
  if (decoded.identity_hash_name.empty()) {
    std::cerr << "[FAIL] missing identity hash for: " << raw << "\n";
    return false;
  }
  if (hashes) hashes->insert(decoded.identity_hash_name);
  if (out_decoded) *out_decoded = decoded;

  std::cout << "[ok] raw=" << raw << "\n";
  std::cout << "     canonical=" << decoded.canonical << "\n";
  std::cout << "     identity_hash=" << decoded.identity_hash_name << "\n";
  if (!decoded.endpoint_hash_name.empty()) {
    std::cout << "     endpoint_hash=" << decoded.endpoint_hash_name << "\n";
  }
  return true;
}

bool expect_fail(const std::string& raw) {
  auto decoded = cuwacunu::camahjucunu::decode_canonical_path(raw);
  if (decoded.ok) {
    std::cerr << "[FAIL] expected parse failure for: " << raw << "\n";
    return false;
  }
  std::cout << "[ok] expected-fail raw=" << raw
            << " error=\"" << decoded.error << "\"\n";
  return true;
}

}  // namespace

int main() {
  try {
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    std::set<std::string> hashes{};
    bool ok = true;
    cuwacunu::camahjucunu::canonical_path_t d2{};
    cuwacunu::camahjucunu::canonical_path_t d3{};

    ok = ok && expect_ok(
        "iinuji.refresh()",
        canonical_path_kind_e::Call,
        "iinuji.refresh()",
        &hashes);
    ok = ok && expect_ok(
        "tsi.wikimyei.representation.vicreg@payload:tensor",
        canonical_path_kind_e::Endpoint,
        "",
        &hashes,
        &d2);
    ok = ok && expect_ok(
        "tsi.wikimyei.representation.vicreg.default.jkimyei@loss:tensor",
        canonical_path_kind_e::Endpoint,
        "",
        &hashes,
        &d3);

    const std::string c2_prefix = "tsi.wikimyei.representation.vicreg.";
    const std::string c2_suffix = "@payload:tensor";
    if (d2.hashimyei.empty() || d2.hashimyei == "default") {
      std::cerr << "[FAIL] expected resolved hashimyei name for vicreg default alias\n";
      return 1;
    }
    if (d2.canonical.rfind(c2_prefix, 0) != 0 ||
        d2.canonical.size() < c2_suffix.size() ||
        d2.canonical.substr(d2.canonical.size() - c2_suffix.size()) != c2_suffix) {
      std::cerr << "[FAIL] expected vicreg payload canonical with resolved hashimyei\n";
      return 1;
    }

    const std::string c3_expected =
        "tsi.wikimyei.representation.vicreg." + d2.hashimyei + ".jkimyei@loss:tensor";
    if (d3.canonical != c3_expected) {
      std::cerr << "[FAIL] expected jkimyei canonical to reuse resolved hashimyei\n";
      std::cerr << "       got=\"" << d3.canonical << "\" expected=\"" << c3_expected << "\"\n";
      return 1;
    }

    ok = ok && expect_fail("tsi.wikimyei.source.dataloader.default.jkimyei@payload:tensor");
    ok = ok && expect_fail("iinuji.view.data.plot(mode=seq)@unknown:tensor");

    if (!ok) return 1;
    if (hashes.size() < 3) {
      std::cerr << "[FAIL] expected distinct hashes across canonical identities\n";
      return 1;
    }

    std::cout << "[round1] " << cuwacunu::camahjucunu::hashimyei_round_note() << "\n";
    std::cout << "[round3] " << cuwacunu::camahjucunu::hashimyei_round_note() << "\n";
    std::cout << "[ok] canonical_path parser smoke passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_bnf_canonical_path] exception: " << e.what() << "\n";
    return 1;
  }
}
