#include "camahjucunu/exchange/exchange_deserialization.h"

/* Utility Macros */

/* ------------------- General ------------------- */
#define INITIAL_PARSE(json_input, root_var, obj_var) \
    cuwacunu::piaabo::JsonValue root_var = \
        cuwacunu::piaabo::JsonParser(json_input).parse(); \
    cuwacunu::piaabo::ObjectType& obj_var = *(root_var.objectValue)

#define RETRIVE_OBJECT(obj_var, key, result_obj) \
  cuwacunu::piaabo::ObjectType& result_obj = *(obj_var[key].objectValue)

#define RETRIVE_ARRAY(obj_var, key, result_arr) \
  cuwacunu::piaabo::ArrayType& result_arr = *(obj_var[key].arrayValue)

#define RETRIVE_OBJECT_FROM_OBJECT(obj_var, key, result_obj) \
  RETRIVE_OBJECT((*obj_var.objectValue), key, result_obj)

/* ------------------- Value - Keys ------------------- */

#define ASSIGN_STRING_FIELD_FROM_JSON_STRING(obj, root_obj, json_key, member) \
  obj.member = root_obj[json_key].stringValue;

#define ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(obj, root_obj, json_key, member) \
  obj.member = std::stod(root_obj[json_key].stringValue.c_str());

#define ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(obj, root_obj, json_key, member, cast_type) \
  obj.member = static_cast<cast_type>(root_obj[json_key].numberValue);

#define ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(obj, root_obj, json_key, member) \
  obj.member = root_obj[json_key].boolValue;

#define ASSIGN_ENUM_FIELD_FROM_JSON_STRING(obj, root_obj, json_key, member, T) \
  obj.member = string_to_enum<T>(root_obj[json_key].stringValue);

/* ------------------- Values From Object ------------------- */

#define ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(obj, root_obj, json_key, member, cast_type) \
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(obj, (*root_obj.objectValue), json_key, member, cast_type)

#define ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(obj, root_obj, json_key, member) \
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(obj, (*root_obj.objectValue), json_key, member)

#define ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(obj, root_obj, json_key, member) \
  ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(obj, (*root_obj.objectValue), json_key, member)

#define ASSIGN_BOOL_FIELD_FROM_JSON_BOOL_IN_OBJECT(obj, root_obj, json_key, member) \
  ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(obj, (*root_obj.objectValue), json_key, member)

#define ASSIGN_ENUM_FIELD_FROM_JSON_STRING_IN_OBJECT(obj, root_obj, json_key, member, T) \
  ASSIGN_ENUM_FIELD_FROM_JSON_STRING(obj, (*root_obj.objectValue), json_key, member, T)

/* ------------------- Values From Array ------------------- */

#define ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_ARRAY(obj, root_arr, index, member, cast_type) \
  obj.member = static_cast<cast_type>((*root_arr.arrayValue)[index].numberValue);

#define ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(obj, root_arr, index, member) \
  obj.member = std::stod((*root_arr.arrayValue)[index].stringValue.c_str());

#define ALLOCATE_VECT(obj, size) \
  obj.reserve(size); \
  obj.clear();


namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* primary return structs*/

void deserialize(ping_ret_t& deserialized, const std::string& json) {
  /*
    {
      "id": "922bcc6e-9de8-440d-9e84-7c80933a8d0d",
      "status": 200,
      "result": {},
      "rateLimits": [
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
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);
  
  /* no other fields for ping_ret_t */
}

void deserialize(time_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "187d3cb2-942d-484c-8271-4e2141bbadb1",
      "status": 200,
      "result": {
        "serverTime": 1656400526260
      },
      "rateLimits": [
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
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "serverTime", serverTime, long);

}


