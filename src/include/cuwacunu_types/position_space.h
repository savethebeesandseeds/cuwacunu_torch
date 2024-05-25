#pragma once
#include "dutils.h"

namespace cuwacunu {
/* position_space_t */
struct position_space_t {
  instrument_t symb;  /* Currency identifier */
  float amount;       /* Quantity held of the currency */
  float capital();    /* capital in ABSOLUTE_BASE_SYMB */
  /* Constructor declaration */
  position_space_t(instrument_t symb, float amount = 0);
  /* Copy Constructor */
  position_space_t(const position_space_t& src);
};
} /* namespace cuwacunu */