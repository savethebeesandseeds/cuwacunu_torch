#include <cassert>
#include <iostream>

#include "camahjucunu/dsl/jkimyei_campaign/jkimyei_campaign.h"
#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"
#include "piaabo/dfiles.h"

int main() {
  try {
    const std::string grammar =
        cuwacunu::piaabo::dfiles::readFileToString(
            "/cuwacunu/src/config/bnf/jkimyei_campaign.bnf");
    const std::string instruction =
        cuwacunu::piaabo::dfiles::readFileToString(
            "/cuwacunu/src/config/instructions/default.jkimyei.campaign.dsl");

    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_jkimyei_campaign_from_dsl(
            grammar, instruction);

    std::cout << decoded.str() << "\n";

    assert(decoded.active_campaign_id == "campaign_train_vicreg_primary");
    assert(decoded.contracts.size() == 1);
    assert(decoded.waves.size() == 1);
    assert(decoded.binds.size() == 1);
    assert(decoded.campaigns.size() == 1);

    assert(decoded.contracts[0].id == "contract_default_iitepi");
    assert(decoded.binds[0].id == "bind_train_vicreg_primary");
    assert(decoded.binds[0].contract_ref == "contract_default_iitepi");
    assert(decoded.binds[0].wave_ref == "train_vicreg_primary");
    assert(decoded.binds[0].variables.size() == 3);
    assert(decoded.binds[0].variables[0].name == "__sampler");
    assert(decoded.binds[0].variables[0].value == "sequential");
    assert(decoded.binds[0].variables[1].name == "__workers");
    assert(decoded.binds[0].variables[1].value == "0");
    assert(decoded.binds[0].variables[2].name == "__symbol");
    assert(decoded.binds[0].variables[2].value == "BTCUSDT");
    assert(decoded.campaigns[0].id == "campaign_train_vicreg_primary");
    assert(decoded.campaigns[0].bind_refs.size() == 1);
    assert(decoded.campaigns[0].bind_refs[0] == "bind_train_vicreg_primary");

    std::string resolved{};
    std::string resolve_error{};
    const bool ok =
        cuwacunu::camahjucunu::resolve_wave_contract_binding_variables_in_text(
            "SAMPLER: % __sampler ? random %;\n"
            "WORKERS: %__workers?1%;\n"
            "SYMBOL: % __symbol ? ETHUSDT %;\n",
            decoded.binds[0], &resolved, &resolve_error);
    assert(ok);
    assert(resolve_error.empty());
    assert(resolved.find("SAMPLER: sequential;") != std::string::npos);
    assert(resolved.find("WORKERS: 0;") != std::string::npos);
    assert(resolved.find("SYMBOL: BTCUSDT;") != std::string::npos);

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_dsl_jkimyei_campaign] exception: " << e.what() << "\n";
    return 1;
  }
}
