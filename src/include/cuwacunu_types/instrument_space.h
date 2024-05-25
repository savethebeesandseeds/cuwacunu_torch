#pragma once
#include <vector>
#include "dutils.h"

namespace cuwacunu {
typedef enum instrument {
  CONST,                    /* constant value */
  SINE,                     /* simple sine wave */
  COUNT_INSTRUMENTS         /* total amount of instruments */
} instrument_t;

/* forall instrument vector, to make it clear that the vector has elements for each instrument_t */
template<typename T>
using instrument_v_t = std::vector<T>;

/* token to string */
static const std::string CURRENCY_STRING[] = {
	std::string("CONST"),
	std::string("SINE")
};

using instrument_token_t = std::vector<int>;
/* tokenize the intruments, used to define network outputs */
static const std::vector<instrument_token_t> CURRENCY_TOKENIZER = {
  {1, 0}, // CONST
  {0, 1}  // SINE
};

} /* namespace cuwacunu */

#define FOR_ALL_INSTRUMENTS(inst) for (cuwacunu::instrument_t inst = (cuwacunu::instrument_t) 0; inst < COUNT_INSTRUMENTS; inst = static_cast<cuwacunu::instrument_t>(inst + 1))
