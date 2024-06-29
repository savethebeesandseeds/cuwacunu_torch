#pragma once
#include <ctime>
#include "piaabo/dutils.h"

namespace cuwacunu {
struct quote_space_t {
  instrument_e base_symb;     /* The base symbol (e.g., USD in USD/EUR) */
  instrument_e target_symb;   /* The target symbol (e.g., EUR in USD/EUR) */
  double bid_price;           /* Highest price a buyer is willing to pay */
  double ask_price;           /* Lowest price a seller is willing to accept */
  double bid_size;            /* Quantity available at the bid price */
  double ask_size;            /* Quantity available at the ask price */
  std::time_t timestamp;      /* Time at which the quote is valid */
  /* Usual Constructor */
  quote_space_t(instrument_e base, instrument_e target, double bidP, double askP, double bidS, double askS);
};
} /* namespace cuwacunu */