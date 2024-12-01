/* exchange_types_trade.h */
#pragma once
#include "camahjucunu/exchange/exchange_utils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct orderStatus_args_t  { void add_signature(); std::string jsonify(); std::string symbol; std::optional<int> orderId = std::nullopt; std::optional<std::string> origClientOrderId = std::nullopt; std::optional<long> recvWindow; std::string apiKey; std::string signature; long timestamp; };
struct orderMarket_args_t  { void add_signature(); std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; std::optional<double> quantity = std::nullopt; std::optional<double> quoteOrderQty = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; std::string apiKey; std::string signature; long timestamp; };
ENFORCE_ARCHITECTURE_DESIGN(      orderStatus_args_t);
ENFORCE_ARCHITECTURE_DESIGN(      orderMarket_args_t);
using order_args_t        = std::variant<orderMarket_args_t>;

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */

/* secondary return structs */
struct order_fill_t        { double price; double qty; double commission; std::string commissionAsset; int tradeId; };
struct order_ack_ret_t     { frame_response_t frame_rsp; order_ack_ret_t(const std::string& json); std::string symbol; int orderId; int orderListId; std::string clientOrderId; long transactTime; };
struct order_result_ret_t  { frame_response_t frame_rsp; order_result_ret_t(); order_result_ret_t(const std::string& json); std::string symbol; int orderId; int orderListId; std::string clientOrderId; long transactTime; double origQty; double executedQty; double cummulativeQuoteQty; orderStatus_e status; time_in_force_e timeInForce; order_type_e type; order_side_e side; long workingTime; stp_modes_e selfTradePreventionMode; };
struct order_full_ret_t    { frame_response_t frame_rsp; order_full_ret_t(const std::string& json); order_result_ret_t result; std::vector<order_fill_t> fills; };
ENFORCE_ARCHITECTURE_DESIGN( order_fill_t);
ENFORCE_ARCHITECTURE_DESIGN(    order_ack_ret_t);
ENFORCE_ARCHITECTURE_DESIGN( order_result_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(   order_full_ret_t);

/* primary returns */
struct orderStatus_ret_t   { frame_response_t frame_rsp; orderStatus_ret_t(); orderStatus_ret_t(const std::string& json); std::string symbol; long orderId; int orderListId; std::string clientOrderId; double price; double origQty; double executedQty; double cummulativeQuoteQty; orderStatus_e status; time_in_force_e timeInForce; order_type_e type; order_side_e side; double stopPrice; double icebergQty; long time; long updateTime; bool isWorking; long workingTime; double origQuoteOrderQty; stp_modes_e selfTradePreventionMode; int preventedMatchId; double preventedQuantity; long trailingDelta; long trailingTime; int strategyId; int strategyType; };
ENFORCE_ARCHITECTURE_DESIGN(  orderStatus_ret_t);
using orderMarket_ret_t    = std::variant<order_ack_ret_t, order_full_ret_t, order_result_ret_t>;
using order_ret_t          = std::variant<orderMarket_ret_t>;

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         deserialize specializations         */
/* --- --- --- --- --- --- --- --- --- --- --- */

void deserialize(orderStatus_ret_t& deserialized, const std::string& json);
void deserialize(order_ack_ret_t& deserialized, const std::string& json);
void deserialize(order_result_ret_t& deserialized, const std::string& json);
void deserialize(order_full_ret_t& deserialized, const std::string& json);

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */

// struct orderLimit_args_t            { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct orderStopLoss_args_t         { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; double quantity; std::optional<double> quoteOrderQty = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct orderStopLoss_limit_args_t   { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; double stopPrice; long trailingDelta; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct orderTakeProfit_args_t       { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; double quantity; std::optional<double> quoteOrderQty = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; double stopPrice; long trailingDelta; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct orderTakeProfit_limit_args_t { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; double stopPrice; long trailingDelta; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct orderLimitMaker_args_t       { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct orderSor_args_t              { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; std::optional<double> quantity = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// using order_args_t                  = std::variant<orderLimit_args_t, orderMarket_args_t, orderStopLoss_args_t, orderStopLoss_limit_args_t, orderTakeProfit_args_t, orderTakeProfit_limit_args_t, orderLimitMaker_args_t, orderSor_args_t>;
// ENFORCE_ARCHITECTURE_DESIGN(             orderLimit_args_t);
// ENFORCE_ARCHITECTURE_DESIGN(         orderStopLoss_args_t);
// ENFORCE_ARCHITECTURE_DESIGN(   orderStopLoss_limit_args_t);
// ENFORCE_ARCHITECTURE_DESIGN(       orderTakeProfit_args_t);
// ENFORCE_ARCHITECTURE_DESIGN( orderTakeProfit_limit_args_t);
// ENFORCE_ARCHITECTURE_DESIGN(       orderLimitMaker_args_t);
// ENFORCE_ARCHITECTURE_DESIGN(               orderSor_args_t);

// struct orderSor_fill_t      { orderSor_fill_t(); orderSor_fill_t(const std::string& json); std::string matchType; double price; double qty; double commission; std::string commissionAsset; int tradeId; int allocId; };
// struct orderSor_full_ret_t { orderSor_full_ret_t(const std::string& json); order_result_ret_t result; double price; allocation_type_e workingFloor; bool usedSor; std::vector<orderSor_fill_t> fills; };
// ENFORCE_ARCHITECTURE_DESIGN(      orderSor_fill_t);
// ENFORCE_ARCHITECTURE_DESIGN( orderSor_full_ret_t);
// using orderLimit_ret_t             = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using orderStopLoss_ret_t         = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using orderStopLoss_limit_ret_t   = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using orderTakeProfit_ret_t       = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using orderTakeProfit_limit_ret_t = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using orderLimitMaker_ret_t       = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using orderSor_ret_t               = std::variant<orderSor_full_ret_t>;
// using order_ret_t                   = std::variant<orderLimit_ret_t, orderMarket_ret_t, orderStopLoss_ret_t, orderStopLoss_limit_ret_t, orderTakeProfit_ret_t, orderTakeProfit_limit_ret_t, orderLimitMaker_ret_t, orderSor_ret_t>;
