#pragma once
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/exchange/exchange_types.h"
#include "camahjucunu/exchange/exchange_enums.h"

RUNTIME_WARNING("(exchange_mech.h)[] remmember the server endpoint is asynchonous.\n");
RUNTIME_WARNING("(exchange_mech.h)[] implement a count on the api usage limit.\n");

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         virtual exchange structure          */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct exchange_mech_t {
  virtual std::optional<ping_ret_t>                    ping                   (                   ping_args_t args, bool await = true )  const = 0;
  virtual std::optional<time_ret_t>                    time                   (                   time_args_t args, bool await = true )  const = 0;
  virtual std::optional<depth_ret_t>                   depth                  (                  depth_args_t args, bool await = true )  const = 0;
  virtual std::optional<tradesRecent_ret_t>            trades                 (                 trades_args_t args, bool await = true )  const = 0;
  virtual std::optional<tradesHistorical_ret_t>        tradesHistorical       (       tradesHistorical_args_t args, bool await = true )  const = 0;
  virtual std::optional<klines_ret_t>                  klines                 (                 klines_args_t args, bool await = true )  const = 0;
  virtual std::optional<avgPrice_ret_t>                avgPrice               (               avgPrice_args_t args, bool await = true )  const = 0;
  virtual std::optional<ticker_ret_t>                  ticker                 (                 ticker_args_t args, bool await = true )  const = 0;
  virtual std::optional<tickerTradingDay_ret_t>        ticker_tradingDay      (       tickerTradingDay_args_t args, bool await = true )  const = 0;
  virtual std::optional<tickerPrice_ret_t>             tickerPrice            (            tickerPrice_args_t args, bool await = true )  const = 0;
  virtual std::optional<tickerBook_ret_t>              tickerBook             (       tickerBook_args_t args, bool await = true )  const = 0;
  virtual std::optional<orderStatus_ret_t>             orderStatus            (            orderStatus_args_t args, bool await = true )  const = 0;
  virtual std::optional<orderMarket_ret_t>             orderMarket            (            orderMarket_args_t args, bool await = true )  const = 0;
  virtual std::optional<account_information_ret_t>     account_information    (    account_information_args_t args, bool await = true )  const = 0;
  virtual std::optional<account_order_history_ret_t>   account_order_history  (  account_order_history_args_t args, bool await = true )  const = 0;
  virtual std::optional<account_trade_list_ret_t>      account_trade_list     (     account_trade_list_args_t args, bool await = true )  const = 0;
  virtual std::optional<query_commission_rates_ret_t>  query_commission_rates ( query_commission_rates_args_t args, bool await = true )  const = 0;
  virtual ~exchange_mech_t              () {} /* destructor */
};

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */

// virtual order_limit_ret_t             order_limit             (             order_limit_args_t args )  const = 0;
// virtual order_stop_loss_ret_t         order_stop_loss         (         order_stop_loss_args_t args )  const = 0;
// virtual order_stop_loss_limit_ret_t   order_stop_loss_limit   (   order_stop_loss_limit_args_t args )  const = 0;
// virtual order_take_profit_ret_t       order_take_profit       (       order_take_profit_args_t args )  const = 0;
// virtual order_take_profit_limit_ret_t order_take_profit_limit ( order_take_profit_limit_args_t args )  const = 0;
// virtual order_limit_maker_ret_t       order_limit_maker       (       order_limit_maker_args_t args )  const = 0;
// virtual order_sor_ret_t               order_sor               (               order_sor_args_t args )  const = 0;
