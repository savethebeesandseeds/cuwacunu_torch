#include <cassert>
#include <iostream>
#include <string>

#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"

int main() {
  try {
    using namespace cuwacunu::camahjucunu;

    wave_contract_binding_decl_t binding{};
    binding.id = "bind_demo";
    binding.contract_ref = "contract_demo";
    binding.wave_ref = "wave_demo";

    std::string error{};
    assert(append_wave_contract_binding_variable(&binding, "__sampler",
                                                 "sequential", &error));
    assert(error.empty());
    assert(append_wave_contract_binding_variable(&binding, "__workers", "2",
                                                 &error));
    assert(error.empty());

    std::string resolved{};
    const bool ok = resolve_wave_contract_binding_variables_in_text(
        "/* keep % __ignored ? nope % in comments */\n"
        "SAMPLER: % __sampler ? random %;\n"
        "WORKERS: %__workers?0%;\n"
        "SYMBOL: % __symbol ? BTCUSDT %;\n",
        binding, &resolved, &error);
    assert(ok);
    assert(error.empty());
    assert(resolved.find("% __ignored ? nope %") != std::string::npos);
    assert(resolved.find("SAMPLER: sequential;") != std::string::npos);
    assert(resolved.find("WORKERS: 2;") != std::string::npos);
    assert(resolved.find("SYMBOL: BTCUSDT;") != std::string::npos);

    error.clear();
    assert(!append_wave_contract_binding_variable(&binding, "__workers", "4",
                                                  &error));
    assert(error.find("duplicate binding variable") != std::string::npos);

    resolved.clear();
    error.clear();
    assert(!resolve_wave_contract_binding_variables_in_text(
        "VALUE: % __missing %;\n", binding, &resolved, &error));
    assert(error.find("missing binding variable") != std::string::npos);

    std::cout << "[test_wave_contract_binding_variables] pass\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_wave_contract_binding_variables] exception: "
              << e.what() << "\n";
    return 1;
  }
}
