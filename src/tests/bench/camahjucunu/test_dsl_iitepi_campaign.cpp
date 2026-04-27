#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"
#include "piaabo/dfiles.h"

int main() {
  try {
    const std::string grammar = cuwacunu::piaabo::dfiles::readFileToString(
        "/cuwacunu/src/config/bnf/iitepi.campaign.bnf");
    const std::string instruction = cuwacunu::piaabo::dfiles::readFileToString(
        "/cuwacunu/src/config/instructions/defaults/"
        "default.iitepi.campaign.dsl");

    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
            grammar, instruction);

    std::cout << decoded.str() << "\n";

    assert(decoded.contracts.size() == 1);
    assert(decoded.waves.size() == 1);
    assert(decoded.binds.size() == 1);
    assert(decoded.runs.size() == 1);

    assert(decoded.contracts[0].id == "contract_default_iitepi");
    assert(decoded.binds[0].id == "bind_train_vicreg_primary");
    assert(decoded.binds[0].contract_ref == "contract_default_iitepi");
    assert(decoded.binds[0].wave_ref == "train_vicreg_primary");
    assert(decoded.binds[0].variables.size() == 5);
    assert(decoded.binds[0].variables[0].name == "__sampler");
    assert(decoded.binds[0].variables[0].value == "sequential");
    assert(decoded.binds[0].variables[1].name == "__workers");
    assert(decoded.binds[0].variables[1].value == "0");
    assert(decoded.binds[0].variables[2].name == "__symbol");
    assert(decoded.binds[0].variables[2].value == "BTCUSDT");
    assert(decoded.binds[0].variables[3].name == "__base_asset");
    assert(decoded.binds[0].variables[3].value == "BTC");
    assert(decoded.binds[0].variables[4].name == "__quote_asset");
    assert(decoded.binds[0].variables[4].value == "USDT");
    assert(decoded.runs[0].bind_ref == "bind_train_vicreg_primary");

    std::string resolved{};
    std::string resolve_error{};
    const bool ok =
        cuwacunu::camahjucunu::resolve_wave_contract_binding_variables_in_text(
            "SAMPLER: % __sampler ? random %;\n"
            "WORKERS: %__workers?1%;\n"
            "SYMBOL: % __symbol %;\n"
            "BASE_ASSET: % __base_asset %;\n"
            "QUOTE_ASSET: % __quote_asset %;\n",
            decoded.binds[0], &resolved, &resolve_error);
    assert(ok);
    assert(resolve_error.empty());
    assert(resolved.find("SAMPLER: sequential;") != std::string::npos);
    assert(resolved.find("WORKERS: 0;") != std::string::npos);
    assert(resolved.find("SYMBOL: BTCUSDT;") != std::string::npos);
    assert(resolved.find("BASE_ASSET: BTC;") != std::string::npos);
    assert(resolved.find("QUOTE_ASSET: USDT;") != std::string::npos);

    const std::filesystem::path instructions_root{
        "/cuwacunu/src/config/instructions"};
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(instructions_root)) {
      if (!entry.is_regular_file())
        continue;
      const std::string filename = entry.path().filename().string();
      if (filename != ".campaign.dsl" && filename != ".campaign.alt.dsl")
        continue;
      const std::string campaign_path = entry.path().generic_string();
      if (campaign_path.find("/objectives/source.lint.") != std::string::npos) {
        continue;
      }
      const std::string campaign_text =
          cuwacunu::piaabo::dfiles::readFileToString(entry.path().string());
      cuwacunu::camahjucunu::iitepi_campaign_instruction_t objective_campaign{};
      try {
        objective_campaign =
            cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
                grammar, campaign_text);
      } catch (const std::exception &e) {
        std::cerr << "[test_dsl_iitepi_campaign] objective campaign failed: "
                  << campaign_path << ": " << e.what() << "\n";
        throw;
      }
      assert(!objective_campaign.contracts.empty());
      assert(!objective_campaign.waves.empty());
      assert(!objective_campaign.binds.empty());
      assert(!objective_campaign.runs.empty());
    }

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "[test_dsl_iitepi_campaign] exception: " << e.what() << "\n";
    return 1;
  }
}
