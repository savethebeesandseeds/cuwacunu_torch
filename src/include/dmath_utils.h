#include "dtypes.h"

namespace cuwacunu {
struct statistics_t { /* #FIXME statistics for actual charts involve dt */
  unsigned long ctx = 0;  /* Number of data points */
  float c_max = std::numeric_limits<float>::lowest();    /* Max value */
  float c_min = std::numeric_limits<float>::max();     /* Min value */
  float c_mean = 0;   /* Running mean */
  float c_S = 0;     /* Running variance * (n-1), this is for the calculation of the sample variance */

  /* Constructor to initialize the struct with an initial value */
  statistics_t(float initial_value) 
  :   ctx(1),
    c_max(initial_value),
    c_min(initial_value),
    c_mean(initial_value),
    c_S(0) {}

  void update(float x) { /* Welford's method */
    float old_mean = c_mean;
    ctx++;
    c_mean += (x - c_mean) / ctx;
    c_S += (x - old_mean) * (x - c_mean);
    c_max = MAX(c_max, x);
    c_min = MIN(c_min, x);
  }

  float mean()    const { return ctx > 0 ? c_mean : 0.0; }
  float variance()const { return (ctx > 1) ? c_S / (ctx - 1) : 0.0; }
  float stddev()  const { return std::sqrt(variance()); }
  float max()     const { return c_max; }
  float min()     const { return c_min; }
  unsigned long count()   const { return ctx; }
};
}