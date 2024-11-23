/* binance_mech_server.cpp */
#include "camahjucunu/exchange/binance/binance_mech_server.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {
namespace binance {

/* constructor */
binance_mech_server_t::binance_mech_server_t(
  cuwacunu::camahjucunu::curl::ws_session_id_t _session_id) 
    : owns_session(false), session_id(_session_id) {
  NOTIFY_INIT("cuwacunu::camahjucunu::mech::binance::binance_mech_server_t");
  ASSERT_SESSION(session_id, owns_session);
}

binance_mech_server_t::~binance_mech_server_t() {
  log_info("Finalizing cuwacunu::camahjucunu::mech::binance::binance_mech_server_t \n");
  if(owns_session) { cuwacunu::camahjucunu::curl::WebsocketAPI::ws_finalize(session_id); }
}

/* methods */
std::optional<ping_ret_t> binance_mech_server_t::ping(
  ping_args_t args, bool await) 
  const {
    const char* method = "ping";
    /* send the message (always) and retrive the response (if await is true) */
    SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into ping_ret_t type */
    return DESERIALIZE_FRAME(ping_ret_t, response);
}

std::optional<time_ret_t> binance_mech_server_t::time(
  time_args_t args, bool await) 
  const { 
    const char* method = "time";
    /* send the message (always) and retrive the response (if await is true) */
    SEND_AND_RETRIVE_FRAME(session_id, frame_id, method, args, await, response);
    /* deserialize the response into time_ret_t type */
    return DESERIALIZE_FRAME(time_ret_t, response);
}

} /* namespace binance */
} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
