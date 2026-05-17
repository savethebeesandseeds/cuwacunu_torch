/* types_server.h */
#pragma once
#include "camahjucunu/types/types_utils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct ping_args_t            { std::string jsonify(); };
struct time_args_t            { std::string jsonify(); };


/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct ping_ret_t             { frame_response_t frame_rsp; ping_ret_t              (const std::string& json); };
struct time_ret_t             { frame_response_t frame_rsp; time_ret_t              (const std::string& json); long serverTime; };


/* --- --- --- --- --- --- --- --- --- --- --- */
/*         deserialize specializations         */
/* --- --- --- --- --- --- --- --- --- --- --- */

void deserialize(ping_ret_t& deserialized, const std::string& json);
void deserialize(time_ret_t& deserialized, const std::string &json);

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
