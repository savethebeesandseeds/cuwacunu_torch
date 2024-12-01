#pragma once
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_server.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         virtual exchange structure          */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct abstract_mech_server_t {
  virtual std::optional<ping_ret_t> ping ( ping_args_t args, bool await = true )  const = 0;
  virtual std::optional<time_ret_t> time ( time_args_t args, bool await = true )  const = 0;
  virtual ~abstract_mech_server_t    () {} /* destructor */
};

} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