void deserialize(depth_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "51e2affb-0aba-4821-ba75-f2625006eb43",
      "status": 200,
      "result": {
        "lastUpdateId": 2731179239,
        // Bid levels are sorted from highest to lowest price.
        "bids": [
          [
            "0.01379900",   // Price
            "3.43200000"    // Quantity
          ],
          [
            "0.01379800",
            "3.24300000"
          ],
          [
            "0.01379700",
            "10.45500000"
          ],
          [
            "0.01379600",
            "3.82100000"
          ],
          [
            "0.01379500",
            "10.26200000"
          ]
        ],
        // Ask levels are sorted from lowest to highest price.
        "asks": [
          [
            "0.01380000",
            "5.91700000"
          ],
          [
            "0.01380100",
            "6.01400000"
          ],
          [
            "0.01380200",
            "0.26800000"
          ],
          [
            "0.01380300",
            "0.33800000"
          ],
          [
            "0.01380400",
            "0.26800000"
          ]
        ]
      },
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 2
        }
      ]
    }
  */
  
  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  RETRIVE_OBJECT(root_obj, "result", result_obj);
  RETRIVE_ARRAY(result_obj, "bids", bids);
  RETRIVE_ARRAY(result_obj, "asks", asks);
  ALLOCATE_VECT(deserialized.bids, bids.size());
  ALLOCATE_VECT(deserialized.asks, asks.size());
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);
  
  /* result fields */
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "lastUpdateId", lastUpdateId, long);
  for (auto& bidEntry : bids) {
    price_qty_t tmp;
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, bidEntry, 0, price);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, bidEntry, 1, qty);
    deserialized.bids.push_back(std::move(tmp));
  }
  for (auto& askEntry : asks) {
    price_qty_t tmp;
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, askEntry, 0, price);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, askEntry, 1, qty);
    deserialized.asks.push_back(std::move(tmp));
  }
}

void deserialize(tradesRecent_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "409a20bd-253d-41db-a6dd-687862a5882f",
      "status": 200,
      "result": [
        {
          "id": 194686783,
          "price": "0.01361000",
          "qty": "0.01400000",
          "quoteQty": "0.00019054",
          "time": 1660009530807,
          "isBuyerMaker": true,
          "isBestMatch": true
        }
      ],
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 2
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
    trade_t tmp;
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "price", price);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "qty", qty);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "quoteQty", quoteQty);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "id", id, long);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "time", time, long);
    ASSIGN_BOOL_FIELD_FROM_JSON_BOOL_IN_OBJECT(tmp, resultEntry, "isBuyerMaker", isBuyerMaker);
    ASSIGN_BOOL_FIELD_FROM_JSON_BOOL_IN_OBJECT(tmp, resultEntry, "isBestMatch", isBestMatch);
    deserialized.trades.push_back(std::move(tmp));
  }
}

void deserialize(tradesHistorical_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "cffc9c7d-4efc-4ce0-b587-6b87448f052a",
      "status": 200,
      "result": [
        {
          "id": 0,
          "price": "0.00005000",
          "qty": "40.00000000",
          "quoteQty": "0.00200000",
          "time": 1500004800376,
          "isBuyerMaker": true,
          "isBestMatch": true
        }
      ],
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 10
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
    trade_t tmp;
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "price", price);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "qty", qty);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "quoteQty", quoteQty);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "id", id, long);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "time", time, long);
    ASSIGN_BOOL_FIELD_FROM_JSON_BOOL_IN_OBJECT(tmp, resultEntry, "isBuyerMaker", isBuyerMaker);
    ASSIGN_BOOL_FIELD_FROM_JSON_BOOL_IN_OBJECT(tmp, resultEntry, "isBestMatch", isBestMatch);
    deserialized.trades.push_back(std::move(tmp));
  }
}

