#include "dutils.h"
#include "/cuwacunu_types/position_space.h"

namespace cuwacunu {
/* position_space_t */
position_space_t::position_space_t(instrument_t symb, float amount)
  : symb(symb), amount(amount) {}
position_space_t(const position_space_t& src) 
  : symb(src.symb), amount(src.amount) {}
float position_space_t::capital() {
  return amount * Broker::get_current_price(symb); 
}
} /* namespace cuwacunu */