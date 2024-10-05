#include "camahjucunu/exchange/exchange_types_trade.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
std::string           orderStatus_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(orderId), pairWrap(origClientOrderId), pairWrap(apiKey), pairWrap(recvWindow), pairWrap(signature), pairWrap(timestamp) ); }
std::string           orderMarket_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(apiKey), pairWrap(signature), pairWrap(timestamp) ); }

/* --- --- --- --- --- --- --- --- --- --- --- */
/*            signature methods                */
/* --- --- --- --- --- --- --- --- --- --- --- */
void orderStatus_args_t::add_signature() {
    /* fill the missing fileds */
    apiKey = cuwacunu::piaabo::dsecurity::SecureVault.which_api_key();
    timestamp = cuwacunu::camahjucunu::exchange::getUnixTimestampMillis();
    /* generate the payload string (alphabetically by key) */
    std::string dmessage = "";
    dmessage += field_signature( pairWrap(apiKey));
    dmessage += field_signature( pairWrap(orderId));
    dmessage += field_signature( pairWrap(origClientOrderId));
    dmessage += field_signature( pairWrap(recvWindow));
    dmessage += field_signature( pairWrap(symbol));
    dmessage += field_signature( pairWrap(timestamp));
    finalize_signature(dmessage);
    /* sign and add signature */
    signature = cuwacunu::piaabo::dsecurity::SecureVault.Ed25519_signMessage(dmessage);
}

void orderMarket_args_t::add_signature() {
    /* fill the missing fileds */
    apiKey = cuwacunu::piaabo::dsecurity::SecureVault.which_api_key();
    timestamp = cuwacunu::camahjucunu::exchange::getUnixTimestampMillis();
    /* generate the payload string (alphabetically by key) */
    std::string dmessage = "";
    dmessage += field_signature( pairWrap(apiKey));
    dmessage += field_signature( pairWrap(icebergQty));
    dmessage += field_signature( pairWrap(newClientOrderId));
    dmessage += field_signature( pairWrap(newOrderRespType));
    dmessage += field_signature( pairWrap(price));
    dmessage += field_signature( pairWrap(quantity));
    dmessage += field_signature( pairWrap(quoteOrderQty));
    dmessage += field_signature( pairWrap(recvWindow));
    dmessage += field_signature( pairWrap(selfTradePreventionMode));
    dmessage += field_signature( pairWrap(side));
    dmessage += field_signature( pairWrap(strategyId));
    dmessage += field_signature( pairWrap(strategyType));
    dmessage += field_signature( pairWrap(stopPrice));
    dmessage += field_signature( pairWrap(symbol));
    dmessage += field_signature( pairWrap(timeInForce));
    dmessage += field_signature( pairWrap(timestamp));
    dmessage += field_signature( pairWrap(trailingDelta));
    dmessage += field_signature( pairWrap(type));
    finalize_signature(dmessage);
    /* sign and add signature */
    signature = cuwacunu::piaabo::dsecurity::SecureVault.Ed25519_signMessage(dmessage);
}

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */
/* orders returns */
orderStatus_ret_t::orderStatus_ret_t                       (const std::string &json) { deserialize(*this, json); };
orderStatus_ret_t::orderStatus_ret_t                       () : frame_rsp(), symbol(""), orderId(0), orderListId(0), clientOrderId(""), price(0.0), origQty(0.0), executedQty(0.0), cummulativeQuoteQty(0.0), status(orderStatus_e::CANCELED), timeInForce(time_in_force_e::GTC), type(order_type_e::MARKET), side(order_side_e::BUY), stopPrice(0.0), icebergQty(0.0), time(0), updateTime(0), isWorking(false), workingTime(0), origQuoteOrderQty(0.0), selfTradePreventionMode(stp_modes_e::NONE), preventedMatchId(0), preventedQuantity(0.0), trailingDelta(0), trailingTime(0), strategyId(0), strategyType(0) {};
order_ack_ret_t::order_ack_ret_t                           (const std::string &json) { deserialize(*this, json); };
order_result_ret_t::order_result_ret_t                     (const std::string &json) { deserialize(*this, json); };
order_result_ret_t::order_result_ret_t                     () : symbol(""), orderId(0), orderListId(0), clientOrderId(""), transactTime(0), origQty(0.0), executedQty(0.0), cummulativeQuoteQty(0.0), status(orderStatus_e::REJECTED), timeInForce(time_in_force_e::FOK), type(order_type_e::MARKET), side(order_side_e::BUY), workingTime(0), selfTradePreventionMode(stp_modes_e::NONE) {};
order_full_ret_t::order_full_ret_t                         (const std::string &json) { deserialize(*this, json); };

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         deserialize specializations         */
/* --- --- --- --- --- --- --- --- --- --- --- */

