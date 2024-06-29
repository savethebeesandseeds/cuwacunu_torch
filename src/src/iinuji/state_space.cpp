#include "piaabo/dutils.h"
#include "iinuji/state_space.h"

namespace cuwacunu {
/* state_space_t */
state_space_t::state_space_t(const instrument_v_t<state_features_t> instruments_state_feat) 
  : instruments_state_feat(std::move(instruments_state_feat)) {
    FOR_ALL_INSTRUMENTS(inst) {
      validate_tensor(instruments_state_feat[inst], "state_space_t constructor");
    }
}
state_space_t::state_space_t(const state_space_t& src) {
  FOR_ALL_INSTRUMENTS(inst) {
    /* deep clone the source */
    instruments_state_feat.push_back(src.instruments_state_feat[inst].clone().detach());
    validate_tensor(instruments_state_feat[inst], "state_space_t constructor");
  }
}
torch::Tensor state_space_t::unpack() const {
  torch::Tensor unpacked = torch::cat(instruments_state_feat, 0);
    validate_tensor(unpacked, "state_space_t::unpack()");
  return unpacked;
}
} /* namespace cuwacunu */