void deserialize(klines_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "1dbbeb56-8eea-466a-8f6e-86bdcfa2fc0b",
      "status": 200,
      "result": [
        [
          1655971200000,      // Kline open time
          "0.01086000",       // Open price
          "0.01086600",       // High price
          "0.01083600",       // Low price
          "0.01083800",       // Close price
          "2290.53800000",    // Volume
          1655974799999,      // Kline close time
          "24.85074442",      // Quote asset volume
          2283,               // Number of trades
          "1171.64000000",    // Taker buy base asset volume
          "12.71225884",      // Taker buy quote asset volume
          "0"                 // Unused field, ignore
        ]
      ],
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 2
        }
      ]
    }
  */
  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  RETRIVE_ARRAY(root_obj, "result", result_arr);
  ALLOCATE_VECT(deserialized.klines, result_arr.size());
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);
  
  /* result fields */
  for (auto& resultEntry : result_arr) {
    kline_t tmp;
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_ARRAY(tmp, resultEntry, 0, open_time, long);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, resultEntry, 1, open_price);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, resultEntry, 2, high_price);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, resultEntry, 3, low_price);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, resultEntry, 4, close_price);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, resultEntry, 5, volume);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_ARRAY(tmp, resultEntry, 6, close_time, long);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, resultEntry, 7, quote_asset_volume);
    ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_ARRAY(tmp, resultEntry, 8, number_of_trades, int);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, resultEntry, 9, taker_buy_base_volume);
    ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(tmp, resultEntry, 10, taker_buy_quote_volume);
    deserialized.klines.push_back(std::move(tmp));
  }
}

void deserialize(avgPrice_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "ddbfb65f-9ebf-42ec-8240-8f0f91de0867",
      "status": 200,
      "result": {
        "mins": 5,                    // Average price interval (in minutes)
        "price": "9.35751834",        // Average price
        "closeTime": 1694061154503    // Last trade time
      },
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 2
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
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "mins", mins, int);
  ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(deserialized, result_obj, "price", price);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "closeTime", close_time, long);
}

void deserialize(tickerTradingDay_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "f4b3b507-c8f2-442a-81a6-b2f12daa030f",
      "status": 200,
      "result": {
        "symbol": "BTCUSDT",
        "priceChange": "-83.13000000",                // Absolute price change
        "priceChangePercent": "-0.317",               // Relative price change in percent
        "weightedAvgPrice": "26234.58803036",         // quoteVolume / volume
        "openPrice": "26304.80000000",
        "highPrice": "26397.46000000",
        "lowPrice": "26088.34000000",
        "lastPrice": "26221.67000000",
        "volume": "18495.35066000",                   // Volume in base asset
        "quoteVolume": "485217905.04210480",
        "openTime": 1695686400000,
        "closeTime": 1695772799999,
        "firstId": 3220151555,
        "lastId": 3220849281,
        "count": 697727
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
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);
  
  /* determine if ticker is full */

  if(root_obj["result"].type == cuwacunu::piaabo::JsonValueType::OBJECT) {
    /* found the result to be an object */
    RETRIVE_OBJECT(root_obj, "result", result_obj);
    deserialized.is_full = result_obj.find("weightedAvgPrice") != result_obj.end();

    /* result fields */
    if(deserialized.is_full) {
      tick_full_t tmp;
      ASSIGN_STRING_FIELD_FROM_JSON_STRING(tmp, result_obj, "symbol", symbol);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "priceChange", priceChange);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "priceChangePercent", priceChangePercent);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "weightedAvgPrice", weightedAvgPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "openPrice", openPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "highPrice", highPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "lowPrice", lowPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "lastPrice", lastPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "volume", volume);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "quoteVolume", quoteVolume);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "openTime", openTime, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "closeTime", closeTime, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "firstId", firstId, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "lastId", lastId, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "count", count, int);
      deserialized.ticks.emplace<tick_full_t>(std::move(tmp));
    } else {
      tick_mini_t tmp;
      ASSIGN_STRING_FIELD_FROM_JSON_STRING(tmp, result_obj, "symbol", symbol);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "lastPrice", lastPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "openPrice", openPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "highPrice", highPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "lowPrice", lowPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "volume", volume);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "quoteVolume", quoteVolume);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "openTime", openTime, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "closeTime", closeTime, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "firstId", firstId, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "lastId", lastId, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "count", count, int);
      deserialized.ticks.emplace<tick_mini_t>(std::move(tmp));
    }
  } else if(root_obj["result"].type == cuwacunu::piaabo::JsonValueType::ARRAY) {
    /* found result to be a list */
    RETRIVE_ARRAY(root_obj, "result", result_arr);
    if(result_arr.size() > 0) {
      /* determine the type of tick */
      deserialized.is_full = result_arr[0].objectValue->find("weightedAvgPrice") != result_arr[0].objectValue->end();
      /* determine the type of tick */
      if(deserialized.is_full) {
        deserialized.ticks.emplace<std::vector<tick_full_t>>();
        std::get<std::vector<tick_full_t>>(deserialized.ticks).reserve(result_arr.size());
      } else {
        deserialized.ticks.emplace<std::vector<tick_mini_t>>();
        std::get<std::vector<tick_mini_t>>(deserialized.ticks).reserve(result_arr.size());
      }
      /* loop over all records */
      for (auto& resultEntry : result_arr) {
        /* result fields */
        if(deserialized.is_full) {
          tick_full_t tmp;
          ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "symbol", symbol);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "priceChange", priceChange);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "priceChangePercent", priceChangePercent);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "weightedAvgPrice", weightedAvgPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "openPrice", openPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "highPrice", highPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "lowPrice", lowPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "lastPrice", lastPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "volume", volume);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "quoteVolume", quoteVolume);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "openTime", openTime, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "closeTime", closeTime, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "firstId", firstId, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "lastId", lastId, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "count", count, int);
          std::get<std::vector<tick_full_t>>(deserialized.ticks).push_back(std::move(tmp));
        } else {
          tick_mini_t tmp;
          ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "symbol", symbol);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "lastPrice", lastPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "openPrice", openPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "highPrice", highPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "lowPrice", lowPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "volume", volume);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "quoteVolume", quoteVolume);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "openTime", openTime, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "closeTime", closeTime, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "firstId", firstId, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "lastId", lastId, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "count", count, int);
          std::get<std::vector<tick_mini_t>>(deserialized.ticks).push_back(std::move(tmp));
        }
      }
    }
  } else {
    /* error case */
    log_secure_fatal("Unexpected [exchange_deserialization](ticker_ret_t) encoutner: %s \n", json.c_str());
  }
}

