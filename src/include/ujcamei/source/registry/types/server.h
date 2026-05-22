/* server.h */
#pragma once
#include "ujcamei/source/registry/types/utils.h"

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace registry {
namespace types {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct ping_args_t {
  std::string jsonify();
};
struct time_args_t {
  std::string jsonify();
};

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct ping_ret_t {
  frame_response_t frame_rsp;
  ping_ret_t(const std::string &json);
};
struct time_ret_t {
  frame_response_t frame_rsp;
  time_ret_t(const std::string &json);
  long serverTime;
};

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         deserialize specializations         */
/* --- --- --- --- --- --- --- --- --- --- --- */

void deserialize(ping_ret_t &deserialized, const std::string &json);
void deserialize(time_ret_t &deserialized, const std::string &json);

} /* namespace types */
} /* namespace registry */
} /* namespace source */
} /* namespace ujcamei */
} /* namespace cuwacunu */
