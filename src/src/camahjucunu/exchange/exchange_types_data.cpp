#include "camahjucunu/exchange/exchange_types_data.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
std::string                depth_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(limit)); }
std::string         tradesRecent_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(limit)); }
std::string     tradesHistorical_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(limit), pairWrap(fromId)); }
std::string               klines_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol), pairWrap(interval), pairWrap(startTime), pairWrap(endTime), pairWrap(timeZone), pairWrap(limit)); }
std::string             avgPrice_args_t::jsonify() { return jsonify_as_object( pairWrap(symbol)); }
std::string               ticker_args_t::jsonify() { return jsonify_as_object( pairWrap_variant(symbol), pairWrap(windowSize), pairWrap(type)); }
std::string     tickerTradingDay_args_t::jsonify() { return jsonify_as_object( pairWrap_variant(symbol), pairWrap(timeZone), pairWrap(type)); }
std::string          tickerPrice_args_t::jsonify() { return jsonify_as_object( pairWrap_variant(symbol)); }
std::string           tickerBook_args_t::jsonify() { return jsonify_as_object( pairWrap_variant(symbol)); }

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/*              deserializations               */
/* --- --- --- --- --- --- --- --- --- --- --- */
depth_ret_t::depth_ret_t                       (const std::string &json) { deserialize(*this, json); };
tradesRecent_ret_t::tradesRecent_ret_t         (const std::string &json) { deserialize(*this, json); };
tradesHistorical_ret_t::tradesHistorical_ret_t (const std::string &json) { deserialize(*this, json); };
klines_ret_t::klines_ret_t                     (const std::string &json) { deserialize(*this, json); };
avgPrice_ret_t::avgPrice_ret_t                 (const std::string &json) { deserialize(*this, json); };
tickerTradingDay_ret_t::tickerTradingDay_ret_t (const std::string &json) { deserialize(*this, json); };
ticker_ret_t::ticker_ret_t                     (const std::string &json) { deserialize(*this, json); };
tickerPrice_ret_t::tickerPrice_ret_t           (const std::string &json) { deserialize(*this, json); };
tickerBook_ret_t::tickerBook_ret_t             (const std::string &json) { deserialize(*this, json); };

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/*                 from_csv                    */
/* --- --- --- --- --- --- --- --- --- --- --- */

trade_t trade_t::from_csv(const std::string& line, char delimiter, size_t line_number) {
  const size_t expected_fields = 7;
  std::vector<std::string> tokens;
  std::stringstream ss(line);
  std::string token;
  while (std::getline(ss, token, delimiter)) {
    tokens.push_back(token);
  }

  if (tokens.size() != expected_fields) {
    throw std::runtime_error("[from_csv](trade_t) Incorrect number of fields in line " + std::to_string(line_number)
      + ": expected " + std::to_string(expected_fields) + ", got "
      + std::to_string(tokens.size()) + ". Line content: " + line);
  }

  trade_t trade;
  size_t idx = 0;

  try {
    trade.id = std::stol(tokens[idx++]);
    trade.price = std::stod(tokens[idx++]);
    trade.qty = std::stod(tokens[idx++]);
    trade.quoteQty = std::stod(tokens[idx++]);
    trade.time = std::stol(tokens[idx++]);
    
    // isBuyerMaker
    {
      std::string& bool_str = tokens[idx++];
      if (bool_str == "true" || bool_str == "1") {
        trade.isBuyerMaker = true;
      } else if (bool_str == "false" || bool_str == "0") {
        trade.isBuyerMaker = false;
      } else {
        throw std::runtime_error("[from_csv](trade_t) Invalid boolean value for isBuyerMaker: " + bool_str);
      }
    }

    // isBestMatch
    {
      std::string& bool_str = tokens[idx++];
      if (bool_str == "true" || bool_str == "1") {
        trade.isBestMatch = true;
      } else if (bool_str == "false" || bool_str == "0") {
        trade.isBestMatch = false;
      } else {
        throw std::runtime_error("[from_csv](trade_t) Invalid boolean value for isBestMatch: " + bool_str);
      }
    }
  } catch (const std::exception& e) {
    throw std::runtime_error("[from_csv](trade_t) Error parsing tokens in line " + std::to_string(line_number) + ": " + e.what());
  }

  return trade;
}


