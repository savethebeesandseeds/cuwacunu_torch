#include "piaabo/dutils.h"
#include "piaabo/config_space.h"
#include "piaabo/security.h"
#include "piaabo/encryption.h"
#include "camahjucunu/crypto_exchange/binance_enums.h"
#include "camahjucunu/crypto_exchange/binance_types.h"

int main() {
  /* test serialization */
  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::depth_args_t] serialization \n");
    cuwacunu::camahjucunu::binance::depth_args_t  variable("value");
    variable.limit = 10;
    log_info("%s\n", variable.jsonify().c_str());
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::ticker_24hr_args_t] serialization \n");
    cuwacunu::camahjucunu::binance::ticker_24hr_args_t  variable("value1");
    variable.type = cuwacunu::camahjucunu::binance::ticker_type_e::FULL;
    log_info("%s\n", variable.jsonify().c_str());
  }
  
  {
    cuwacunu::camahjucunu::binance::ticker_24hr_args_t  variable(std::vector<std::string>{"vaelu1", "value2"});
    variable.type = cuwacunu::camahjucunu::binance::ticker_type_e::FULL;
    log_info("%s\n", variable.jsonify().c_str());
  }
  
  /* test deserialization */
  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::ping_ret_t] deserialization \n");
    std::string json = "{}";
    
    log_info("json: %s\n", json.c_str());

    cuwacunu::camahjucunu::binance::ping_ret_t parsed(json);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::time_ret_t] deserialization \n");
    std::string json = "{\"serverTime\": 1499827319559,12,12m,1231{},{\"serverTime\": 12}}";
    cuwacunu::camahjucunu::binance::time_ret_t parsed(json);

    log_info("json: %s\n", json.c_str());

    log_info("serverTime: %ld\n", parsed.serverTime);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::depth_ret_t] deserialization \n");
    std::string json = "{\"lastUpdateId\": 1027024,\"bids\": [[\"4.00000000\",\"431.00000000\"]],\"asks\": [[\"4.00000200\",\"12.00000000\"]]}";
    cuwacunu::camahjucunu::binance::depth_ret_t parsed(json);
    
    log_info("json: %s\n", json.c_str());

    log_info("lastUpdateId: %ld\n", parsed.lastUpdateId);
    log_info(".bids.length: %ld\n", parsed.bids.size());
    log_info(".asks.length: %ld\n", parsed.asks.size());
  }

  {
    std::string json = "{\"lastUpdateId\": 1027024,\"bids\": [[\"4.00000000\",\"441.00000000\"],[\"1.00000000\",\"2.00000000\"]],\"asks\": [[\"4.00000200\",\"12.00000000\"],[\"1.00000000\",\"2.00000000\"],[\"1.00000000\",\"2.00000000\"],[\"1.00000000\",\"2.00000000\"]]}";
    cuwacunu::camahjucunu::binance::depth_ret_t parsed(json);
    
    log_info("json: %s\n", json.c_str());

    log_info("lastUpdateId: %ld\n", parsed.lastUpdateId);
    log_info(".bids.length: %ld\n", parsed.bids.size());
    log_info(".asks.length: %ld\n", parsed.asks.size());

  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::trades_ret_t] deserialization \n");
    std::string json = "[{\"id\": 28457,\"price\": \"4.00000100\",\"qty\": \"12.00000000\",\"quoteQty\": \"48.000012\",\"time\": 1499865549590,\"isBuyerMaker\": true,\"isBestMatch\": true}]";

    cuwacunu::camahjucunu::binance::trades_ret_t parsed(json);
    
    log_info("json: %s\n", json.c_str());

    log_info(".trades.size(): %ld\n", parsed.trades.size());
    log_info(".trades[0].id: %ld\n", parsed.trades[0].id);
    log_info(".trades[0].price: %.10f\n", parsed.trades[0].price);
    log_info(".trades[0].qty: %.10f\n", parsed.trades[0].qty);
    log_info(".trades[0].quoteQty: %.10f\n", parsed.trades[0].quoteQty);
    log_info(".trades[0].time: %ld\n", parsed.trades[0].time);
    log_info(".trades[0].isBuyerMaker: %d\n", parsed.trades[0].isBuyerMaker);
    log_info(".trades[0].isBestMatch: %d\n", parsed.trades[0].isBestMatch);

  }
  
  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::historicalTrades_ret_t] deserialization \n");
    std::string json = "[{\"id\": 2812467,\"price\": \"411.00000100\",\"qty\": \"142.00000000\",\"quoteQty\": \"4518.000012\",\"time\": 1149986664990,\"isBuyerMaker\": false,\"isBestMatch\": true}, {\"id\": 28467,\"price\": \"4.00000100\",\"qty\": \"12.00000000\",\"quoteQty\": \"48.000012\",\"time\": 1499866649690,\"isBuyerMaker\": true,\"isBestMatch\": false}]";

    cuwacunu::camahjucunu::binance::historicalTrades_ret_t parsed(json);

    log_info("json: %s\n", json.c_str());
    
    log_info(".trades.size(): %ld\n", parsed.trades.size());
    log_info(".trades[0].id: %ld\n", parsed.trades[0].id);
    log_info(".trades[0].price: %.10f\n", parsed.trades[0].price);
    log_info(".trades[0].qty: %.10f\n", parsed.trades[0].qty);
    log_info(".trades[0].quoteQty: %.10f\n", parsed.trades[0].quoteQty);
    log_info(".trades[0].time: %ld\n", parsed.trades[0].time);
    log_info(".trades[0].isBuyerMaker: %d\n", parsed.trades[0].isBuyerMaker);
    log_info(".trades[0].isBestMatch: %d\n", parsed.trades[0].isBestMatch);
    
    log_info(".trades[1].id: %ld\n", parsed.trades[1].id);
    log_info(".trades[1].price: %.10f\n", parsed.trades[1].price);
    log_info(".trades[1].qty: %.10f\n", parsed.trades[1].qty);
    log_info(".trades[1].quoteQty: %.10f\n", parsed.trades[1].quoteQty);
    log_info(".trades[1].time: %ld\n", parsed.trades[1].time);
    log_info(".trades[1].isBuyerMaker: %d\n", parsed.trades[1].isBuyerMaker);
    log_info(".trades[1].isBestMatch: %d\n", parsed.trades[1].isBestMatch);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::klines_ret_t] deserialization \n");
    std::string json = "[[1499040000000,\"0.01634790\",\"0.80000000\",\"0.01575800\",\"0.01577100\",\"148976.11427815\",1499644799999,\"2434.19055334\",308,\"1756.87402397\",\"28.46694368\",\"0\"],[1499040000111,\"1.11634791\",\"1.81111111\",\"1.11575811\",\"1.11577111\",\"148976.11427815\",1499644799999,\"2434.19155334\",318,\"1756.87412397\",\"28.46694368\",\"1\"]]";
    
    cuwacunu::camahjucunu::binance::klines_ret_t parsed(json);

    log_info("json: %s\n", json.c_str());
    
    log_info(".klines.size(): %ld\n", parsed.klines.size());
    
    log_info(".klines[0].open_time: %ld\n", parsed.klines[0].open_time);
    log_info(".klines[0].open_price: %.10f\n", parsed.klines[0].open_price);
    log_info(".klines[0].high_price: %.10f\n", parsed.klines[0].high_price);
    log_info(".klines[0].low_price: %.10f\n", parsed.klines[0].low_price);
    log_info(".klines[0].close_price: %.10f\n", parsed.klines[0].close_price);
    log_info(".klines[0].volume: %.10f\n", parsed.klines[0].volume);
    log_info(".klines[0].close_time: %ld\n", parsed.klines[0].close_time);
    log_info(".klines[0].quote_asset_volume: %.10f\n", parsed.klines[0].quote_asset_volume);
    log_info(".klines[0].number_of_trades: %d\n", parsed.klines[0].number_of_trades);
    log_info(".klines[0].taker_buy_base_volume: %.10f\n", parsed.klines[0].taker_buy_base_volume);
    log_info(".klines[0].taker_buy_quote_volume: %.10f\n", parsed.klines[0].taker_buy_quote_volume);

    log_info(".klines[1].open_time: %ld\n", parsed.klines[1].open_time);
    log_info(".klines[1].open_price: %.10f\n", parsed.klines[1].open_price);
    log_info(".klines[1].high_price: %.10f\n", parsed.klines[1].high_price);
    log_info(".klines[1].low_price: %.10f\n", parsed.klines[1].low_price);
    log_info(".klines[1].close_price: %.10f\n", parsed.klines[1].close_price);
    log_info(".klines[1].volume: %.10f\n", parsed.klines[1].volume);
    log_info(".klines[1].close_time: %ld\n", parsed.klines[1].close_time);
    log_info(".klines[1].quote_asset_volume: %.10f\n", parsed.klines[1].quote_asset_volume);
    log_info(".klines[1].number_of_trades: %d\n", parsed.klines[1].number_of_trades);
    log_info(".klines[1].taker_buy_base_volume: %.10f\n", parsed.klines[1].taker_buy_base_volume);
    log_info(".klines[1].taker_buy_quote_volume: %.10f\n", parsed.klines[1].taker_buy_quote_volume);

  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::avgPrice_ret_t] deserialization \n");
    std::string json = "{\"mins\": 5,\"price\": \"9.35751834\",\"closeTime\": 1694061154503}";
    
    cuwacunu::camahjucunu::binance::avgPrice_ret_t parsed(json);
    
    log_info("json: %s\n", json.c_str());
    
    log_info(".mins: %d\n", parsed.mins);
    log_info(".price: %.10f\n", parsed.price);
    log_info(".close_time: %ld\n", parsed.close_time);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::ticker_24hr_ret_t] deserialization \n");
    std::string json = "{\"symbol\":\"BTCUSDT\",\"priceChange\":\"-83.13000000\",\"priceChangePercent\": \"-0.317\",\"weightedAvgPrice\":\"26234.58803036\",\"openPrice\":\"26304.80000000\",\"highPrice\":\"26397.46000000\",\"lowPrice\":\"26088.34000000\",\"lastPrice\":\"26221.67000000\",\"volume\":\"18495.35066000\",\"quoteVolume\":\"485217905.04210480\",\"openTime\":1695686400000,\"closeTime\":1695772799999,\"firstId\":3220151555,\"lastId\":3220849281,\"count\":697727}";
    
    cuwacunu::camahjucunu::binance::ticker_24hr_ret_t parsed(json);
    
    log_info("json: %s\n", json.c_str());
    
    
    log_info(".symbol: %s\n", parsed.symbol.c_str());
    log_info(".priceChange: %.10f\n", parsed.priceChange);
    log_info(".priceChangePercent: %.10f\n", parsed.priceChangePercent);
    log_info(".weightedAvgPrice: %.10f\n", parsed.weightedAvgPrice);
    log_info(".openPrice: %.10f\n", parsed.openPrice);
    log_info(".highPrice: %.10f\n", parsed.highPrice);
    log_info(".lowPrice: %.10f\n", parsed.lowPrice);
    log_info(".lastPrice: %.10f\n", parsed.lastPrice);
    log_info(".volume: %.10f\n", parsed.volume);
    log_info(".quoteVolume: %.10f\n", parsed.quoteVolume);
    log_info(".openTime: %.10f\n", parsed.openTime);
    log_info(".closeTime: %.10f\n", parsed.closeTime);
    log_info(".firstId: %.10f\n", parsed.firstId);
    log_info(".lastId: %.10f\n", parsed.lastId);
    log_info(".count: %.10f\n", parsed.count);
  }

  
  
  return 0;
}