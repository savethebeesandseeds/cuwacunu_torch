#pragma once
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_server.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

RUNTIME_WARNING("(exchange_mech_server.h)[] implement a count on the api usage limit.\n");
RUNTIME_WARNING("(exchange_mech_server.h)[] remmember the server endpoint is asynchonous.\n");
RUNTIME_WARNING("(exchange_mech_server.h)[] slipt mech into implementation files .cpp.\n");

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         virtual exchange structure          */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct virtual_mech_server_t {
  virtual std::optional<ping_ret_t> ping ( ping_args_t args, bool await = true )  const = 0;
  virtual std::optional<time_ret_t> time ( time_args_t args, bool await = true )  const = 0;
  virtual ~virtual_mech_server_t    () {} /* destructor */
};

} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
