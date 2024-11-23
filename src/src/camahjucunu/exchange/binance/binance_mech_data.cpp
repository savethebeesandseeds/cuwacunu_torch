/* binance_mech_data.cpp */
#include "camahjucunu/exchange/binance/binance_mech_data.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {
namespace binance {

/* constructor */
binance_mech_data_t::binance_mech_data_t(
  cuwacunu::camahjucunu::curl::ws_session_id_t _session_id) 
    : owns_session(false), session_id(_session_id) {
  NOTIFY_INIT("cuwacunu::camahjucunu::mech::binance::binance_mech_data_t");
  ASSERT_SESSION(session_id, owns_session);
}

binance_mech_data_t::~binance_mech_data_t() {
  log_info("Finalizing cuwacunu::camahjucunu::mech::binance::binance_mech_data_t \n");
  if(owns_session) { cuwacunu::camahjucunu::curl::WebsocketAPI::ws_finalize(session_id); }
}

std::optional<depth_ret_t> binance_mech_data_t::depth(
  depth_args_t args, bool await) const { 
    const char* method = "depth";
    /* send the message (always) and retrive the response (if await is true) */
    SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into depth_ret_t type */
    return DESERIALIZE_FRAME(depth_ret_t, response);
}
  
std::optional<klines_ret_t> binance_mech_data_t::klines(
  klines_args_t args, bool await) const { 
    const char* method = "klines";
    /* send the message (always) and retrive the response (if await is true) */
    SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into klines_ret_t type */
    return DESERIALIZE_FRAME(klines_ret_t, response);
}

std::optional<avgPrice_ret_t> binance_mech_data_t::avgPrice(
  avgPrice_args_t args, bool await) const { 
    const char* method = "avgPrice";
    /* send the message (always) and retrive the response (if await is true) */
    SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into avgPrice_ret_t type */
    return DESERIALIZE_FRAME(avgPrice_ret_t, response);
}
  
std::optional<ticker_ret_t> binance_mech_data_t::ticker(
  ticker_args_t args, bool await) const { 
    const char* method = "ticker";
    /* send the message (always) and retrive the response (if await is true) */
    SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into ticker_ret_t type */
    return DESERIALIZE_FRAME(ticker_ret_t, response);
}

std::optional<tickerTradingDay_ret_t> binance_mech_data_t::ticker_tradingDay(
  tickerTradingDay_args_t args, bool await) const { 
    const char* method = "ticker.tradingDay";
    /* send the message (always) and retrive the response (if await is true) */
    SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into tickerTradingDay_ret_t type */
    return DESERIALIZE_FRAME(tickerTradingDay_ret_t, response);
}

std::optional<tickerPrice_ret_t> binance_mech_data_t::tickerPrice(
  tickerPrice_args_t args, bool await) const { 
    const char* method = "ticker.price";
    /* send the message (always) and retrive the response (if await is true) */
    SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into tickerPrice_ret_t type */
    return DESERIALIZE_FRAME(tickerPrice_ret_t, response);
}

std::optional<tickerBook_ret_t> binance_mech_data_t::tickerBook(
  tickerBook_args_t args, bool await) const { 
    const char* method = "ticker.book";
    /* send the message (always) and retrive the response (if await is true) */
    SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into tickerBook_ret_t type */
    return DESERIALIZE_FRAME(tickerBook_ret_t, response);
}

std::optional<tradesRecent_ret_t> binance_mech_data_t::tradesRecent(
  tradesRecent_args_t args, bool await) const { 
    const char* method = "trades.recent";
    /* send the message (always) and retrive the response (if await is true) */
    SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into tradesRecent_ret_t type */
    return DESERIALIZE_FRAME(tradesRecent_ret_t, response);
}

std::optional<tradesHistorical_ret_t> binance_mech_data_t::tradesHistorical(
  tradesHistorical_args_t args, bool await) const { 
    const char* method = "trades.historical";
    /* send the message (always) and retrive the response (if await is true) */
    SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into tradesHistorical_ret_t type */
    return DESERIALIZE_FRAME(tradesHistorical_ret_t, response);
}

} /* namespace binance */
} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
