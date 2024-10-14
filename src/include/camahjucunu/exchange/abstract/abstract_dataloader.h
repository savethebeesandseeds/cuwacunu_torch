#pragma once
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         virtual exchange structure          */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct abstract_dataloader {
  virtual std::optional<depth_ret_t>            depth             ( depth_args_t args, bool await = true )  const = 0;
  virtual ~abstract_dataloader                  () {}             /* destructor */
};

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */