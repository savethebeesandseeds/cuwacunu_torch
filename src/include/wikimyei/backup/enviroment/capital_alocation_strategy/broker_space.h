/* broker_space.h ----------------------------------------------------------- */
#pragma once

#include <string>
#include <cstdint>
#include <cmath>        // std::isfinite

#include "piaabo/dutils.h"
#include "wikimyei/capital_alocation_strategy/instrument_space.h"
#include "wikimyei/capital_alocation_strategy/quote_space.h"
#include "wikimyei/capital_alocation_strategy/portfolio_space.h"

RUNTIME_WARNING("(broker_space.h)[] simplified; exchange ignores broker commision\n");

namespace cuwacunu {
namespace wikimyei {
namespace capital_alocation_strategy {

/*───────────────────────────────────────────────────────────────────────────*/
struct broker_space_t {

    portfolio_space_t portfolio;

    /*─────────────────────────────────────────────────────────────────────────────
     * Forward-declare a helper that your environment already exposes somewhere:
     *   quote_space_t get_quote(const instrument_space_t& base_symb,
     *                           const instrument_space_t& target_symb);
     *───────────────────────────────────────────────────────────────────────────*/
    quote_space_t get_quote(const instrument_space_t& base_symb,
                            const instrument_space_t& target_symb)
    {
        #FIXME portfolio.... implement
    }


    /*---------------------------------------------------------------------*/
    [[nodiscard]]
    std::uint32_t exchange(std::uint32_t    from_qty,
                           const instrument_space_t& from_symb,
                           const instrument_space_t& to_symb)
    {
        if (from_symb == to_symb)
            throw std::invalid_argument("exchange(): symbols are identical");

        /* 1. ensure we hold enough of the source asset */
        if (!portfolio.contains_instrument(from_symb))
            throw std::domain_error("exchange(): no position in " + from_symb);

        /* 2. fetch quote base=from, target=to */
        quote_space_t q = get_quote(from_symb, to_symb);
        float ask = static_cast<float>(q.ask_price);

        if (ask <= 0.0f || !std::isfinite(ask))
            throw std::domain_error("exchange(): invalid ask price");

        /* 3. convert: target units we can buy with `from_qty` base units  */
        std::uint32_t buy_qty = static_cast<std::uint32_t>(
                                    static_cast<float>(from_qty) / ask );
        if (buy_qty == 0) return 0;

        /* 4. lose the from exposure */
        portfolio.delta_quantity(from_symb, -static_cast<std::int64_t>(from_qty));

        /* 5. receive the target exposure (or create it) */
        if (portfolio.contains_instrument(to_symb)) 
            portfolio.delta_quantity(to_symb, static_cast<std::int64_t>(buy_qty));
        else
            portfolio.add_exposure(exposure_space_t{to_symb, buy_qty});

        return buy_qty;
    }

    /* aggregates ----------------------------------------------------------*/
    [[nodiscard]] float get_total_market_value()  const { return portfolio.get_total_market_value();  }
    [[nodiscard]] float get_total_sensitivity()   const { return portfolio.get_total_sensitivity();   }
    [[nodiscard]] float get_total_vulnerability() const { return portfolio.get_total_vulnerability(); }
};

} // namespace capital_alocation_strategy
} // namespace wikimyei
} // namespace cuwacunu
