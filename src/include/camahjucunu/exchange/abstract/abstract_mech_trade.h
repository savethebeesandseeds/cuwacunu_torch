#pragma once
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_trade.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         virtual exchange structure          */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct abstract_mech_trade_t {
  virtual std::optional<order_ret_t>                order                ( order_type_e type, order_args_t args, bool testOrder = false, bool await = true )  const = 0;
  virtual std::optional<orderStatus_ret_t>          orderStatus          ( orderStatus_args_t args, bool await = true )                                       const = 0;
  virtual std::optional<orderMarket_ret_t>          orderMarket          ( orderMarket_args_t args, bool testOrder = false, bool await = true )               const = 0;
  // virtual std::optional<orderLimit_ret_t>           orderLimit           ( orderLimit_args_t args, bool testOrder = false, bool await = true )                const = 0;
  // virtual std::optional<orderStopLoss_ret_t>        orderStopLoss        ( orderStopLoss_args_t args, bool testOrder = false, bool await = true )             const = 0;
  // virtual std::optional<orderStopLossLimit_ret_t>   orderStopLossLimit   ( orderStopLossLimit_args_t args, bool testOrder = false, bool await = true )        const = 0;
  // virtual std::optional<orderTakeProfit_ret_t>      orderTakeProfit      ( orderTakeProfit_args_t args, bool testOrder = false, bool await = true )           const = 0;
  // virtual std::optional<orderTakeProfitLimit_ret_t> orderTakeProfitLimit ( orderTakeProfitLimit_args_t args, bool testOrder = false, bool await = true )      const = 0;
  // virtual std::optional<orderLimitMaker_ret_t>      orderLimitMaker      ( orderLimitMaker_args_t args, bool testOrder = false, bool await = true )           const = 0;
  // virtual std::optional<orderSor_ret_t>             orderSor             ( orderSor_args_t args, bool testOrder = false, bool await = true )                  const = 0;  
  virtual ~abstract_mech_trade_t                     () {}                /* destructor */
};

} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
