#include "camahjucunu/exchange/exchange_types_account.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
std::string      account_information_args_t::jsonify() { return jsonify_as_object( pairWrap(omitZeroBalances), pairWrap(recvWindow), pairWrap(apiKey), pairWrap(signature), pairWrap(timestamp)); }
std::string    account_order_history_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(orderId), pairWrap(startTime), pairWrap(endTime), pairWrap(limit), pairWrap(recvWindow), pairWrap(apiKey), pairWrap(signature), pairWrap(timestamp)); }
std::string       account_trade_list_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(orderId), pairWrap(startTime), pairWrap(endTime), pairWrap(fromId), pairWrap(limit), pairWrap(recvWindow), pairWrap(apiKey), pairWrap(signature), pairWrap(timestamp)); }
std::string account_commission_rates_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(apiKey), pairWrap(signature), pairWrap(timestamp)); }

/* --- --- --- --- --- --- --- --- --- --- --- */
/*            signature methods                */
/* --- --- --- --- --- --- --- --- --- --- --- */
void account_information_args_t::add_signature() {
    /* fill the missing fileds */
    apiKey = cuwacunu::piaabo::dsecurity::SecureVault.which_api_key();
    timestamp = cuwacunu::camahjucunu::exchange::getUnixTimestampMillis();
    /* generate the payload string (alphabetically by key) */
    std::string dmessage = "";
    dmessage += field_signature( pairWrap(apiKey) );
    dmessage += field_signature( pairWrap(omitZeroBalances) );
    dmessage += field_signature( pairWrap(recvWindow) );
    dmessage += field_signature( pairWrap(timestamp) );
    finalize_signature(dmessage);
    /* sign and add signature */
    signature = cuwacunu::piaabo::dsecurity::SecureVault.Ed25519_signMessage(dmessage);
}

void account_order_history_args_t::add_signature() {
    /* fill the missing fileds */
    apiKey = cuwacunu::piaabo::dsecurity::SecureVault.which_api_key();
    timestamp = cuwacunu::camahjucunu::exchange::getUnixTimestampMillis();
    /* generate the payload string (alphabetically by key) */
    std::string dmessage = "";
    dmessage += field_signature( pairWrap(apiKey));
    dmessage += field_signature( pairWrap(endTime));
    dmessage += field_signature( pairWrap(limit));
    dmessage += field_signature( pairWrap(orderId));
    dmessage += field_signature( pairWrap(recvWindow));
    dmessage += field_signature( pairWrap(startTime));
    dmessage += field_signature( pairWrap(symbol));
    dmessage += field_signature( pairWrap(timestamp));
    finalize_signature(dmessage);
    /* sign and add signature */
    signature = cuwacunu::piaabo::dsecurity::SecureVault.Ed25519_signMessage(dmessage);
}

void account_trade_list_args_t::add_signature() {
    /* fill the missing fileds */
    apiKey = cuwacunu::piaabo::dsecurity::SecureVault.which_api_key();
    timestamp = cuwacunu::camahjucunu::exchange::getUnixTimestampMillis();
    /* generate the payload string (alphabetically by key) */
    std::string dmessage = "";
    dmessage += field_signature( pairWrap(apiKey));
    dmessage += field_signature( pairWrap(endTime));
    dmessage += field_signature( pairWrap(fromId));
    dmessage += field_signature( pairWrap(limit));
    dmessage += field_signature( pairWrap(orderId));
    dmessage += field_signature( pairWrap(recvWindow));
    dmessage += field_signature( pairWrap(startTime));
    dmessage += field_signature( pairWrap(symbol));
    dmessage += field_signature( pairWrap(timestamp));
    finalize_signature(dmessage);
    /* sign and add signature */
    signature = cuwacunu::piaabo::dsecurity::SecureVault.Ed25519_signMessage(dmessage);
}

void account_commission_rates_args_t::add_signature() {
    /* fill the missing fileds */
    apiKey = cuwacunu::piaabo::dsecurity::SecureVault.which_api_key();
    timestamp = cuwacunu::camahjucunu::exchange::getUnixTimestampMillis();
    /* generate the payload string (alphabetically by key) */
    std::string dmessage = "";
    dmessage += field_signature( pairWrap(apiKey));
    dmessage += field_signature( pairWrap(symbol));
    dmessage += field_signature( pairWrap(timestamp));
    finalize_signature(dmessage);
    /* sign and add signature */
    signature = cuwacunu::piaabo::dsecurity::SecureVault.Ed25519_signMessage(dmessage);
}

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */
account_information_ret_t::account_information_ret_t           (const std::string &json) : commissionRates() { deserialize(*this, json); };
account_order_history_ret_t::account_order_history_ret_t       (const std::string &json) { deserialize(*this, json); };
account_trade_list_ret_t::account_trade_list_ret_t             (const std::string &json) { deserialize(*this, json); };
account_commission_rates_ret_t::account_commission_rates_ret_t (const std::string &json) { deserialize(*this, json); };

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         deserialize specializations         */
/* --- --- --- --- --- --- --- --- --- --- --- */

