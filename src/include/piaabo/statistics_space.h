#pragma once
#include <cmath>
#include "piaabo/dutils.h"
#include "piaabo/architecture.h"

namespace cuwacunu {
/* statistics_space_t */
struct statistics_space_t {
  unsigned long ctx = 0;                                /* Number of data points */
  float c_max = std::numeric_limits<float>::lowest();   /* Max value */
  float c_min = std::numeric_limits<float>::max();      /* Min value */
  float c_mean = 0;                                     /* Running mean */
  float c_S = 0;                                        /* Running variance * (n-1), this is for the calculation of the sample variance */
  /* Constructor declaration */
  statistics_space_t(float initial_value);
  void update(float x);
  float mean()            const;
  float variance()        const;
  float stddev()          const;
  float max()             const;
  float min()             const;
  unsigned long count()   const;
  /* Copy Constructor */
  statistics_space_t(const statistics_space_t& src);
};
ENFORCE_ARCHITECTURE_DESIGN(statistics_space_t);
} /* namespace cuwacunu */