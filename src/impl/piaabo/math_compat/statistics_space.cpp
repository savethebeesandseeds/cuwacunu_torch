#include "piaabo/math_compat/statistics_space.h"

RUNTIME_WARNING("(statistics_space.h)[] statistics_space_t needs to include delta_time in the calculation.\n");
RUNTIME_WARNING("(statistics_space.h)[] statistics_space_t and statistics_space_n_t only work for dots spaced equaly in time.\n");
RUNTIME_WARNING("(statistics_space.cpp)[] #FIXME statistics_space_t for actual charts involve more measures.\n");
RUNTIME_WARNING("(statistics_space.cpp)[] is better to use Non-parametric spearman rank correlation coheficient.\n");
RUNTIME_WARNING("(statistics_space.h)[] add RSI to statistics_space_t.\n");
RUNTIME_WARNING("(statistics_space.h)[] add MACD to statistics_space_t.\n");

namespace cuwacunu {
/* statistics_space_t */
statistics_space_t::statistics_space_t(double initial_value) 
  : ctx(1),
    c_max(initial_value),
    c_min(initial_value),
    c_mean(initial_value),
    c_S(0) {}
void statistics_space_t::update(double x) { /* Welford's method */
  double old_mean = c_mean;
  ctx++;
  c_mean += (x - c_mean) / ctx;
  c_S += (x - old_mean) * (x - c_mean);
  c_max = MAX(c_max, x);
  c_min = MIN(c_min, x);
}
double statistics_space_t::normalize(double x) const { 
  double std_dev = stddev();
  return std_dev > 0 ? (x - mean()) / std_dev : 0.0;
}
double statistics_space_t::mean()           const { return ctx > 0 ? c_mean : 0.0; }
double statistics_space_t::variance()       const { return (ctx > 1) ? c_S / (ctx - 1) : 0.0; }
double statistics_space_t::stddev()         const { return std::sqrt(variance()); }
double statistics_space_t::max()            const { return c_max; }
double statistics_space_t::min()            const { return c_min; }
unsigned long statistics_space_t::count()   const { return ctx; }
statistics_space_t::statistics_space_t (const statistics_space_t& src) 
  : ctx(src.ctx),
    c_max(src.c_max),
    c_min(src.c_min),
    c_mean(src.c_mean),
    c_S(src.c_S) {}


/* statistics_space_n_t */
statistics_space_n_t::statistics_space_n_t (unsigned int N) : window_size(N) {}
bool  statistics_space_n_t::ready()         const { return ctx >= window_size; }
double statistics_space_n_t::mean()         const { return window.size() > 0 ? sum / window.size() : 0.0f; }
double statistics_space_n_t::stddev()       const { return std::sqrt(variance()); }
double statistics_space_n_t::max()          const { return !window_values.empty() ? *window_values.rbegin() : std::numeric_limits<double>::lowest(); }
double statistics_space_n_t::min()          const { return !window_values.empty() ? *window_values.begin() : std::numeric_limits<double>::max(); }
unsigned long statistics_space_n_t::count() const { return ctx; }
double statistics_space_n_t::normalize(double x) const { 
  double std_dev = stddev();
  return std_dev > 0 ? (x - mean()) / std_dev : 0.0;
}
double statistics_space_n_t::variance()     const {
  const unsigned int n = static_cast<unsigned int>(window.size());
  if (n <= 1) return 0.0;

  const double mu = mean();
  double ss = 0.0;
  for (const double x : window) {
    const double d = x - mu;
    ss += d * d;
  }
  return ss / static_cast<double>(n - 1);
}
void statistics_space_n_t::update(double x) {
  ctx++;
  /* Add new data point */
  window.push_back(x);
  window_values.insert(x);
  sum += x;
  sum_sq += x * x;
  /* Remove oldest data point if window is full */
  if (window.size() > window_size) {
    double x_old = window.front();
    window.pop_front();
    sum -= x_old;
    sum_sq -= x_old * x_old;
    /* Remove x_old from window_values */
    auto it = window_values.find(x_old);
    if (it != window_values.end()) {
      window_values.erase(it);
    }
  }
}

} /* namespace cuwacunu */
