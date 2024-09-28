#pragma once
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/exchange/exchange_mech.h"
#include "camahjucunu/https/curl_toolkit/websockets_api/curl_websocket_api.h"

RUNTIME_WARNING("(binance_mech.h)[] handle websocket api key revocation\n");
RUNTIME_WARNING("(binance_mech.h)[] handle websocket 24 h reconection\n");
RUNTIME_WARNING("(binance_mech.h)[] implmenet rest codes verification 200, 4XX, 400, 403, 409, 418, 429, 5XX\n");
RUNTIME_WARNING("(binance_mech.h)[] implement ratelimit cooldown\n");
RUNTIME_WARNING("(binance_mech.h)[] implement ratelimit verfication\n");
RUNTIME_WARNING("(binance_mech.h)[] implement two key managment system, one for account data and other for trading\n");
RUNTIME_WARNING("(binance_mech.h)[] if noted that bot is trying to do high frequency, it's important to manage recWindow\n");

/* # Methods
    ping
    time
    exchangeInfo
    depth
    trades.recent
    trades.historical
    trades.aggregate
    klines
    uiKlines
    avgPrice
    ticker.24hr
    ticker.tradingDay
    ticker
    ticker.price
    ticker.book
    session.logon
    session.status
    session.logout
    order.place
    order.test
    order.status
    order.cancel
    order.cancelReplace
    openOrders.status
    openOrders.cancelAll
    orderList.place
    orderList.place.oco
    orderList.place.oto
    orderList.place.otoco
    orderList.status
    orderList.cancel
    openOrderLists.status
    sor.order.place
    sor.order.test
    account.status
    account.rateLimits.orders
    allOrders
    allOrderLists
    myTrades
    myPreventedMatches
    myAllocations
    account.commission
    userDataStream.start
    userDataStream.ping
    userDataStream.stop
*/

#define FORMAT_FRAME(frame_id, method, args) \
  FORMAT_STRING("{\"id\":\"%s\",\"method\":\"%s\",\"params\":%s}", \
    frame_id.c_str(), method, args.jsonify().c_str())

#define SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response) \
    /* compute the frame id */ \
    const std::string frame_id_format = method + std::string("-xxxx-xxxx"); \
    std::string frame_id = cuwacunu::piaabo::generate_random_string(frame_id_format); \
    cuwacunu::piaabo::string_replace(frame_id, '.', '_'); \
    /* format and send the frame message */ \
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_text( \
      session_id, FORMAT_FRAME(frame_id, method, args), frame_id \
    ); \
    /* no awaiting for response case */ \
    if(!await) { \
      return std::nullopt; \
    } \
    /* await and retrive server response */ \
    std::optional<cuwacunu::camahjucunu::curl::ws_incomming_data_t> response =  \
      cuwacunu::camahjucunu::curl::WebsocketAPI::ws_await_and_retrive_server_response(session_id, frame_id); \
    /* handle failure case */ \
    if(!response.has_value()) { \
      log_fatal("Websocket server failed to respond to [ %s ] method, for session_id[ %d ] in frame_id[ %s ]\n",  \
        method, session_id, frame_id.c_str()); \
      return std::nullopt; \
    }

#define SIGN_SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, api_key, signature_fn, await, response) \
    /* compute the frame id */ \
    const std::string frame_id_format = method + std::string("-xxxx-xxxx"); \
    std::string frame_id = cuwacunu::piaabo::generate_random_string(frame_id_format); \
    cuwacunu::piaabo::string_replace(frame_id, '.', '_'); \
    /* */ \
    args.add_signature(api_key, signature_fn); \
    /* format and send the frame message */ \
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_text( \
      session_id, FORMAT_FRAME(frame_id, method, args), frame_id \
    ); \
    /* no awaiting for response case */ \
    if(!await) { \
      return std::nullopt; \
    } \
    /* await and retrive server response */ \
    std::optional<cuwacunu::camahjucunu::curl::ws_incomming_data_t> response =  \
      cuwacunu::camahjucunu::curl::WebsocketAPI::ws_await_and_retrive_server_response(session_id, frame_id); \
    /* handle failure case */ \
    if(!response.has_value()) { \
      log_fatal("Websocket server failed to respond to [ %s ] method, for session_id[ %d ] in frame_id[ %s ]\n",  \
        method, session_id, frame_id.c_str()); \
      return std::nullopt; \
    }

