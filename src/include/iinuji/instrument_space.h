#pragma once
#include <vector>
#include "piaabo/dutils.h"
#include "piaabo/architecture.h"
#include "camahjucunu/architecture.h"

RUNTIME_WARNING("(instrument_space.h)[] include all the desired here indicators.\n");
RUNTIME_WARNING("(instrument_space.h)[] one shot encoding for instrument_space_t is not stable across configuration changes, be aware not to include it.\n");






instrument contects to camahjucunu



namespace cuwacunu {
namespace iinuji {
template<typename T>
using instrument_v_t = std::vector<T>;
/* metric_t */
struct metric_t {
  float price;            /* price in ABSOLUTE_BASE_SYMB */
  statistics_t stats;     /* Welford's method statistics of the price */
};
ENFORCE_ARCHITECTURE_DESIGN(metric_t);

/* instrument_space_t */
struct instrument_space_t {
  metric_t ind;       /* indicators of the instrument */
  std::string str;        /* string representing the instrument */
  /* constructor */
  instrument_space_t(const std::string symb);
  /* step */
  void instrument_space_t::step(float updated_price);
};
ENFORCE_ARCHITECTURE_DESIGN(instrument_space_t);

/* template for a vector that holds elements for each type of instrument_e */
using instrument_iterator = instrument_v_t<instrument_space_t>::iterator;
using instrument_const_iterator = instrument_v_t<instrument_space_t>::const_iterator;

} /* namespace iinuji */
} /* namespace cuwacunu */