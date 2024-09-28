#include "camahjucunu/exchange/exchange_types.h"
#include "camahjucunu/exchange/exchange_serialization.h"
#include "camahjucunu/exchange/exchange_deserialization.h"

RUNTIME_WARNING("(exchange_types.cpp)[] it is convinient (due to std::variant and std::optional) for this file to have c++20, libtorch uses c++17\n");

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
std::string                   ping_args_t::jsonify() { return "{}"; }
std::string                   time_args_t::jsonify() { return "{}"; }
std::string                  depth_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(limit)); }
std::string                 trades_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(limit)); }
std::string       tradesHistorical_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(limit), pairWrap(fromId)); }
std::string                 klines_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(interval), pairWrap(startTime), pairWrap(endTime), pairWrap(timeZone), pairWrap(limit)); }
std::string               avgPrice_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol)); }
std::string                 ticker_args_t::jsonify() { return jsonify_as_object( pairWrap_variant(symbol), pairWrap(windowSize), pairWrap(type)); }
std::string       tickerTradingDay_args_t::jsonify() { return jsonify_as_object( pairWrap_variant(symbol), pairWrap(timeZone), pairWrap(type)); }
std::string            tickerPrice_args_t::jsonify() { return jsonify_as_object( pairWrap_variant(symbol)); }
std::string       tickerBook_args_t::jsonify() { return jsonify_as_object( pairWrap_variant(symbol)); }

/* order args */
std::string           orderStatus_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(orderId), pairWrap(origClientOrderId), pairWrap(apiKey), pairWrap(recvWindow), pairWrap(signature), pairWrap(timestamp) ); }
std::string           orderMarket_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }

/* account args */
std::string    account_information_args_t::jsonify() { return jsonify_as_object( pairWrap(omitZeroBalances), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string  account_order_history_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(orderId), pairWrap(startTime), pairWrap(endTime), pairWrap(limit), pairWrap(apiKey), pairWrap(recvWindow), pairWrap(signature), pairWrap(timestamp)); }
std::string     account_trade_list_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(orderId), pairWrap(startTime), pairWrap(endTime), pairWrap(fromId), pairWrap(limit), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string query_commission_rates_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol)); }

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */
/* orders returns */
orderStatus_ret_t::orderStatus_ret_t                     (const std::string &json) { deserialize(*this, json); };
orderStatus_ret_t::orderStatus_ret_t                     () : frame_rsp(), symbol(""), orderId(0), orderListId(0), clientOrderId(""), price(0.0), origQty(0.0), executedQty(0.0), cummulativeQuoteQty(0.0), status(orderStatus_e::CANCELED), timeInForce(time_in_force_e::GTC), type(order_type_e::MARKET), side(order_side_e::BUY), stopPrice(0.0), icebergQty(0.0), time(0), updateTime(0), isWorking(false), workingTime(0), origQuoteOrderQty(0.0), selfTradePreventionMode(stp_modes_e::NONE), preventedMatchId(0), preventedQuantity(0.0), trailingDelta(0), trailingTime(0), strategyId(0), strategyType(0) {};
order_ack_ret_t::order_ack_ret_t                           (const std::string &json) { deserialize(*this, json); };
order_result_ret_t::order_result_ret_t                     (const std::string &json) { deserialize(*this, json); };
order_result_ret_t::order_result_ret_t                     () : symbol(""), orderId(0), orderListId(0), clientOrderId(""), transactTime(0), origQty(0.0), executedQty(0.0), cummulativeQuoteQty(0.0), status(orderStatus_e::REJECTED), timeInForce(time_in_force_e::FOK), type(order_type_e::MARKET), side(order_side_e::BUY), workingTime(0), selfTradePreventionMode(stp_modes_e::NONE) {};
order_full_ret_t::order_full_ret_t                         (const std::string &json) { deserialize(*this, json); };

/* primary return structs */
ping_ret_t::ping_ret_t                                     (const std::string &json) { deserialize(*this, json); };
time_ret_t::time_ret_t                                     (const std::string &json) { deserialize(*this, json); };
depth_ret_t::depth_ret_t                                   (const std::string &json) { deserialize(*this, json); };
tradesRecent_ret_t::tradesRecent_ret_t                     (const std::string &json) { deserialize(*this, json); };
tradesHistorical_ret_t::tradesHistorical_ret_t             (const std::string &json) { deserialize(*this, json); };
klines_ret_t::klines_ret_t                                 (const std::string &json) { deserialize(*this, json); };
avgPrice_ret_t::avgPrice_ret_t                             (const std::string &json) { deserialize(*this, json); };
tickerTradingDay_ret_t::tickerTradingDay_ret_t             (const std::string &json) { deserialize(*this, json); };
ticker_ret_t::ticker_ret_t                                 (const std::string &json) { deserialize(*this, json); };
tickerPrice_ret_t::tickerPrice_ret_t                       (const std::string &json) { deserialize(*this, json); };
tickerBook_ret_t::tickerBook_ret_t                         (const std::string &json) { deserialize(*this, json); };

/* account returns */
account_information_ret_t::account_information_ret_t       (const std::string &json) : commissionRates() { deserialize(*this, json); };
account_order_history_ret_t::account_order_history_ret_t   (const std::string &json) { deserialize(*this, json); };
account_trade_list_ret_t::account_trade_list_ret_t         (const std::string &json) { deserialize(*this, json); };
query_commission_rates_ret_t::query_commission_rates_ret_t (const std::string &json) { deserialize(*this, json); };

} /* namespace exchange */
} /* namespace cuwacunu */
} /* namespace camahjucunu */

// std::string             order_limit_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
// std::string         order_stop_loss_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
// std::string   order_stop_loss_limit_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
// std::string       order_take_profit_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
// std::string order_take_profit_limit_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
// std::string       order_limit_maker_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
// std::string               order_sor_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }

// order_sor_fill_t::order_sor_fill_t           (const std::string &json) { deserialize(*this, json); };
// order_sor_fill_t::order_sor_fill_t           () : matchType(""), price(0.0), qty(0.0), commission(0.0), commissionAsset(""), tradeId(0), allocId(0) {};
// order_sor_full_ret_t::order_sor_full_ret_t (const std::string &json) { deserialize(*this, json); };
