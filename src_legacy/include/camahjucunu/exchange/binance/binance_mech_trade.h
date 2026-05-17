#pragma once
#include "camahjucunu/exchange/abstract/abstract_mech_trade.h"
#include "camahjucunu/exchange/binance/binance_utils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {
namespace binance {
struct binance_mech_trade_t: public abstract_mech_trade_t {
/* variables */
public:
  bool owns_session;
  cuwacunu::piaabo::curl::ws_session_id_t session_id;
public:
  /* constructor */
  binance_mech_trade_t(cuwacunu::piaabo::curl::ws_session_id_t _session_id = NULL_CURL_SESSION);
  ~binance_mech_trade_t();

  /* methods */
  std::optional<order_ret_t> order(order_type_e type, order_args_t args, bool testOrder, bool await)const override;
  std::optional<orderStatus_ret_t> orderStatus(orderStatus_args_t args, bool await) const override;
  std::optional<orderMarket_ret_t> orderMarket(orderMarket_args_t args, bool testOrder, bool await) const override;
};

} /* namespace binance */
} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