void deserialize(ticker_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "f4b3b507-c8f2-442a-81a6-b2f12daa030f",
      "status": 200,
      "result": {
        "symbol": "BNBBTC",
        "priceChange": "0.00061500",
        "priceChangePercent": "4.735",
        "weightedAvgPrice": "0.01368242",
        "openPrice": "0.01298900",
        "highPrice": "0.01418800",
        "lowPrice": "0.01296000",
        "lastPrice": "0.01360400",
        "volume": "587179.23900000",
        "quoteVolume": "8034.03382165",
        "openTime": 1659580020000,
        "closeTime": 1660184865291,
        "firstId": 192977765,       // First trade ID
        "lastId": 195365758,        // Last trade ID
        "count": 2387994            // Number of trades
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
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);
  
  /* determine if ticker is full */

  if(root_obj["result"].type == cuwacunu::piaabo::JsonValueType::OBJECT) {
    /* found the result to be an object */
    RETRIVE_OBJECT(root_obj, "result", result_obj);
    deserialized.is_full = result_obj.find("weightedAvgPrice") != result_obj.end();

    /* result fields */
    if(deserialized.is_full) {
      tick_full_t tmp;
      ASSIGN_STRING_FIELD_FROM_JSON_STRING(tmp, result_obj, "symbol", symbol);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "priceChange", priceChange);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "priceChangePercent", priceChangePercent);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "weightedAvgPrice", weightedAvgPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "openPrice", openPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "highPrice", highPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "lowPrice", lowPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "lastPrice", lastPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "volume", volume);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "quoteVolume", quoteVolume);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "openTime", openTime, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "closeTime", closeTime, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "firstId", firstId, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "lastId", lastId, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "count", count, int);
      deserialized.ticks.emplace<tick_full_t>(std::move(tmp));
    } else {
      tick_mini_t tmp;
      ASSIGN_STRING_FIELD_FROM_JSON_STRING(tmp, result_obj, "symbol", symbol);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "lastPrice", lastPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "openPrice", openPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "highPrice", highPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "lowPrice", lowPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "volume", volume);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "quoteVolume", quoteVolume);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "openTime", openTime, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "closeTime", closeTime, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "firstId", firstId, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "lastId", lastId, long);
      ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(tmp, result_obj, "count", count, int);
      deserialized.ticks.emplace<tick_mini_t>(std::move(tmp));
    }
  } else if(root_obj["result"].type == cuwacunu::piaabo::JsonValueType::ARRAY) {
    /* found result to be a list */
    RETRIVE_ARRAY(root_obj, "result", result_arr);
    if(result_arr.size() > 0) {
      /* determine the type of tick */
      deserialized.is_full = result_arr[0].objectValue->find("weightedAvgPrice") != result_arr[0].objectValue->end();
      /* determine the type of tick */
      if(deserialized.is_full) {
        deserialized.ticks.emplace<std::vector<tick_full_t>>();
        std::get<std::vector<tick_full_t>>(deserialized.ticks).reserve(result_arr.size());
      } else {
        deserialized.ticks.emplace<std::vector<tick_mini_t>>();
        std::get<std::vector<tick_mini_t>>(deserialized.ticks).reserve(result_arr.size());
      }
      /* loop over all records */
      for (auto& resultEntry : result_arr) {
        /* result fields */
        if(deserialized.is_full) {
          tick_full_t tmp;
          ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "symbol", symbol);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "priceChange", priceChange);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "priceChangePercent", priceChangePercent);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "weightedAvgPrice", weightedAvgPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "openPrice", openPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "highPrice", highPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "lowPrice", lowPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "lastPrice", lastPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "volume", volume);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "quoteVolume", quoteVolume);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "openTime", openTime, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "closeTime", closeTime, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "firstId", firstId, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "lastId", lastId, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "count", count, int);
          std::get<std::vector<tick_full_t>>(deserialized.ticks).push_back(std::move(tmp));
        } else {
          tick_mini_t tmp;
          ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "symbol", symbol);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "lastPrice", lastPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "openPrice", openPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "highPrice", highPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "lowPrice", lowPrice);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "volume", volume);
          ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "quoteVolume", quoteVolume);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "openTime", openTime, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "closeTime", closeTime, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "firstId", firstId, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "lastId", lastId, long);
          ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(tmp, resultEntry, "count", count, int);
          std::get<std::vector<tick_mini_t>>(deserialized.ticks).push_back(std::move(tmp));
        }
      }
    }
  } else {
    /* error case */
    log_secure_fatal("Unexpected [exchange_deserialization](ticker_ret_t) encoutner: %s \n", json.c_str());
  }
}

