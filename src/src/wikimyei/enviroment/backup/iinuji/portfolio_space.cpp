#include "iinuji/portfolio_space.h"

namespace cuwacunu {
namespace iinuji {
portfolio_space_t::portfolio_space_t(std::vector<std::string> instruments) 
  : count_instruments(instruments.size()), portfolio() {
    portfolio.reserve(count_instruments);
    for(const auto& inst : instruments) {
      portfolio.emplace_back(..);
    }
}
} /* namespace iinuji */
} /* namespace cuwacunu */