#define DESERIALIZE_FRAME(object, response) \
  cuwacunu::camahjucunu::exchange::object(response.value().data);

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace binance {

enum class mech_type_e {
  REAL,
  TESTNET
};

struct binance_mech_t: public exchange_mech_t {
/* variables */
public:
  const mech_type_e mech_type;
  cuwacunu::camahjucunu::curl::ws_session_id_t session_id;
  std::string websocket_url;
  std::string& api_key;
  cuwacunu::piaabo::dsecurity::signature_fn_t signature_fn;

/* methods */
public:
  /* constructor */
  binance_mech_t(const mech_type_e _mech_type) 
    : mech_type(_mech_type), session_id(NULL_CURL_SESSION) {
    
    log_info("Initializing Binance Mech \n");
        
    /* determine the endpoint */
    switch(mech_type) {
      case mech_type_e::REAL:
        log_fatal("%s %s %s\n",
          "[cuwacunu::camahjucunu::exchange::binance::binance_mech](): ",
          "Request to start in REAL mech.",
          "Are you out of your mind?");
        // websocket_url = "wss://ws-api.binance.com:443/ws-api/v3";
        (*environment_config)["ACTIVE_SYMBOLS"].c_str()
        break;
      case mech_type_e::TESTNET:
        websocket_url = "wss://testnet.binance.vision/ws-api/v3";
        break;
    }
    /* signature function is the same for both cases */
    signature_fn = cuwacunu::piaabo::dsecurity::SecureVault.Ed25519_signMessage;
    
    /* start the websocket session */
    session_id = cuwacunu::camahjucunu::curl::WebsocketAPI::ws_init(websocket_url);
  }

  ~binance_mech_t() {
    log_info("Finalizing Binance Mech \n");
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_finalize(session_id);
  }

  std::optional<ping_ret_t> ping(
      ping_args_t args, bool await) 
    const override {
      const char* method = "ping";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into ping_ret_t type */
      return DESERIALIZE_FRAME(ping_ret_t, response);
    }

  std::optional<time_ret_t> time(
      time_args_t args, bool await) 
    const override { 
      const char* method = "time";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into time_ret_t type */
      return DESERIALIZE_FRAME(time_ret_t, response);
     }

  std::optional<depth_ret_t> depth(
      depth_args_t args, bool await) 
    const override { 
      const char* method = "depth";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into depth_ret_t type */
      return DESERIALIZE_FRAME(depth_ret_t, response);
     }
  
  std::optional<klines_ret_t> klines(
      klines_args_t args, bool await) 
    const override { 
      const char* method = "klines";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into klines_ret_t type */
      return DESERIALIZE_FRAME(klines_ret_t, response);
     }

  std::optional<avgPrice_ret_t> avgPrice(
      avgPrice_args_t args, bool await) 
    const override { 
      const char* method = "avgPrice";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into avgPrice_ret_t type */
      return DESERIALIZE_FRAME(avgPrice_ret_t, response);
     }
  
  std::optional<ticker_ret_t> ticker(
      ticker_args_t args, bool await) 
    const override { 
      const char* method = "ticker";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into ticker_ret_t type */
      return DESERIALIZE_FRAME(ticker_ret_t, response);
     }

  std::optional<tickerTradingDay_ret_t> ticker_tradingDay(
      tickerTradingDay_args_t args, bool await) 
    const override { 
      const char* method = "ticker.tradingDay";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into tickerTradingDay_ret_t type */
      return DESERIALIZE_FRAME(tickerTradingDay_ret_t, response);
     }

  std::optional<tickerPrice_ret_t> tickerPrice(
      tickerPrice_args_t args, bool await) 
    const override { 
      const char* method = "ticker.price";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into tickerPrice_ret_t type */
      return DESERIALIZE_FRAME(tickerPrice_ret_t, response);
     }

  std::optional<tickerBook_ret_t> tickerBook(
      tickerBook_args_t args, bool await) 
    const override { 
      const char* method = "ticker.book";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into tickerBook_ret_t type */
      return DESERIALIZE_FRAME(tickerBook_ret_t, response);
     }

  std::optional<tradesRecent_ret_t> trades(
      trades_args_t args, bool await) 
    const override { 
      const char* method = "trades.recent";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into tradesRecent_ret_t type */
      return DESERIALIZE_FRAME(tradesRecent_ret_t, response);
     }

  std::optional<tradesHistorical_ret_t> tradesHistorical(
      tradesHistorical_args_t args, bool await) 
    const override { 
      const char* method = "trades.historical";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into tradesHistorical_ret_t type */
      return DESERIALIZE_FRAME(tradesHistorical_ret_t, response);
     }

  std::optional<orderStatus_ret_t> orderStatus(
      orderStatus_args_t args, bool await) 
    const override { 
      const char* method = "order.status";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into orderStatus_ret_t type */
      return DESERIALIZE_FRAME(orderStatus_ret_t, response);
     }
     
  std::optional<orderMarket_ret_t> orderMarket(
      orderMarket_args_t args, bool await) 
    const override { log_fatal("[binance_mech](orderMarket) not implemented.\n"); return std::nullopt; }
  
  std::optional<account_information_ret_t> account_information(
      account_information_args_t args, bool await) 
    const override { 
      const char* method = "account.status";
      /* send the message (always) and retrive the response (if await is true) */
      SIGN_SEND_AND_RETRIVE_FRAME(
        session_id, 
        frame_id, 
        method, 
        args, 
        *api_key,     /* API_KEY */
        signature_fn, /* sgnatureFN */
        await, 
        response);
      /* deserialize the response into account_information_ret_t type */
      return DESERIALIZE_FRAME(account_information_ret_t, response);
     }

  std::optional<account_order_history_ret_t> account_order_history(
      account_order_history_args_t args, bool await) 
    const override { 
      const char* method = "allOrders";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into account_order_history_ret_t type */
      return DESERIALIZE_FRAME(account_order_history_ret_t, response);
     }

  std::optional<account_trade_list_ret_t> account_trade_list(
      account_trade_list_args_t args, bool await) 
    const override { 
      const char* method = "myTrades";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into account_trade_list_ret_t type */
      return DESERIALIZE_FRAME(account_trade_list_ret_t, response);
     }

  std::optional<query_commission_rates_ret_t> query_commission_rates(
      query_commission_rates_args_t args, bool await) 
    const override { 
      const char* method = "account.commission";
      /* send the message (always) and retrive the response (if await is true) */
      SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
      /* deserialize the response into query_commission_rates_ret_t type */
      return DESERIALIZE_FRAME(query_commission_rates_ret_t, response);
     }
};

} /* namespace binance */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