void deserialize(orderStatus_ret_t& deserialized, const std::string& json) {
  /*
    {
      "id": "aa62318a-5a97-4f3b-bdc7-640bbe33b291",
      "status": 200,
      "result": {
        "symbol": "BTCUSDT",
        "orderId": 12569099453,
        "orderListId": -1,                  // set only for orders of an order list
        "clientOrderId": "4d96324ff9d44481926157",
        "price": "23416.10000000",
        "origQty": "0.00847000",
        "executedQty": "0.00847000",
        "cummulativeQuoteQty": "198.33521500",
        "status": "FILLED",
        "timeInForce": "GTC",
        "type": "LIMIT",
        "side": "SELL",
        "stopPrice": "0.00000000",          // always present, zero if order type does not use stopPrice
        "trailingDelta": 10,                // present only if trailingDelta set for the order
        "trailingTime": -1,                 // present only if trailingDelta set for the order
        "icebergQty": "0.00000000",         // always present, zero for non-iceberg orders
        "time": 1660801715639,              // time when the order was placed
        "updateTime": 1660801717945,        // time of the last update to the order
        "isWorking": true,
        "workingTime": 1660801715639,
        "origQuoteOrderQty": "0.00000000"   // always present, zero if order type does not use quoteOrderQty
        "strategyId": 37463720,             // present only if strategyId set for the order
        "strategyType": 1000000,            // present only if strategyType set for the order
        "selfTradePreventionMode": "NONE",
        "preventedMatchId": 0,              // present only if the order expired due to STP
        "preventedQuantity": "1.200000"     // present only if the order expired due to STP
      },
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 4
        }
      ]
    }
  */
  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  RETRIVE_OBJECT(root_obj, "result", result_obj);
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);

  /* result fields */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized, result_obj, "symbol", symbol);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "orderId", orderId, long);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "orderListId", orderListId, int);
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized, result_obj, "clientOrderId", clientOrderId);
  ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized, result_obj, "price", price);
  ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized, result_obj, "origQty", origQty);
  ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized, result_obj, "executedQty", executedQty);
  ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized, result_obj, "cummulativeQuoteQty", cummulativeQuoteQty);
  ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized, result_obj, "status", status, orderStatus_e);
  ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized, result_obj, "timeInForce", timeInForce, time_in_force_e);
  ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized, result_obj, "type", type, order_type_e);
  ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized, result_obj, "side", side, order_side_e);
  ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized, result_obj, "stopPrice", stopPrice);
  ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized, result_obj, "icebergQty", icebergQty);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "time", time, long);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "updateTime", updateTime, long);
  ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(deserialized, result_obj, "isWorking", isWorking);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "workingTime", workingTime, long);
  ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized, result_obj, "origQuoteOrderQty", origQuoteOrderQty);
  ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized, result_obj, "selfTradePreventionMode", selfTradePreventionMode, stp_modes_e);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "preventedMatchId", preventedMatchId, int);
  ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized, result_obj, "preventedQuantity", preventedQuantity);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "trailingDelta", trailingDelta, long);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "trailingTime", trailingTime, long);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "strategyId", strategyId, int);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "strategyType", strategyType, int);
}

void deserialize(order_ack_ret_t& deserialized, const std::string& json) {
  /*
    {
      "id": "56374a46-3061-486b-a311-99ee972eb648",
      "status": 200,
      "result": {
        "symbol": "BTCUSDT",
        "orderId": 12569099453,
        "orderListId": -1, // always -1 for singular orders
        "clientOrderId": "4d96324ff9d44481926157ec08158a40",
        "transactTime": 1660801715639
      },
      "rateLimits": [
        {
          "rateLimitType": "ORDERS",
          "interval": "SECOND",
          "intervalNum": 10,
          "limit": 50,
          "count": 1
        },
        {
          "rateLimitType": "ORDERS",
          "interval": "DAY",
          "intervalNum": 1,
          "limit": 160000,
          "count": 1
        },
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 1
        }
      ]
    }
  */

  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  RETRIVE_OBJECT(root_obj, "result", result_obj);
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);

  /* result fields */
  {
    ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized, result_obj, "symbol", symbol);
    ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized, result_obj, "clientOrderId", clientOrderId);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "transactTime", transactTime, long);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "orderId", orderId, int);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "orderListId", orderListId, int);
  }
}

