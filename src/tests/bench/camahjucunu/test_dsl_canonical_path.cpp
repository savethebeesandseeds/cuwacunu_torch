// test_dsl_canonical_path.cpp
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "piaabo/dconfig.h"
#include "camahjucunu/dsl/canonical_path/canonical_path.h"

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
    cuwacunu::iitepi::config_space_t::change_config_file(config_folder);
    cuwacunu::iitepi::config_space_t::update_config();

    std::set<std::string> hashes{};
    bool ok = true;
    cuwacunu::camahjucunu::canonical_path_t d2{};
    cuwacunu::camahjucunu::canonical_path_t d3{};
    cuwacunu::camahjucunu::canonical_path_t d4{};
    cuwacunu::camahjucunu::canonical_path_t d5{};
    cuwacunu::camahjucunu::canonical_path_t d6{};
    cuwacunu::camahjucunu::canonical_path_t d7{};

    ok = ok && expect_ok(
        "tsi",
        canonical_path_kind_e::Node,
        "tsi",
        &hashes);
    ok = ok && expect_ok(
        "tsi.wikimyei",
        canonical_path_kind_e::Node,
        "tsi.wikimyei",
        &hashes);
    ok = ok && expect_ok(
        "tsi.wikimyei.representation",
        canonical_path_kind_e::Node,
        "tsi.wikimyei.representation",
        &hashes);
    ok = ok && expect_ok(
        "iinuji.refresh()",
        canonical_path_kind_e::Call,
        "iinuji.refresh()",
        &hashes);
    ok = ok && expect_ok(
        "board.wave",
        canonical_path_kind_e::Node,
        "board.wave",
        &hashes);
    ok = ok && expect_ok(
        "tsi.wikimyei.representation.vicreg.0x0001@payload:tensor",
        canonical_path_kind_e::Endpoint,
        "",
        &hashes,
        &d2);
    ok = ok && expect_ok(
        "tsi.wikimyei.representation.vicreg.0x0001@jkimyei:tensor",
        canonical_path_kind_e::Endpoint,
        "",
        &hashes,
        &d3);
    ok = ok && expect_ok(
        "tsi.wikimyei.representation.vicreg_0x0003@payload:tensor",
        canonical_path_kind_e::Endpoint,
        "",
        &hashes,
        &d4);
    ok = ok && expect_ok(
        "tsi.wikimyei.representation.vicreg.0x0003@payload:tensor",
        canonical_path_kind_e::Endpoint,
        "",
        &hashes,
        &d5);
    ok = ok && expect_ok(
        "tsi.source.dataloader@payload:tensor",
        canonical_path_kind_e::Endpoint,
        "tsi.source.dataloader@payload:tensor",
        &hashes,
        &d6);
    ok = ok && expect_ok(
        "tsi.source.dataloader@init:str",
        canonical_path_kind_e::Endpoint,
        "tsi.source.dataloader@init:str",
        &hashes,
        &d7);
    ok = ok && expect_ok(
        "tsi.source.dataloader.init()",
        canonical_path_kind_e::Call,
        "tsi.source.dataloader.init()",
        &hashes);

    const std::string c2_expected =
        "tsi.wikimyei.representation.vicreg.0x0001@payload:tensor";
    if (d2.canonical != c2_expected) {
      std::cerr << "[FAIL] expected vicreg payload canonical with explicit hashimyei\n";
      std::cerr << "       got=\"" << d2.canonical << "\" expected=\"" << c2_expected << "\"\n";
      return 1;
    }

    const std::string c3_expected =
        "tsi.wikimyei.representation.vicreg.0x0001@jkimyei:tensor";
    if (d3.canonical != c3_expected) {
      std::cerr << "[FAIL] expected @jkimyei canonical with explicit hashimyei\n";
      std::cerr << "       got=\"" << d3.canonical << "\" expected=\"" << c3_expected << "\"\n";
      return 1;
    }

    const std::string c4_expected =
        "tsi.wikimyei.representation.vicreg.0x0003@payload:tensor";
    if (d4.canonical != c4_expected) {
      std::cerr << "[FAIL] expected fused model/hash syntax to normalize to canonical dotted form\n";
      std::cerr << "       got=\"" << d4.canonical << "\" expected=\"" << c4_expected << "\"\n";
      return 1;
    }

    const std::string c5_expected =
        "tsi.wikimyei.representation.vicreg.0x0003@payload:tensor";
    if (d5.canonical != c5_expected) {
      std::cerr << "[FAIL] expected fused model/hash endpoint to normalize to canonical dotted form\n";
      std::cerr << "       got=\"" << d5.canonical << "\" expected=\"" << c5_expected << "\"\n";
      return 1;
    }

    const std::string c6_expected = "tsi.source.dataloader@payload:tensor";
    if (d6.canonical != c6_expected) {
      std::cerr << "[FAIL] expected source dataloader payload canonical\n";
      return 1;
    }
    const std::string c7_expected = "tsi.source.dataloader@init:str";
    if (d7.canonical != c7_expected) {
      std::cerr << "[FAIL] expected source dataloader init directive canonical\n";
      return 1;
    }
    ok = ok && expect_fail("tsi.wikimyei.representation.vicreg@payload:tensor");
    ok = ok && expect_fail("tsi.wikimyei.representation.vicreg.default@jkimyei:tensor");
    ok = ok && expect_fail("tsi.wikimyei.representation.vicreg_0x0003@weights:tensor");
    ok = ok && expect_fail("tsi.wikimyei.source.dataloader");
    ok = ok && expect_fail("tsi.wikimyei.source.dataloader.default@jkimyei:tensor");
    ok = ok && expect_fail("tsi.wikimyei.representation.vicreg.0x0001.jkimyei@loss:tensor");
    ok = ok && expect_fail("tsi.source.dataloader.jkimyei@payload:tensor");
    ok = ok && expect_fail("tsi.wikimyei.representation.vicreg.0x0001@meta:tensor");
    ok = ok && expect_fail("tsi.sink.log.sys@info:str");
    ok = ok && expect_fail("tsi.source.dataloader@init:tensor");
    ok = ok && expect_fail("tsi.wave");
    ok = ok && expect_fail("tsi.wave.generator");
    ok = ok && expect_fail("tsi.wikimyei.wave.generator");
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
    std::cerr << "[test_dsl_canonical_path] exception: " << e.what() << "\n";
    return 1;
  }
}
