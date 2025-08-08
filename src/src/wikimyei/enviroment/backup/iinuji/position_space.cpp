#include "piaabo/dutils.h"
#include "iinuji/position_space.h"

RUNTIME_WARNING("(position_space.cpp)[] #FIXME change floats to double. \n");

namespace cuwacunu {
/* position_space_t */
position_space_t::position_space_t(instrument_e symb, float amount)
  : symb(symb), amount(amount) {}
position_space_t(const position_space_t& src) 
  : symb(src.symb), amount(src.amount) {}
float position_space_t::capital() {
  return amount * Broker::get_current_price(symb); 
}
} /* namespace cuwacunu */