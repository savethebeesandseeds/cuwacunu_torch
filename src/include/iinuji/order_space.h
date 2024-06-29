#pragma once
#include "piaabo/dutils.h"

namespace cuwacunu {
/* order_space_t */
struct order_space_t {      /* there is no concept of BUY or SELL, it depends on the base and target symbols */
  instrument_e base_symb;   /*  |   instrument_e      | the currency of the holding capital to be converted into target_symb, once the order is liquidated */
  instrument_e target_symb; /*  |   instrument_e      | the type of coin that will convert the base_symb into, once the order is liquidated */
  float target_price;       /*  |   interval([0, inf])   | close settlement price (in target_symb/base_symb) */
  float target_amount;      /*  |   interval([0, inf])   | amount of shares to be bought of target_symb  */
  bool liquidated;          /*  |   bool              | flag to indicate if the order has been fulfiled */
  /* Constructor declaration */
  order_space_t(instrument_e base_symb, instrument_e target_symb, float target_price = INFINITY, float target_amount = 0, bool liquidated = false);
};
/* mechanic_order_t */
struct mechanic_order_t {
  action_space_t action;  /* action space, comming from the base policy network */
  order_space_t order;    /* order space, transforms the action space into an executable instruction*/
  /* Constructor declaration */
  mechanic_order_t(action_space_t& action, float target_amount);
};
} /* namespace cuwacunu */