void deserialize(account_information_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "605a6d20-6588-4cb9-afa0-b0ab087507ba",
      "status": 200,
      "result": {
        "makerCommission": 15,
        "takerCommission": 15,
        "buyerCommission": 0,
        "sellerCommission": 0,
        "canTrade": true,
        "canWithdraw": true,
        "canDeposit": true,
        "commissionRates": {
          "maker": "0.00150000",
          "taker": "0.00150000",
          "buyer": "0.00000000",
          "seller": "0.00000000"
        },
        "brokered": false,
        "requireSelfTradePrevention": false,
        "preventSor": false,
        "updateTime": 1660801833000,
        "accountType": "SPOT",
        "balances": [
          {
            "asset": "BNB",
            "free": "0.00000000",
            "locked": "0.00000000"
          },
          {
            "asset": "BTC",
            "free": "1.3447112",
            "locked": "0.08600000"
          },
          {
            "asset": "USDT",
            "free": "1021.21000000",
            "locked": "0.00000000"
          }
        ],
        "permissions": [
          "SPOT"
        ],
        "uid": 354937868
      },
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 20
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
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "makerCommission", makerCommission, int);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "takerCommission", takerCommission, int);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "buyerCommission", buyerCommission, int);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "sellerCommission", sellerCommission, int);
  
  ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(deserialized, result_obj, "canTrade", canTrade);
  ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(deserialized, result_obj, "canWithdraw", canWithdraw);
  ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(deserialized, result_obj, "canDeposit", canDeposit);
  ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(deserialized, result_obj, "brokered", brokered);
  ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(deserialized, result_obj, "requireSelfTradePrevention", requireSelfTradePrevention);
  ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(deserialized, result_obj, "preventSor", preventSor);
  
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "updateTime", updateTime, long);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "uid", uid, long);
  
  {
    RETRIVE_OBJECT(result_obj, "commissionRates", commissionRates_obj);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionRates, commissionRates_obj, "maker", maker);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionRates, commissionRates_obj, "taker", taker);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionRates, commissionRates_obj, "buyer", buyer);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionRates, commissionRates_obj, "seller", seller);
  }
  
  {
    RETRIVE_ARRAY(result_obj, "balances", balances_arr);
    ALLOCATE_VECT(deserialized.balances, balances_arr.size());
    for (auto& balanceEntry : balances_arr) {
      balance_t tmp;
      ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, balanceEntry, "asset", asset);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, balanceEntry, "free", free);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, balanceEntry, "locked", locked);
      deserialized.balances.push_back(std::move(tmp));
    }
  }

  {
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING(deserialized, result_obj, "accountType", accountType, account_and_symbols_permissions_e);
  }

  {
    RETRIVE_ARRAY(result_obj, "permissions", permissions_arr);
    ALLOCATE_VECT(deserialized.permissions, permissions_arr.size());
    for (auto& permissionEntry : permissions_arr) {
      deserialized.permissions.push_back(
        string_to_enum<account_and_symbols_permissions_e>(
          permissionEntry.stringValue
        )
      );
    }
  }
}

void deserialize(account_order_history_ret_t& deserialized, const std::string& json) {
  /*
    {
      "id": "734235c2-13d2-4574-be68-723e818c08f3",
      "status": 200,
      "result": [
        {
          "symbol": "BTCUSDT",
          "orderId": 12569099453,
          "orderListId": -1,
          "clientOrderId": "4d96324ff9d44481926157",
          "price": "23416.10000000",
          "origQty": "0.00847000",
          "executedQty": "0.00847000",
          "cummulativeQuoteQty": "198.33521500",
          "status": "FILLED",
          "timeInForce": "GTC",
          "type": "LIMIT",
          "side": "SELL",
          "stopPrice": "0.00000000",
          "icebergQty": "0.00000000",
          "time": 1660801715639,
          "updateTime": 1660801717945,
          "isWorking": true,
          "workingTime": 1660801715639,
          "origQuoteOrderQty": "0.00000000",
          "selfTradePreventionMode": "NONE",
          "preventedMatchId": 0,            // This field only appears if the order expired due to STP.
          "preventedQuantity": "1.200000"   // This field only appears if the order expired due to STP.
        }
      ],
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 20
        }
      ]
    }
  */
  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  RETRIVE_ARRAY(root_obj, "result", result_arr);
  ALLOCATE_VECT(deserialized.orders, result_arr.size());
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);

  /* result fields */
  for (auto& resultEntry : result_arr) {
    orderStatus_ret_t tmp;
    ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "symbol", symbol);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "orderId", orderId, long);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "orderListId", orderListId, int);
    ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "clientOrderId", clientOrderId);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "price", price);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "origQty", origQty);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "executedQty", executedQty);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "cummulativeQuoteQty", cummulativeQuoteQty);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "status", status, orderStatus_e);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "timeInForce", timeInForce, time_in_force_e);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "type", type, order_type_e);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "side", side, order_side_e);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "stopPrice", stopPrice);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "icebergQty", icebergQty);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "time", time, long);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "updateTime", updateTime, long);
    ASSIGN_BOOL_FIELD_FROM_JSON_BOOL_IN_OBJECT(tmp, resultEntry, "isWorking", isWorking);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "workingTime", workingTime, long);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "origQuoteOrderQty", origQuoteOrderQty);
    ASSIGN_ENUM_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "selfTradePreventionMode", selfTradePreventionMode, stp_modes_e);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "preventedMatchId", preventedMatchId, int);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "preventedQuantity", preventedQuantity);
    /* these fields are assign to an arbitrary default */
    {
      tmp.trailingDelta = -1;
      tmp.trailingTime = -1;
      tmp.strategyId = 0;
      tmp.strategyType = 0;
    }
    deserialized.orders.push_back(std::move(tmp));
  }
}

