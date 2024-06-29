#pragma once
#include "piaabo/dutils.h"

namespace cuwacunu {
/* position_space_t */
struct position_space_t {
  instrument_e symb;  /* Currency identifier */
  float amount;       /* Quantity held of the currency */
  float capital();    /* capital in ABSOLUTE_BASE_SYMB */
  /* Constructor declaration */
  position_space_t(instrument_e symb, float amount = 0);
  /* Copy Constructor */
  position_space_t(const position_space_t& src);
};
} /* namespace cuwacunu */