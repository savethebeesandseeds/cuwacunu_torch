/* exposure_space.h */
#pragma once

#include <string>

#include "piaabo/dutils.h"
#include "wikimyei/capital_alocation_strategy/instrument_space.h"

RUNTIME_WARNING("(exposure_space.h)[] sensitivy not implemented.\n");
RUNTIME_WARNING("(exposure_space.h)[] vulnerability not implemented.\n");

namespace cuwacunu {
namespace wikimyei {
namespace capital_alocation_strategy {

struct exposure_space_t {
    uint32_t quantity;
    instrument_space_t instrument;

    // Constructor to initialize all parameters explicitly
    exposure_space_t(const instrument_space_t& instrument_, uint32_t quantity_)
        : quantity(quantity_), instrument(instrument_) {}

    // Update or set exposure explicitly
    void set_exposure(const instrument_space_t& instrument_, uint32_t quantity_) {
        quantity = quantity_;
        instrument = instrument_;
    }

    // Retrieve the current exposure's market value
    [[nodiscard]] float get_market_value(const instrument_space_t& base_symb) const {
        float bid_price = instrument.get_quote(base_symb).bid_price;
        return dynamic_cast<float>(quantity) * bid_price;
    }

    // Calculate sensitivity to price change
    [[nodiscard]] float get_sensitivity() const {
        RUNTIME_WARNING("(exposure_space.h)[get_sensitivity] sensitivy not implemented\n");
        return get_market_value() * 0.0;
    }
    
    // Calculate vulnerability as a risk measure (example: volatility-based)
    [[nodiscard]] float get_vulnerability() const {
        RUNTIME_WARNING("(exposure_space.h)[get_vulnerability] vulnerability not implemented\n");
        return get_market_value() * 0.0;
    }

    // Retrieve exposure details as a formatted string
    [[nodiscard]] std::string to_string() const {
        return instrument.get_name() + ": Quantity = " + std::to_string(quantity) + 
               ", Market Value = " + std::to_string(get_market_value());
    }

    // Retrive the underlying instrument name
    [[nodiscard]] std::string get_name() const { return instrument.get_name(); }
};

} /* namespace capital_alocation_strategy */
} /* namespace wikimyei */
} /* namespace cuwacunu */