void deserialize(tickerPrice_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "043a7cf2-bde3-4888-9604-c8ac41fcba4d",
      "status": 200,
      "result": {
        "symbol": "BNBBTC",
        "price": "0.01361900"
      },
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 2
        }
      ]
    }
  */
  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);

  /* result fields */
  if(root_obj["result"].type == cuwacunu::piaabo::JsonValueType::OBJECT) {
    RETRIVE_OBJECT(root_obj, "result", result_obj);
    /* only one symbol */
    {
      price_t tmp;
      ASSIGN_STRING_FIELD_FROM_JSON_STRING(tmp, result_obj, "symbol", symbol);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "price", price);
      deserialized.prices.emplace<price_t>(std::move(tmp));
    }
  } else if(root_obj["result"].type == cuwacunu::piaabo::JsonValueType::ARRAY) {
    RETRIVE_ARRAY(root_obj, "result", result_arr);
    deserialized.prices.emplace<std::vector<price_t>>();
    std::get<std::vector<price_t>>(deserialized.prices).reserve(result_arr.size());
    /* list of symbols */
    for (auto& resultEntry : result_arr) {
      price_t tmp;
      ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "symbol", symbol);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "price", price);
      std::get<std::vector<price_t>>(deserialized.prices).push_back(std::move(tmp));
    }
  } else {
    /* error case */
    log_secure_fatal("Unexpected [exchange_deserialization](tickerPrice_ret_t) encoutner: %s \n", json.c_str());
  }
}

