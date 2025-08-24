/* quote_space.h */
#pragma once
#include <ctime>
#include <cmath>
#include <stdexcept>
#include "piaabo/dutils.h"
#include "wikimyei/enviroment/capital_alocation_strategy/dutils.h"

namespace cuwacunu {
namespace wikimyei {
namespace capital_alocation_strategy {

  /* quote_space_t 
      price = (how many units of BASE you pay)  per   1 unit of TARGET
              └── ask/bid price measured in BASE ─┘         └──────┘
  */
struct quote_space_t {
  instrument_space_t base_symb;     /* The base symbol (e.g., USD in USD/EUR) */
  instrument_space_t target_symb;   /* The target symbol (e.g., EUR in USD/EUR) */
  double bid_price;           /* Highest price a buyer is willing to pay */
  double ask_price;           /* Lowest price a seller is willing to accept */
  double bid_size;            /* Quantity available at the bid price */
  double ask_size;            /* Quantity available at the ask price */
  std::time_t timestamp;      /* Time at which the quote is valid */
  /* Usual Constructor with validations */
  inline quote_space_t(
            instrument_space_t  base,
            instrument_space_t  target,
            double              bidP,
            double              askP,
            double              bidS,
            double              askS, 
            std::time_t timestamp = std::time(nullptr))
    : base_symb   (base), 
      target_symb (target), 
      bid_price   (bidP), 
      ask_price   (askP), 
      bid_size    (bidS), 
      ask_size    (askS), 
      timestamp   (timestamp)
  {
    /* --- basic sanity checks --- */
    if (base_symb == target_symb) {
      throw std::invalid_argument("(quote_space.h)[constructor]: base and target symbols must differ");}
    if (bid_price > ask_price) {
      throw std::invalid_argument("(quote_space.h)[constructor]: bid price cannot exceed ask price");}
    if (bid_size < 0.0 || ask_size < 0.0) {
      throw std::invalid_argument("(quote_space.h)[constructor]: bid/ask sizes must be non-negative");}
    if (!std::isfinite(bid_price) || !std::isfinite(ask_price)) {
      throw std::invalid_argument("(quote_space.h)[constructor] bid/ask must be finite numbers");}
    if (!std::isfinite(bid_size)  || !std::isfinite(bid_size) ) {
      throw std::invalid_argument("(quote_space.h)[constructor]: bid/ask sizes must be finite");}
  }
};
} /* namespace capital_alocation_strategy */
} /* namespace wikimyei */
} /* namespace cuwacunu */