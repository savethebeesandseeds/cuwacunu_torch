#include "camahjucunu/crypto_exchange/binance/binance_types.h"
#include "camahjucunu/crypto_exchange/binance/binance_serialization.h"
#include "camahjucunu/crypto_exchange/binance/binance_deserialization.h"

RUNTIME_WARNING("(binance_types.cpp)[] it is convinient (due to std::variant and std::optional) for this file to have c++20, libtorch uses c++17\n");

namespace cuwacunu {
namespace camahjucunu {
namespace binance {
/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
std::string                    ping_args_t::jsonify() { return "{}"; }
std::string                    time_args_t::jsonify() { return "{}"; }
std::string                   depth_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(limit)); }
std::string                  trades_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(limit)); }
std::string        historicalTrades_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(limit), pairWrap(fromId)); }
std::string                  klines_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(interval), pairWrap(startTime), pairWrap(endTime), pairWrap(timeZone), pairWrap(limit)); }
std::string                avgPrice_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol)); }
std::string             ticker_24hr_args_t::jsonify() { return jsonify_as_object(pairWrap(symbols), pairWrap(type)); }
std::string       ticker_tradingDay_args_t::jsonify() { return jsonify_as_object(pairWrap(symbols), pairWrap(timeZone), pairWrap(type)); }
std::string            ticker_price_args_t::jsonify() { return jsonify_as_object(pairWrap(symbols)); }
std::string       ticker_bookTicker_args_t::jsonify() { return jsonify_as_object(pairWrap(symbols)); }
std::string             ticker_wind_args_t::jsonify() { return jsonify_as_object(pairWrap(symbols), pairWrap(windowSize), pairWrap(type)); }
std::string             order_limit_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string            order_market_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string         order_stop_loss_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string   order_stop_loss_limit_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string       order_take_profit_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string order_take_profit_limit_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string       order_limit_maker_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string               order_sor_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string     account_information_args_t::jsonify() { return jsonify_as_object(pairWrap(omitZeroBalances), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string      account_trade_list_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(orderId), pairWrap(startTime), pairWrap(endTime), pairWrap(fromId), pairWrap(limit), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string   query_commision_rates_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol)); }

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */
/* secondary return structs */
price_qty_t::price_qty_t                     (const std::string &json) { deserialize(*this, json); };
trade_t::trade_t                             (const std::string &json) { deserialize(*this, json); };
kline_t::kline_t                             (const std::string &json) { deserialize(*this, json); };
tick_full_t::tick_full_t                     (const std::string &json) { deserialize(*this, json); };
tick_mini_t::tick_mini_t                     (const std::string &json) { deserialize(*this, json); };
price_t::price_t                             (const std::string &json) { deserialize(*this, json); };
bookPrice_t::bookPrice_t                     (const std::string &json) { deserialize(*this, json); };
commissionRates_t::commissionRates_t         () : maker(0.0), taker(0.0), buyer(0.0), seller(0.0) {};
commissionRates_t::commissionRates_t         (const std::string &json) { deserialize(*this, json); };
balance_t::balance_t                         (const std::string &json) { deserialize(*this, json); };
historicTrade_t::historicTrade_t             (const std::string &json) { deserialize(*this, json); };
comission_discount_t::comission_discount_t   () : enabledForAccount(false), enabledForSymbol(false), discountAsset(""), discount(0.0) {};
comission_discount_t::comission_discount_t   (const std::string &json) { deserialize(*this, json); };
order_ack_resp_t::order_ack_resp_t           (const std::string &json) { deserialize(*this, json); };
order_result_resp_t::order_result_resp_t     (const std::string &json) { deserialize(*this, json); };
order_result_resp_t::order_result_resp_t     () : symbol(""), orderId(0), orderListId(0), clientOrderId(""), transactTime(0), origQty(0.0), executedQty(0.0), cummulativeQuoteQty(0.0), status(order_status_e::REJECTED), timeInForce(time_in_force_e::FOK), type(order_type_e::MARKET), side(order_side_e::BUY), workingTime(0), selfTradePreventionMode(stp_modes_e::NONE) {};
order_fill_t::order_fill_t                   (const std::string &json) { deserialize(*this, json); };
order_fill_t::order_fill_t                   () : price(0.0), qty(0.0), commission(0.0), commissionAsset(0), tradeId(0) {};
order_full_resp_t::order_full_resp_t         (const std::string &json) { deserialize(*this, json); };
order_sor_fill_t::order_sor_fill_t           (const std::string &json) { deserialize(*this, json); };
order_sor_fill_t::order_sor_fill_t           () : matchType(""), price(0.0), qty(0.0), commission(0.0), commissionAsset(""), tradeId(0), allocId(0) {};
order_sor_full_resp_t::order_sor_full_resp_t (const std::string &json) { deserialize(*this, json); };

/* primary return structs */
ping_ret_t::ping_ret_t                                   (const std::string &json) { deserialize(*this, json); };
time_ret_t::time_ret_t                                   (const std::string &json) { deserialize(*this, json); };
depth_ret_t::depth_ret_t                                 (const std::string &json) { deserialize(*this, json); };
trades_ret_t::trades_ret_t                               (const std::string &json) { deserialize(*this, json); };
historicalTrades_ret_t::historicalTrades_ret_t           (const std::string &json) { deserialize(*this, json); };
klines_ret_t::klines_ret_t                               (const std::string &json) { deserialize(*this, json); };
avgPrice_ret_t::avgPrice_ret_t                           (const std::string &json) { deserialize(*this, json); };
ticker_24hr_ret_t::ticker_24hr_ret_t                     (const std::string &json) { deserialize(*this, json); };
ticker_price_ret_t::ticker_price_ret_t                   (const std::string &json) { deserialize(*this, json); };
ticker_bookTicker_ret_t::ticker_bookTicker_ret_t         (const std::string &json) { deserialize(*this, json); };
ticker_wind_ret_t::ticker_wind_ret_t                     (const std::string &json) { deserialize(*this, json); };
account_information_ret_t::account_information_ret_t     (const std::string &json) : commissionRates() { deserialize(*this, json); };
account_trade_list_ret_t::account_trade_list_ret_t       (const std::string &json) { deserialize(*this, json); };
query_commision_rates_ret_t::query_commision_rates_ret_t (const std::string &json) : standardCommission(), taxCommission(), discount() { deserialize(*this, json); };

} /* namespace binance */
} /* namespace cuwacunu */
} /* namespace camahjucunu */