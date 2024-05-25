#include "dutils.h"
#include "/cuwacunu_types/order_space.h"

namespace cuwacunu {
/* order_space_t */
order_space_t::order_space_t(instrument_t base_symb, instrument_t target_symb, float target_price, float target_amount, bool liquidated)
  : base_symb(base_symb),
    target_symb(target_symb),
    target_price(target_price),
    target_amount(target_amount),
    liquidated(liquidated)
    { if (base_symb == target_symb) { log_warn("[order_space_t] base_symb and target_symb cannot be the same.\n"); }}
/* mechanic_order_t */
mechanic_order_t::mechanic_order_t(action_space_t& action, float target_amount) 
  : action(action),
    order(
    /* base_symb      */  action.base_symb,
    /* target_symb    */  action.target_symb,
    /* target_price   */  action.target_price(),
    /* target_amount  */  target_amount,        // #FIXME maybe avoid the argument and deduced
    /* liquidated     */  false) {}
} /* namespace cuwacunu */