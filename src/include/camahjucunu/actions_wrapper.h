#pragma once
#include "piaabo/dutils.h"
RUNTIME_WARNING("(actions_wrapper.h)[] tsodao aprove \n");
RUNTIME_WARNING("(actions_wrapper.h)[] the placement of an order does not have to specify the price or timesampt, the price is ovieted from the current timesamp \n");

namespace cuwacunu {
namespace camahjucunu {
/*  */
trade_result_t trade(symbol_t from, symbol_t to, double qty);
defease_result_t defease(/* ... */);
hedge_result_t hedge(/* ... */);
arbitrage_result_t arbitrage(/* ... */);
} /* namespace camahjucunu */
} /* namespace cuwacunu*/
