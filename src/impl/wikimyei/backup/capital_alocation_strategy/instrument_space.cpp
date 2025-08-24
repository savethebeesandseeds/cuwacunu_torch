/* instrument_space.cpp */
#include "wikimyei/enviroment/capital_alocation_strategy/instrument_space.h"
#include "wikimyei/enviroment/capital_alocation_strategy/quote_space.h"

#include <cmath>        // std::abs, std::isfinite
#include <algorithm>    // std::max

RUNTIME_WARNING("(instrument_space.cpp)[] #FIXME instrument_space_t could use many more indicators.\n");
RUNTIME_WARNING("(instrument_space.cpp)[] #FIXME instrument_space_t evaluate if instrument_space_t should be in tensors.\n");
RUNTIME_WARNING("(instrument_space.cpp)[] #FIXME instrument_space_t might be absorb or mix with quote_space_t.\n");
RUNTIME_WARNING("(instrument_space.cpp)[] #FIXME change floats to double. \n");

namespace cuwacunu {
namespace wikimyei {
namespace capital_alocation_strategy {

/* -----------------------------------------------------------------------
  Produce a quote for *this* instrument against a given base symbol.
  Throws if last_price is unset (NaN or ±Inf).                           */
quote_space_t
instrument_space_t::get_price(const std::string& base_sym) const
{
  ... // #FIXME implement

  /* 4. Wrap everything into a quote_space_t ------------------------- */
  return quote_space_t(...);
}

representation_space_t get_representation() {
  ... // #FIXME implement
}

} /* namespace capital_alocation_strategy */
} /* namespace wikimyei */
} /* namespace cuwacunu */