// test_iinuji_cmd_terminal.cpp
#include <iostream>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "piaabo/dconfig.h"
#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "iinuji/iinuji_cmd/commands.h"

namespace {

using cuwacunu::camahjucunu::canonical_path_kind_e;

bool require(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "[FAIL] " << msg << "\n";
    return false;
  }
  return true;
}

std::set<std::string> source_dataloader_init_snapshot() {
  std::set<std::string> out;
  for (const auto& item : tsiemene::list_source_dataloader_init_entries()) {
    out.insert(item.init_id);
  }
  return out;
}

bool cleanup_new_source_dataloader_inits(const std::set<std::string>& baseline) {
  bool ok = true;
  for (const auto& item : tsiemene::list_source_dataloader_init_entries()) {
    if (baseline.count(item.init_id)) continue;
    std::uintmax_t removed_count = 0;
    std::string error;
    if (!tsiemene::delete_source_dataloader_init(item.init_id, &removed_count, &error)) {
      std::cerr << "[cleanup] failed to remove source dataloader init " << item.init_id << ": " << error
                << "\n";
      ok = false;
    }
  }
  return ok;
}

void print_decoded(const cuwacunu::camahjucunu::canonical_path_t& d) {
  std::cout << "raw:       " << d.raw << "\n";
  std::cout << "canonical: " << d.canonical << "\n";
  std::cout << "kind:      "
            << (d.path_kind == canonical_path_kind_e::Call ? "Call"
                : (d.path_kind == canonical_path_kind_e::Endpoint ? "Endpoint" : "Node"))
            << "\n";
  if (!d.directive.empty()) std::cout << "directive: " << d.directive << "\n";
  if (!d.kind.empty()) std::cout << "payload:   " << d.kind << "\n";
  std::cout << "idhash:    " << d.identity_hash_name << "\n";
  if (!d.endpoint_hash_name.empty()) std::cout << "ephash:    " << d.endpoint_hash_name << "\n";
}

}  // namespace

