#include <vector>
#include "piaabo/dutils.h"
#include "piaabo/architecture.h"
#include "iinuji/instrument_space.h"
RUNTIME_WARNING("(portfolio_space.h)[] add multithreading guards.\n");
RUNTIME_WARNING("(portfolio_space.h)[] Remmeber to include Capital Asset Pricing Model (CAPM) and Efficient Market Hypothesis (EMH).\n");

namespace cuwacunu {
namespace iinuji {

struct portfolio_space_t {
  size_t count_instruments;
  instrument_v_t<instrument_space_t> portfolio;
  /* constructors */
  portfolio_space_t(std::vector<std::string> instruments);
  /* iterators */
  iterator begin() { return portfolio.begin(); }
  iterator end() { return portfolio.end(); }
  instrument_iterator begin() const { return portfolio.cbegin(); }
  instrument_const_iterator end() const { return portfolio.cend(); }
};
ENFORCE_ARCHITECTURE_DESIGN(portfolio_space_t);

} /* namespace iinuji */
} /* namespace cuwacunu */