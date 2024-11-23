#pragma once
#include "camahjucunu/exchange/abstract/abstract_mech_server.h"
#include "camahjucunu/exchange/binance/binance_utils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {
namespace binance {
struct binance_mech_server_t: public abstract_mech_server_t {
/* variables */
public:
  bool owns_session;
  cuwacunu::camahjucunu::curl::ws_session_id_t session_id;
public:
  /* constructor */
  binance_mech_server_t(cuwacunu::camahjucunu::curl::ws_session_id_t _session_id = NULL_CURL_SESSION);
  ~binance_mech_server_t();

  /* methods */
  std::optional<ping_ret_t> ping(ping_args_t args, bool await) const override;
  std::optional<time_ret_t> time(time_args_t args, bool await) const override;
};

} /* namespace binance */
} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