void deserialize(tickerBook_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "9d32157c-a556-4d27-9866-66760a174b57",
      "status": 200,
      "result": {
        "symbol": "BNBBTC",
        "bidPrice": "0.01358000",
        "bidQty": "12.53400000",
        "askPrice": "0.01358100",
        "askQty": "17.83700000"
      },
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 2
        }
      ]
    }
  */
  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);

  /* result fields */
  if(root_obj["result"].type == cuwacunu::piaabo::JsonValueType::OBJECT) {
    RETRIVE_OBJECT(root_obj, "result", result_obj);
    /* only one symbol */
    {
      bookPrice_t tmp;
      ASSIGN_STRING_FIELD_FROM_JSON_STRING(tmp, result_obj, "symbol", symbol);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "bidPrice", bidPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "bidQty", bidQty);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "askPrice", askPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp, result_obj, "askQty", askQty);
      deserialized.bookPrices.emplace<bookPrice_t>(std::move(tmp));
    }
  } else if(root_obj["result"].type == cuwacunu::piaabo::JsonValueType::ARRAY) {
    RETRIVE_ARRAY(root_obj, "result", result_arr);
    deserialized.bookPrices.emplace<std::vector<bookPrice_t>>();
    std::get<std::vector<bookPrice_t>>(deserialized.bookPrices).reserve(result_arr.size());
    /* list of symbols */
    for (auto& resultEntry : result_arr) {
      bookPrice_t tmp;
      ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "symbol", symbol);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "bidPrice", bidPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "bidQty", bidQty);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "askPrice", askPrice);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "askQty", askQty);
      std::get<std::vector<bookPrice_t>>(deserialized.bookPrices).push_back(std::move(tmp));
    }
  } else {
    /* error case */
    log_secure_fatal("Unexpected [exchange_deserialization](tickerBook_ret_t) encoutner: %s \n", json.c_str());
  }
}

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

void deserialize(query_commission_rates_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "d3df8a61-98ea-4fe0-8f4e-0fcea5d418b0",
      "status": 200,
      "result":
      [
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
        }
      ],
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
  RETRIVE_ARRAY(root_obj, "result", result_arr);
  ALLOCATE_VECT(deserialized.commissionDesc, result_arr.size());
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);

  /* result fields */
  for (auto& resultEntry : result_arr) {
    commissionDesc_t tmp;
    ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(tmp, resultEntry, "symbol", symbol);
    {
      RETRIVE_OBJECT_FROM_OBJECT(resultEntry, "standardCommission", standardCommission_obj);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp.standardCommission, standardCommission_obj, "maker", maker);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp.standardCommission, standardCommission_obj, "taker", taker);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp.standardCommission, standardCommission_obj, "buyer", buyer);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp.standardCommission, standardCommission_obj, "seller", seller);
    }

    {
      RETRIVE_OBJECT_FROM_OBJECT(resultEntry, "taxCommission", taxCommission_obj);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp.taxCommission, taxCommission_obj, "maker", maker);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp.taxCommission, taxCommission_obj, "taker", taker);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp.taxCommission, taxCommission_obj, "buyer", buyer);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp.taxCommission, taxCommission_obj, "seller", seller);
    }

    {
      RETRIVE_OBJECT_FROM_OBJECT(resultEntry, "discount", discount_obj);
      ASSIGN_STRING_FIELD_FROM_JSON_STRING(tmp.discount, discount_obj, "discountAsset", discountAsset);
      ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(tmp.discount, discount_obj, "discount", discount);
      ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(tmp.discount, discount_obj, "enabledForAccount", enabledForAccount);
      ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(tmp.discount, discount_obj, "enabledForSymbol", enabledForSymbol);
    }
    deserialized.commissionDesc.push_back(std::move(tmp));
  }
}

/* order returns */
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
} /* namespace camahjucunu */
} /* namespace cuwacunu */