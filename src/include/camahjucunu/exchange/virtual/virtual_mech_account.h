#pragma once
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_account.h"
#include "camahjucunu/exchange/exchange_types_enums.h"
#include "camahjucunu/https/curl_toolkit/websockets_api/curl_websocket_api.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         virtual exchange structure          */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct virtual_mech_account_t {
  virtual std::optional<account_information_ret_t>      account_information      (      account_information_args_t args, bool await = true )  const = 0;
  virtual std::optional<account_order_history_ret_t>    account_order_history    (    account_order_history_args_t args, bool await = true )  const = 0;
  virtual std::optional<account_trade_list_ret_t>       account_trade_list       (       account_trade_list_args_t args, bool await = true )  const = 0;
  virtual std::optional<account_commission_rates_ret_t> account_commission_rates ( account_commission_rates_args_t args, bool await = true )  const = 0;
  virtual ~virtual_mech_account_t                       () {}                    /* destructor */
};

} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