void deserialize(account_trade_list_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "f4ce6a53-a29d-4f70-823b-4ab59391d6e8",
      "status": 200,
      "result": [
        {
          "symbol": "BTCUSDT",
          "id": 1650422481,
          "orderId": 12569099453,
          "orderListId": -1,
          "price": "23416.10000000",
          "qty": "0.00635000",
          "quoteQty": "148.69223500",
          "commission": "0.00000000",
          "commissionAsset": "BNB",
          "time": 1660801715793,
          "isBuyer": false,
          "isMaker": true,
          "isBestMatch": true
        },
        {
          "symbol": "BTCUSDT",
          "id": 1650422482,
          "orderId": 12569099453,
          "orderListId": -1,
          "price": "23416.50000000",
          "qty": "0.00212000",
          "quoteQty": "49.64298000",
          "commission": "0.00000000",
          "commissionAsset": "BNB",
          "time": 1660801715793,
          "isBuyer": false,
          "isMaker": true,
          "isBestMatch": true
        }
      ],
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 20
        }
      ]
    }
  */
  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  RETRIVE_ARRAY(root_obj, "result", result_arr);
  ALLOCATE_VECT(deserialized.trades, result_arr.size());
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);

  /* result fields */
  for (auto& resultEntry : result_arr) {
    historicTrade_t tmp;
    ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "symbol", symbol);
    ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "commissionAsset", commissionAsset);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "id", id, int);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "orderId", orderId, int);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "orderListId", orderListId, int);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "time", time, long);
    ASSIGN_BOOL_FIELD_FROM_JSON_BOOL_IN_OBJECT(tmp, resultEntry, "isBuyer", isBuyer);
    ASSIGN_BOOL_FIELD_FROM_JSON_BOOL_IN_OBJECT(tmp, resultEntry, "isMaker", isMaker);
    ASSIGN_BOOL_FIELD_FROM_JSON_BOOL_IN_OBJECT(tmp, resultEntry, "isBestMatch", isBestMatch);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "price", price);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "qty", qty);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "quoteQty", quoteQty);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "commission", commission);
    deserialized.trades.push_back(std::move(tmp));
  }
}

void deserialize(account_commission_rates_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "d3df8a61-98ea-4fe0-8f4e-0fcea5d418b0",
      "status": 200,
      "result":
      {
        "symbol": "BTCUSDT",
        "standardCommission":              //Standard commission rates on trades from the order.
        {
          "maker": "0.00000010",
          "taker": "0.00000020",
          "buyer": "0.00000030",
          "seller": "0.00000040"
        },
        "taxCommission":                   //Tax commission rates on trades from the order.
        {
          "maker": "0.00000112",
          "taker": "0.00000114",
          "buyer": "0.00000118",
          "seller": "0.00000116"
        },
        "discount":                        //Discount on standard commissions when paying in BNB.
        {
          "enabledForAccount": true,
          "enabledForSymbol": true,
          "discountAsset": "BNB",
          "discount": "0.75000000"         //Standard commission is reduced by this rate when paying commission in BNB.
        }
      },
      "rateLimits":
      [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 20
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
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.commissionDesc, result_obj, "symbol", symbol);
  {
    RETRIVE_OBJECT(result_obj, "standardCommission", standardCommission_obj);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionDesc.standardCommission, standardCommission_obj, "maker", maker);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionDesc.standardCommission, standardCommission_obj, "taker", taker);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionDesc.standardCommission, standardCommission_obj, "buyer", buyer);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionDesc.standardCommission, standardCommission_obj, "seller", seller);
  }

  {
    RETRIVE_OBJECT(result_obj, "taxCommission", taxCommission_obj);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionDesc.taxCommission, taxCommission_obj, "maker", maker);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionDesc.taxCommission, taxCommission_obj, "taker", taker);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionDesc.taxCommission, taxCommission_obj, "buyer", buyer);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionDesc.taxCommission, taxCommission_obj, "seller", seller);
  }

  {
    RETRIVE_OBJECT(result_obj, "discount", discount_obj);
    ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.commissionDesc.discount, discount_obj, "discountAsset", discountAsset);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized.commissionDesc.discount, discount_obj, "discount", discount);
    ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(deserialized.commissionDesc.discount, discount_obj, "enabledForAccount", enabledForAccount);
    ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(deserialized.commissionDesc.discount, discount_obj, "enabledForSymbol", enabledForSymbol);
  }
}

} /* namespace exchange */
} /* namespace cuwacunu */
} /* namespace camahjucunu */
