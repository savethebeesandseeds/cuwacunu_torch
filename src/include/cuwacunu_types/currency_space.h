#pragma once
#include "dutils.h"
#include "cuwacunu_types/instrument_space.h"
#include <torch/torch.h>

namespace cuwacunu {
/* currency_space_t */
struct currency_space_t {
private: /* to inforce architecture design, only friend classes have access */
  instrument_t symb;    /* Currency identifier */
  torch::Tensor token;  /* Tokenization of currency */
  torch::Tensor price;  /* price in ABSOLUTE_BASE_SYMB */
  statistics_t stats;   /* Welford's method statistics of the price */
  // ... RSI
  // ... MACD
  /* Usual Constructor */
  currency_space_t(const instrument_t symb, float price);
  /* Copy constructor */
  currency_space_t(const currency_space_t& src);
  /* Step */
  void currency_space_t::step(float updated_price);
  /* Step pirce (delta) */
  void currency_space_t::delta_step(float delta_price);
};
} /* namespace cuwacunu */