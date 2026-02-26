/* binance_mech_account.h */
#pragma once
#include "camahjucunu/exchange/abstract/abstract_mech_account.h"
#include "camahjucunu/exchange/binance/binance_utils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {
namespace binance {
struct binance_mech_account_t: public abstract_mech_account_t {
/* variables */
public:
  bool owns_session;
  cuwacunu::piaabo::curl::ws_session_id_t session_id;
public:
  /* constructor */
  binance_mech_account_t(cuwacunu::piaabo::curl::ws_session_id_t _session_id = NULL_CURL_SESSION);
  ~binance_mech_account_t();
  
  /* methods */
  std::optional<account_information_ret_t> account_information(account_information_args_t args, bool await) const override;
  std::optional<account_order_history_ret_t> account_order_history(account_order_history_args_t args, bool await) const override;
  std::optional<account_trade_list_ret_t> account_trade_list(account_trade_list_args_t args, bool await) const override;
  std::optional<account_commission_rates_ret_t> account_commission_rates(account_commission_rates_args_t args, bool await) const override;
};

} /* namespace binance */
} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
