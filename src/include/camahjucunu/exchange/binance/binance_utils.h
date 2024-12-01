/* binance_utils.h */
#pragma once
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/https/curl_toolkit/websockets_api/curl_websocket_api.h"

/* # Methods (check if all of these are implemented)
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

#define SIGN_SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response) \
    /* compute the frame id */ \
    const std::string frame_id_format = method + std::string("-xxxx-xxxx"); \
    std::string frame_id = cuwacunu::piaabo::generate_random_string(frame_id_format); \
    cuwacunu::piaabo::string_replace(frame_id, '.', '_'); \
    /* */ \
    args.add_signature(); \
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

#define NOTIFY_INIT(mech_type) \
  switch(cuwacunu::piaabo::dconfig::config_space_t::exchange_type) { \
    case cuwacunu::piaabo::dconfig::exchange_type_e::REAL: \
      log_info("\tStarting Binance [%sREAL%s] %s%s%s.\n", \
        ANSI_COLOR_Green, ANSI_COLOR_RESET, ANSI_COLOR_Blue, mech_type, ANSI_COLOR_RESET); \
      break; \
    case cuwacunu::piaabo::dconfig::exchange_type_e::TEST: \
      log_info("\tStarting Binance [%sTESTNET%s] %s%s%s.\n", \
        ANSI_COLOR_Green, ANSI_COLOR_RESET, ANSI_COLOR_Blue, mech_type, ANSI_COLOR_RESET); \
      break; \
    case cuwacunu::piaabo::dconfig::exchange_type_e::NONE: \
      log_terminate_gracefully("[%s]() %s, terminating program.\n", \
        mech_type, "Request to start in Binance Mech without prior reading configuration"); \
      break; \
  }

#define FORBIT_REAL_MECH(mech_type) \
  if(cuwacunu::piaabo::dconfig::config_space_t::exchange_type == cuwacunu::piaabo::dconfig::exchange_type_e::REAL) { \
      log_terminate_gracefully("%s %s %s. %s, terminating program.\n", \
        "[cuwacunu::camahjucunu::exchange::mech::binance::...](): ", \
        "Request to start", mech_type, \
        "Are you out of your mind?"); \
  }

#define ASSERT_SESSION(session_id, owns_session) \
  /* if no websocket session_id is passed, then generate one */ \
  if((owns_session = (session_id == NULL_CURL_SESSION))) { \
    /* start the websocket session */ \
    session_id = cuwacunu::camahjucunu::curl::WebsocketAPI::ws_init( \
      /* websocket_url */ \
      cuwacunu::piaabo::dconfig::config_space_t::websocket_url() \
    ); \
  }
