#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#include "camahjucunu/dsl/lattice_target/lattice_target.h"
#include "piaabo/dfiles.h"

namespace {

[[nodiscard]] bool throws_lattice_target_decode(const std::string &grammar,
                                                const std::string &dsl) {
  try {
    (void)cuwacunu::camahjucunu::dsl::decode_lattice_target_from_dsl(grammar,
                                                                     dsl);
  } catch (const std::exception &) {
    return true;
  }
  return false;
}

} // namespace

int main() {
  try {
    const std::string grammar = cuwacunu::piaabo::dfiles::readFileToString(
        "/cuwacunu/src/config/bnf/lattice.target.bnf");
    const std::string instruction = cuwacunu::piaabo::dfiles::readFileToString(
        "/cuwacunu/src/config/instructions/defaults/"
        "default.lattice.target.dsl");

    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_lattice_target_from_dsl(grammar,
                                                                   instruction);

    std::cout << decoded.str() << "\n";

    assert(decoded.id == "default_lattice_target_v1");
    assert(decoded.contracts.size() == 1);
    assert(decoded.contracts[0].id == "contract_default_iitepi");
    assert(decoded.targets.size() == 1);

    const auto &target = decoded.targets[0];
    assert(target.id == "btcusdt_spot_vicreg_mdn_v1");
    assert(target.contract_ref == "contract_default_iitepi");
    assert(target.component_targets.size() == 2);
    assert(target.component_targets[0].slot == "w_rep");
    assert(target.component_targets[0].tag == "vicreg.default");
    assert(target.component_targets[1].slot == "w_ev");
    assert(target.component_targets[1].tag == "expected_value.mdn.default");

    assert(target.source.signature.symbol == "BTCUSDT");
    assert(target.source.signature.record_type == "kline");
    assert(target.source.signature.market_type == "spot");
    assert(target.source.signature.venue == "binance");
    assert(target.source.signature.base_asset == "BTC");
    assert(target.source.signature.quote_asset == "USDT");
    assert(target.source.train_window == "2020-01-01..2023-12-31");
    assert(target.source.validate_window == "2024-01-01..2024-08-31");
    assert(target.source.test_window == "2024-09-01..2024-12-31");

    assert(target.effort.epochs == 1024);
    assert(target.effort.batch_size == 64);
    assert(target.effort.max_batches_per_epoch == 4000);
    assert(target.require.trained);
    assert(target.require.validated);
    assert(target.require.tested);

    const auto obligations = decoded.obligations();
    assert(obligations.size() == 2);
    assert(obligations[0].target_id == target.id);
    assert(obligations[0].component_slot == "w_rep");
    assert(obligations[0].component_tag == "vicreg.default");
    assert(obligations[1].component_slot == "w_ev");
    assert(obligations[1].component_tag == "expected_value.mdn.default");

    const std::string any_source =
        "LATTICE_TARGET bad {"
        " IMPORT_CONTRACT \"./default.iitepi.contract.dsl\" AS c;"
        " TARGET t {"
        "  CONTRACT = c;"
        "  COMPONENT_TARGET { w_rep = TAG vicreg.default; };"
        "  SOURCE { SYMBOL = ANY; RECORD_TYPE = kline; MARKET_TYPE = spot;"
        "           VENUE = binance; BASE_ASSET = BTC; QUOTE_ASSET = USDT;"
        "           TRAIN = 2020..2021; VALIDATE = 2022..2022;"
        "           TEST = 2023..2023; };"
        "  EFFORT { EPOCHS = 1; BATCH_SIZE = 1;"
        "           MAX_BATCHES_PER_EPOCH = 1; };"
        "  REQUIRE { TRAINED = true; VALIDATED = true; TESTED = true; };"
        " };"
        "};";
    assert(throws_lattice_target_decode(grammar, any_source));

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "[test_dsl_lattice_target] exception: " << e.what() << "\n";
    return 1;
  }
}
