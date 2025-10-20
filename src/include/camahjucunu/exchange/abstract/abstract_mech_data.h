#pragma once
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         virtual exchange structure          */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct abstract_mech_data_t {
  virtual std::optional<depth_ret_t>            depth             (            depth_args_t args, bool await = true )  const = 0;
  virtual std::optional<tradesRecent_ret_t>     tradesRecent      (     tradesRecent_args_t args, bool await = true )  const = 0;
  virtual std::optional<tradesHistorical_ret_t> tradesHistorical  ( tradesHistorical_args_t args, bool await = true )  const = 0;
  virtual std::optional<klines_ret_t>           klines            (           klines_args_t args, bool await = true )  const = 0;
  virtual std::optional<avgPrice_ret_t>         avgPrice          (         avgPrice_args_t args, bool await = true )  const = 0;
  virtual std::optional<ticker_ret_t>           ticker            (           ticker_args_t args, bool await = true )  const = 0;
  virtual std::optional<tickerTradingDay_ret_t> ticker_tradingDay ( tickerTradingDay_args_t args, bool await = true )  const = 0;
  virtual std::optional<tickerPrice_ret_t>      tickerPrice       (      tickerPrice_args_t args, bool await = true )  const = 0;
  virtual std::optional<tickerBook_ret_t>       tickerBook        (       tickerBook_args_t args, bool await = true )  const = 0;
  virtual ~abstract_mech_data_t                  () {}             /* destructor */
};

} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