kline_t kline_t::from_csv(const std::string& line, char delimiter, size_t line_number) {
  const size_t expected_fields = 11;
  std::vector<std::string> tokens;
  std::stringstream ss(line);
  std::string token;
  while (std::getline(ss, token, delimiter)) {
    tokens.push_back(token);
  }

  if (tokens.size() != expected_fields) {
    throw std::runtime_error("[from_csv](kline_t) Incorrect number of fields in line " + std::to_string(line_number)
      + ": expected " + std::to_string(expected_fields) + ", got "
      + std::to_string(tokens.size()) + ". Line content: " + line);
  }

  kline_t kline;
  size_t idx = 0;

  try {
    kline.open_time = std::stol(tokens[idx++]);
    kline.open_price = std::stod(tokens[idx++]);
    kline.high_price = std::stod(tokens[idx++]);
    kline.low_price = std::stod(tokens[idx++]);
    kline.close_price = std::stod(tokens[idx++]);
    kline.volume = std::stod(tokens[idx++]);
    kline.close_time = std::stol(tokens[idx++]);
    kline.quote_asset_volume = std::stod(tokens[idx++]);
    kline.number_of_trades = std::stoi(tokens[idx++]);
    kline.taker_buy_base_volume = std::stod(tokens[idx++]);
    kline.taker_buy_quote_volume = std::stod(tokens[idx++]);
  } catch (const std::exception& e) {
    throw std::runtime_error("[from_csv](kline_t) Error parsing tokens in line " + std::to_string(line_number) + ": " + e.what());
  }

  return kline;
}

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/*               from_binary                   */
/* --- --- --- --- --- --- --- --- --- --- --- */
kline_t kline_t::from_binary(const char* data) {
  kline_t obj;
  std::memcpy(&obj, data, sizeof(kline_t));
  return obj;
}

trade_t trade_t::from_binary(const char* data) {
  trade_t obj;
  std::memcpy(&obj, data, sizeof(trade_t));
  return obj;
}

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/*             tensor_features                 */
/* --- --- --- --- --- --- --- --- --- --- --- */
std::vector<double> kline_t::tensor_features() {
  return {
    static_cast<double>(open_time),
    open_price,
    high_price,
    low_price,
    close_price,
    volume,
    static_cast<double>(close_time),
    quote_asset_volume,
    static_cast<double>(number_of_trades),
    taker_buy_base_volume,
    taker_buy_quote_volume
  };
}

std::vector<double> trade_t::tensor_features() {
  return {
    static_cast<double>(trade.id),
    trade.price,
    trade.qty,
    trade.quoteQty,
    static_cast<double>(trade.time),
    static_cast<double>(trade.isBuyerMaker),  // Convert bool to double (1.0 for true, 0.0 for false)
    static_cast<double>(trade.isBestMatch)    // Convert bool to double
  };
}

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         deserialize specializations         */
/* --- --- --- --- --- --- --- --- --- --- --- */

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
    log_secure_fatal("Unexpected [exchange_types_deserialization](ticker_ret_t) encoutner: %s \n", json.c_str());
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
    log_secure_fatal("Unexpected [exchange_types_deserialization](ticker_ret_t) encoutner: %s \n", json.c_str());
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
    log_secure_fatal("Unexpected [exchange_types_deserialization](tickerPrice_ret_t) encoutner: %s \n", json.c_str());
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
    log_secure_fatal("Unexpected [exchange_types_deserialization](tickerBook_ret_t) encoutner: %s \n", json.c_str());
  }
}

} /* namespace exchange */
} /* namespace cuwacunu */
} /* namespace camahjucunu */
