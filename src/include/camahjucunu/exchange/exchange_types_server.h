#pragma once
#include "camahjucunu/exchange/exchange_utils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct ping_args_t            { std::string jsonify(); };
struct time_args_t            { std::string jsonify(); };
ENFORCE_ARCHITECTURE_DESIGN(             ping_args_t);
ENFORCE_ARCHITECTURE_DESIGN(             time_args_t);


/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct ping_ret_t             { frame_response_t frame_rsp; ping_ret_t              (const std::string& json); };
struct time_ret_t             { frame_response_t frame_rsp; time_ret_t              (const std::string& json); long serverTime; };
ENFORCE_ARCHITECTURE_DESIGN(             ping_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(             time_ret_t);


/* --- --- --- --- --- --- --- --- --- --- --- */
/*         deserialize specializations         */
/* --- --- --- --- --- --- --- --- --- --- --- */

void deserialize(ping_ret_t& deserialized, const std::string& json);
void deserialize(time_ret_t& deserialized, const std::string &json);

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