int main() {
  std::set<std::string> source_dataloader_init_before;
  try {
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();
    source_dataloader_init_before = source_dataloader_init_snapshot();

    const std::vector<std::string> valid_samples = {
        "iinuji.refresh()",
        "iinuji.view.data.plot(mode=seq)",
        "board.wave",
        "tsi.source.dataloader@payload:tensor",
        "tsi.source.dataloader@init:str",
        "tsi.wikimyei.representation.vicreg.default@payload:tensor",
        "tsi.wikimyei.representation.vicreg.default@jkimyei:tensor",
        "tsi.wikimyei.representation.vicreg_0x3@weights:tensor",
    };

    std::set<std::string> identity_hashes;
    std::set<std::string> canonical_identities;
    bool ok = true;
    for (const auto& sample : valid_samples) {
      auto decoded = cuwacunu::camahjucunu::decode_canonical_path(sample);
      print_decoded(decoded);
      std::cout << "---\n";

      ok = ok && require(decoded.ok, "sample should parse: " + sample);
      ok = ok && require(cuwacunu::camahjucunu::validate_canonical_path(decoded), "sample should validate: " + sample);
      ok = ok && require(!decoded.identity_hash_name.empty(), "identity hash should not be empty: " + sample);
      identity_hashes.insert(decoded.identity_hash_name);
      canonical_identities.insert(decoded.canonical_identity);
    }

    // Deterministic hash check on same canonical expression.
    {
      auto a = cuwacunu::camahjucunu::decode_canonical_path(valid_samples.front());
      auto b = cuwacunu::camahjucunu::decode_canonical_path(valid_samples.front());
      ok = ok && require(a.identity_hash_name == b.identity_hash_name,
                         "identity hash must be deterministic");
    }

    // Distinct canonical identities should produce distinct identity hashes.
    ok = ok && require(identity_hashes.size() == canonical_identities.size(),
                       "expected one identity hash per canonical identity");

    // Invalid directive/kind should fail.
    {
      auto invalid_dir = cuwacunu::camahjucunu::decode_canonical_path(
          "tsi.wikimyei.representation.vicreg.default@unknown:tensor");
      ok = ok && require(!invalid_dir.ok, "invalid directive must fail");
    }
    {
      auto invalid_kind = cuwacunu::camahjucunu::decode_canonical_path(
          "tsi.wikimyei.representation.vicreg.default@payload:bytes");
      ok = ok && require(!invalid_kind.ok, "invalid kind must fail");
    }
    {
      auto source_jk = cuwacunu::camahjucunu::decode_canonical_path(
          "tsi.wikimyei.source.dataloader.default@jkimyei:tensor");
      ok = ok && require(!source_jk.ok, "legacy source dataloader wikimyei path should fail");
    }
    {
      auto facet_jk = cuwacunu::camahjucunu::decode_canonical_path(
          "tsi.wikimyei.representation.vicreg.default.jkimyei@loss:tensor");
      ok = ok && require(!facet_jk.ok, "legacy .jkimyei facet syntax should fail");
    }
    {
      auto source_init = cuwacunu::camahjucunu::decode_canonical_path(
          "tsi.source.dataloader.init()");
      ok = ok && require(source_init.ok, "source dataloader init path should parse");
    }
    {
      auto source_no_jk = cuwacunu::camahjucunu::decode_canonical_path(
          "tsi.source.dataloader.jkimyei@payload:tensor");
      ok = ok && require(!source_no_jk.ok, "source dataloader should not accept .jkimyei facet syntax");
    }
    {
      auto bad_vicreg_meta = cuwacunu::camahjucunu::decode_canonical_path(
          "tsi.wikimyei.representation.vicreg.default@meta:tensor");
      ok = ok && require(!bad_vicreg_meta.ok, "vicreg should reject @meta:tensor");
    }
    {
      auto bad_log_info = cuwacunu::camahjucunu::decode_canonical_path(
          "tsi.sink.log.sys@info:str");
      ok = ok && require(!bad_log_info.ok, "sink.log.sys should reject @info:str");
    }
    {
      auto bad_init_kind = cuwacunu::camahjucunu::decode_canonical_path(
          "tsi.source.dataloader@init:tensor");
      ok = ok && require(!bad_init_kind.ok, "source dataloader should reject @init:tensor");
    }
    // Migration adapter stubs (primitive -> DSL) smoke.
    {
      auto primitive_ep = cuwacunu::camahjucunu::decode_primitive_endpoint_text("w_rep@loss:tensor");
      print_decoded(primitive_ep);
      ok = ok && require(primitive_ep.ok, "primitive endpoint adapter should produce valid canonical path");
    }
    {
      auto primitive_cmd = cuwacunu::camahjucunu::decode_primitive_command_text("data plot seq");
      print_decoded(primitive_cmd);
      ok = ok && require(primitive_cmd.ok, "primitive command adapter should produce valid canonical path");
    }

    // Regression: command dispatch must be safe even when no command log box is provided.
    {
      cuwacunu::iinuji::iinuji_cmd::CmdState st{};
      cuwacunu::iinuji::iinuji_cmd::run_command(st, "help", nullptr);
      ok = ok && require(st.help_view, "help command should enable help overlay without log box");
      st.help_view = false;

      cuwacunu::iinuji::iinuji_cmd::run_command(st, "h", nullptr);
      ok = ok && require(st.help_view, "h alias should enable help overlay without log box");
      st.help_view = false;

      cuwacunu::iinuji::iinuji_cmd::run_command(st, "iinuji.help", nullptr);
      ok = ok && require(st.help_view, "help canonical shorthand should enable help overlay");
      st.help_view = false;

      cuwacunu::iinuji::iinuji_cmd::run_command(st, "tsi.iinuji.help()", nullptr);
      ok = ok && require(!st.help_view, "tsi.iinuji command prefix should not dispatch");

      cuwacunu::iinuji::iinuji_cmd::run_command(st, "iinuji.data.plot.on()", nullptr);
      ok = ok && require(st.data.plot_view, "plot on command should work without log box");

      cuwacunu::iinuji::iinuji_cmd::run_command(st, "iinuji.data.plot.off", nullptr);
      ok = ok && require(!st.data.plot_view, "plot off shorthand should work without log box");

      cuwacunu::iinuji::iinuji_cmd::run_command(st, "iinuji.data.plot.off()", nullptr);
      ok = ok && require(!st.data.plot_view, "plot off command should work without log box");

      cuwacunu::iinuji::iinuji_cmd::run_command(st, "dataloader.init", nullptr);
      ok = ok && require(st.screen == cuwacunu::iinuji::iinuji_cmd::ScreenMode::Tsiemene,
                         "dataloader.init alias should switch to tsi screen");

      cuwacunu::iinuji::iinuji_cmd::run_command(st, "quit", nullptr);
      ok = ok && require(!st.running, "quit alias should work without log box");
      st.running = true;

      cuwacunu::iinuji::iinuji_cmd::run_command(st, "q", nullptr);
      ok = ok && require(!st.running, "q alias should work without log box");
      st.running = true;

      cuwacunu::iinuji::iinuji_cmd::run_command(st, "exit", nullptr);
      ok = ok && require(!st.running, "exit alias should work without log box");
    }

    ok = ok && require(cleanup_new_source_dataloader_inits(source_dataloader_init_before),
                       "cleanup: created tsi.source.dataloader init artifacts should be removed");

    std::cout << "[round2] " << cuwacunu::camahjucunu::hashimyei_round_note() << "\n";

    if (!ok) return 1;
    std::cout << "[ok] iinuji cmd terminal dsl smoke passed\n";
    return 0;
  } catch (const std::exception& e) {
    cleanup_new_source_dataloader_inits(source_dataloader_init_before);
    std::cerr << "[test_iinuji_cmd_terminal] exception: " << e.what() << "\n";
    return 1;
  }
}
