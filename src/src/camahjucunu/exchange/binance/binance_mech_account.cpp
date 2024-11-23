/* binance_mech_account.cpp */
#include "camahjucunu/exchange/binance/binance_mech_account.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {
namespace binance {

binance_mech_account_t::binance_mech_account_t(
  cuwacunu::camahjucunu::curl::ws_session_id_t _session_id) 
    : owns_session(false), session_id(_session_id) {
  NOTIFY_INIT("cuwacunu::camahjucunu::mech::binance::binance_mech_account_t");
  ASSERT_SESSION(session_id, owns_session);
}

binance_mech_account_t::~binance_mech_account_t() {
  log_info("Finalizing cuwacunu::camahjucunu::mech::binance::binance_mech_account_t \n");
  if(owns_session) { cuwacunu::camahjucunu::curl::WebsocketAPI::ws_finalize(session_id); }
}

std::optional<account_information_ret_t> binance_mech_account_t::account_information(
  account_information_args_t args, bool await) const { 
    const char* method = "account.status";
    /* send the message (always) and retrive the response (if await is true) */
    SIGN_SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into account_information_ret_t type */
    return DESERIALIZE_FRAME(account_information_ret_t, response);
}

std::optional<account_order_history_ret_t> binance_mech_account_t::account_order_history(
  account_order_history_args_t args, bool await) const { 
    const char* method = "allOrders";
    /* send the message (always) and retrive the response (if await is true) */
    SIGN_SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into account_order_history_ret_t type */
    return DESERIALIZE_FRAME(account_order_history_ret_t, response);
}

std::optional<account_trade_list_ret_t> binance_mech_account_t::account_trade_list(
  account_trade_list_args_t args, bool await) const { 
    const char* method = "myTrades";
    /* send the message (always) and retrive the response (if await is true) */
    SIGN_SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into account_trade_list_ret_t type */
    return DESERIALIZE_FRAME(account_trade_list_ret_t, response);
}

std::optional<account_commission_rates_ret_t> binance_mech_account_t::account_commission_rates(
  account_commission_rates_args_t args, bool await) const { 
    const char* method = "account.commission";
    /* send the message (always) and retrive the response (if await is true) */
    SIGN_SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into account_commission_rates_ret_t type */
    return DESERIALIZE_FRAME(account_commission_rates_ret_t, response);
}

} /* namespace binance */
} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
