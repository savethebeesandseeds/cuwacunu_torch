/* portfolio_space.h */
#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <stdexcept>

#include "wikimyei/capital_alocation_strategy/exposure_space.h"

RUNTIME_WARNING("(portfolio_space.h)[] get_total_market_value could include broker commissions\n");

namespace cuwacunu {
namespace wikimyei {
namespace capital_alocation_strategy {

struct portfolio_space_t {
    std::vector<exposure_space_t> holdings;   // order preserved – duplicates prevented on insertion

private:
    /*───────────────────────────────────────────────────────────────────────────
     * Internal helpers
     *─────────────────────────────────────────────────────────────────────────*/

    // mutable iterator finder (non‑const overload)
    [[nodiscard]] auto find_exposure(const instrument_space_t &instr) {
        return std::find_if(holdings.begin(), holdings.end(),
                            [&instr](const exposure_space_t &e) { return e.instrument == instr; });
    }

    // const iterator finder (const‑qualified overload)
    [[nodiscard]] auto find_exposure(const instrument_space_t &instr) const {
        return std::find_if(holdings.cbegin(), holdings.cend(),
                            [&instr](const exposure_space_t &e) { return e.instrument == instr; });
    }

public:
    /*───────────────────────────────────────────────────────────────────────────
     * Instrument utilities
     *─────────────────────────────────────────────────────────────────────────*/

    // Return the instrument symbols in the order they were added (duplicates possible)
    [[nodiscard]] std::vector<std::string> list_of_instruments() const {
        std::vector<std::string> instruments;
        instruments.reserve(holdings.size());
        for (const auto &item : holdings) instruments.push_back(item.instrument.symbol);
        return instruments; // maintains original ordering – caller may deduplicate if desired
    }

    // Check if an instrument already exists in the portfolio
    [[nodiscard]] bool contains_instrument(const instrument_space_t &instr) const noexcept {
        return find_exposure(instr) != holdings.cend();
    }

    /*───────────────────────────────────────────────────────────────────────────
     * Aggregates
     *─────────────────────────────────────────────────────────────────────────*/

    [[nodiscard]] float get_total_market_value() const {
        float total = 0.0f;
        for (const auto &item : holdings) total += item.get_market_value();
        return total;
    }

    [[nodiscard]] float get_total_sensitivity() const {
        float total = 0.0f;
        for (const auto &item : holdings) total += item.get_sensitivity();
        return total;
    }

    [[nodiscard]] float get_total_vulnerability() const {
        float total = 0.0f;
        for (const auto &item : holdings) total += item.get_vulnerability();
        return total;
    }

    /*───────────────────────────────────────────────────────────────────────────
     * Mutators
     *─────────────────────────────────────────────────────────────────────────*/

    // Adds a new exposure only if the instrument is not already present.
    // Returns true if added, false if duplicate (no modification).
    bool add_exposure(const exposure_space_t &exposure) {
        if (contains_instrument(exposure.instrument)) return false; // already present
        holdings.push_back(exposure);
        return true;
    }

    // Update quantity of an existing exposure; throws if not found
    void update_quantity(const instrument_space_t &instr, uint32_t new_qty) {
        auto it = find_exposure(instr);
        if (it == holdings.end())
            throw std::invalid_argument("Instrument not found in portfolio");
        it->quantity = new_qty;
    }

    // Adjust quantity by a signed delta; throws if instrument missing or result negative
    void delta_quantity(const instrument_space_t &instr, std::int64_t delta) {
        auto it = find_exposure(instr);
        if (it == holdings.end())
            throw std::invalid_argument("Instrument not found in portfolio");
        if (delta < 0 && it->quantity < static_cast<uint32_t>(-delta))
            throw std::domain_error("Resulting quantity would be negative");
        it->quantity = static_cast<uint32_t>(static_cast<std::int64_t>(it->quantity) + delta);
    }

    /*───────────────────────────────────────────────────────────────────────────
     * Diagnostics
     *─────────────────────────────────────────────────────────────────────────*/
    [[nodiscard]] std::string summary() const {
        std::ostringstream oss;
        oss << "Portfolio (" << holdings.size() << " positions): ";
        for (size_t i = 0; i < holdings.size(); ++i) {
            oss << holdings[i].to_string();
            if (i + 1 < holdings.size()) oss << " | ";
        }
        return oss.str();
    }
};

} // namespace capital_alocation_strategy
} // namespace wikimyei
} // namespace cuwacunu
