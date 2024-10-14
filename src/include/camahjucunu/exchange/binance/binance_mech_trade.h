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
  cuwacunu::camahjucunu::curl::ws_session_id_t session_id;
/* methods */
public:
  /* constructor */
  binance_mech_trade_t(
    cuwacunu::camahjucunu::curl::ws_session_id_t _session_id = NULL_CURL_SESSION) 
      : owns_session(false), session_id(_session_id) {
    FORBIT_REAL_MECH("cuwacunu::camahjucunu::mech::binance::binance_mech_trade_t");
    NOTIFY_INIT("cuwacunu::camahjucunu::mech::binance::binance_mech_trade_t");
    ASSERT_SESSION(session_id, owns_session);
  }

  ~binance_mech_trade_t() {
    log_info("Finalizing cuwacunu::camahjucunu::mech::binance::binance_mech_trade_t \n");
    if(owns_session) { cuwacunu::camahjucunu::curl::WebsocketAPI::ws_finalize(session_id); }
  }

  std::optional<order_ret_t> order (
      order_type_e type, order_args_t args, bool testOrder, bool await )
    const override {
      switch(type) {
        case cuwacunu::camahjucunu::exchange::order_type_e::LIMIT:
          log_terminate_gracefully("Request to create [cuwacunu::camahjucunu::exchange::order_type_e::LIMIT] not implemented. Terminating program. \n");
          break;
        case cuwacunu::camahjucunu::exchange::order_type_e::MARKET:
          return orderMarket(std::get<orderMarket_args_t>(args), testOrder, await);
          break;
        case cuwacunu::camahjucunu::exchange::order_type_e::STOP_LOSS:
          log_terminate_gracefully("Request to create [cuwacunu::camahjucunu::exchange::order_type_e::STOP_LOSS] not implemented. Terminating program. \n");
          break;
        case cuwacunu::camahjucunu::exchange::order_type_e::STOP_LOSS_LIMIT:
          log_terminate_gracefully("Request to create [cuwacunu::camahjucunu::exchange::order_type_e::STOP_LOSS_LIMIT] not implemented. Terminating program. \n");
          break;
        case cuwacunu::camahjucunu::exchange::order_type_e::TAKE_PROFIT:
          log_terminate_gracefully("Request to create [cuwacunu::camahjucunu::exchange::order_type_e::TAKE_PROFIT] not implemented. Terminating program. \n");
          break;
        case cuwacunu::camahjucunu::exchange::order_type_e::TAKE_PROFIT_LIMIT:
          log_terminate_gracefully("Request to create [cuwacunu::camahjucunu::exchange::order_type_e::TAKE_PROFIT_LIMIT] not implemented. Terminating program. \n");
          break;
        case cuwacunu::camahjucunu::exchange::order_type_e::LIMIT_MAKER:
          log_terminate_gracefully("Request to create [cuwacunu::camahjucunu::exchange::order_type_e::LIMIT_MAKER] not implemented. Terminating program. \n");
          break;
      }
      return std::nullopt;
    }

  std::optional<orderStatus_ret_t> orderStatus(
      orderStatus_args_t args, bool await) 
    const override { 
      const char* method = "order.status";
      /* send the message (always) and retrive the response (if await is true) */
      SIGN_SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into orderStatus_ret_t type */
      return DESERIALIZE_FRAME(orderStatus_ret_t, response);
    }
     
  std::optional<orderMarket_ret_t> orderMarket(
      orderMarket_args_t args, bool testOrder, bool await) 
    const override {
      const char* method = testOrder ? "order.test" : "order.place";
      /* send the message (always) and retrive the response (if await is true) */
      SIGN_SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into orderMarket_ret_t type */
      return std::nullopt;
    }
};

} /* namespace binance */
} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
