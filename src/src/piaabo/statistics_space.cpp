#include "piaabo/statistics_space.h"

RUNTIME_WARNING("(statistics_space.h)[] statistics_space_t needs to include delta_time in the calculation.\n");
RUNTIME_WARNING("(statistics_space.cpp)[] #FIXME statistics_space_t for actual charts involve more measures.\n");
RUNTIME_WARNING("(statistics_space.cpp)[] is better to use Non-parametric spearman rank correlation coheficient.\n");
RUNTIME_WARNING("(statistics_space.h)[] add RSI to statistics_space_t.\n");
RUNTIME_WARNING("(statistics_space.h)[] add MACD to statistics_space_t.\n");
RUNTIME_WARNING("(statistics_space.cpp)[] #FIXME change floats to double. \n");

namespace cuwacunu {
/* statistics_space_t */
statistics_space_t::statistics_space_t(float initial_value) 
  : ctx(1),
    c_max(initial_value),
    c_min(initial_value),
    c_mean(initial_value),
    c_S(0) {}
void statistics_space_t::update(float x) { /* Welford's method */
  float old_mean = c_mean;
  ctx++;
  c_mean += (x - c_mean) / ctx;
  c_S += (x - old_mean) * (x - c_mean);
  c_max = MAX(c_max, x);
  c_min = MIN(c_min, x);
}
float statistics_space_t::mean()            const { return ctx > 0 ? c_mean : 0.0; }
float statistics_space_t::variance()        const { return (ctx > 1) ? c_S / (ctx - 1) : 0.0; }
float statistics_space_t::stddev()          const { return std::sqrt(variance()); }
float statistics_space_t::max()             const { return c_max; }
float statistics_space_t::min()             const { return c_min; }
unsigned long statistics_space_t::count()   const { return ctx; }
statistics_space_t::statistics_space_t(const statistics_space_t& src) 
  : ctx(src.ctx),
    c_max(src.c_max),
    c_min(src.c_min),
    c_mean(src.c_mean),
    c_S(src.c_S) {}
} /* namespace cuwacunu */