void deserialize(order_result_ret_t& deserialized, const std::string& json) {
  /*
    {
      "id": "56374a46-3061-486b-a311-99ee972eb648",
      "status": 200,
      "result": {
        "symbol": "BTCUSDT",
        "orderId": 12569099453,
        "orderListId": -1, // always -1 for singular orders
        "clientOrderId": "4d96324ff9d44481926157ec08158a40",
        "transactTime": 1660801715639,
        "price": "23416.10000000",
        "origQty": "0.00847000",
        "executedQty": "0.00000000",
        "cummulativeQuoteQty": "0.00000000",
        "status": "NEW",
        "timeInForce": "GTC",
        "type": "LIMIT",
        "side": "SELL",
        "workingTime": 1660801715639,
        "selfTradePreventionMode": "NONE"
      },
      "rateLimits": [
        {
          "rateLimitType": "ORDERS",
          "interval": "SECOND",
          "intervalNum": 10,
          "limit": 50,
          "count": 1
        },
        {
          "rateLimitType": "ORDERS",
          "interval": "DAY",
          "intervalNum": 1,
          "limit": 160000,
          "count": 1
        },
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000
    ,
          "count": 1
        }
      ]
    }
  */
  
  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  RETRIVE_OBJECT(root_obj, "result", result_obj);
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);

  /* result fields */
  {
    ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized, result_obj, "symbol", symbol);
    ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized, result_obj, "clientOrderId", clientOrderId);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "orderId", orderId, int);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "orderListId", orderListId, int);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "transactTime", transactTime, long);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "origQty", origQty, double);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "executedQty", executedQty, double);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "cummulativeQuoteQty", cummulativeQuoteQty, double);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "workingTime", workingTime, long);

    ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized, result_obj, "status", status, orderStatus_e);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized, result_obj, "timeInForce", timeInForce, time_in_force_e);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized, result_obj, "type", type, order_type_e);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized, result_obj, "side", side, order_side_e);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized, result_obj, "selfTradePreventionMode", selfTradePreventionMode, stp_modes_e);
  }
}

void deserialize(order_full_ret_t& deserialized, const std::string& json) {
  /*
    {
      "id": "56374a46-3061-486b-a311-99ee972eb648",
      "status": 200,
      "result": {
        "symbol": "BTCUSDT",
        "orderId": 12569099453,
        "orderListId": -1,
        "clientOrderId": "4d96324ff9d44481926157ec08158a40",
        "transactTime": 1660801715793,
        "price": "23416.10000000",
        "origQty": "0.00847000",
        "executedQty": "0.00847000",
        "cummulativeQuoteQty": "198.33521500",
        "status": "FILLED",
        "timeInForce": "GTC",
        "type": "LIMIT",
        "side": "SELL",
        "workingTime": 1660801715793,
        // FULL response is identical to RESULT response, with the same optional fields
        // based on the order type and parameters. FULL response additionally includes
        // the list of trades which immediately filled the order.
        "fills": [
          {
            "price": "23416.10000000",
            "qty": "0.00635000",
            "commission": "0.000000",
            "commissionAsset": "BNB",
            "tradeId": 1650422481
          },
          {
            "price": "23416.50000000",
            "qty": "0.00212000",
            "commission": "0.000000",
            "commissionAsset": "BNB",
            "tradeId": 1650422482
          }
        ]
      },
      "rateLimits": [
        {
          "rateLimitType": "ORDERS",
          "interval": "SECOND",
          "intervalNum": 10,
          "limit": 50,
          "count": 1
        },
        {
          "rateLimitType": "ORDERS",
          "interval": "DAY",
          "intervalNum": 1,
          "limit": 160000,
          "count": 1
        },
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 1
        }
      ]
    }
  */
  
  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  
  RETRIVE_OBJECT(root_obj, "result", result_obj);

  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);


  /* result fields */
  {
    ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.result, result_obj, "symbol", symbol);
    ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.result, result_obj, "clientOrderId", clientOrderId);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.result, result_obj, "orderId", orderId, int);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.result, result_obj, "orderListId", orderListId, int);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.result, result_obj, "transactTime", transactTime, long);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.result, result_obj, "origQty", origQty, double);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.result, result_obj, "executedQty", executedQty, double);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.result, result_obj, "cummulativeQuoteQty", cummulativeQuoteQty, double);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.result, result_obj, "workingTime", workingTime, long);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized.result, result_obj, "status", status, orderStatus_e);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized.result, result_obj, "timeInForce", timeInForce, time_in_force_e);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized.result, result_obj, "type", type, order_type_e);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized.result, result_obj, "side", side, order_side_e);
    deserialized.result.selfTradePreventionMode = stp_modes_e::NONE; /* not present in the request */
  }
  
  {
    RETRIVE_ARRAY(result_obj, "fills", fills_arr);
    ALLOCATE_VECT(deserialized.fills, fills_arr.size());
    for (auto& fillEntry : fills_arr) {
      order_fill_t tmp;
      ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, fillEntry, "commissionAsset", commissionAsset);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, fillEntry, "price", price);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, fillEntry, "qty", qty);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, fillEntry, "commission", commission);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, fillEntry, "tradeId", tradeId, int);
      deserialized.fills.push_back(std::move(tmp));
    }
  }
}

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
