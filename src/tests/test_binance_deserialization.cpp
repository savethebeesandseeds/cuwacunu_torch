#include "piaabo/dutils.h"
#include "piaabo/config_space.h"
#include "piaabo/security.h"
#include "piaabo/encryption.h"
#include "camahjucunu/crypto_exchange/binance/binance_enums.h"
#include "camahjucunu/crypto_exchange/binance/binance_types.h"

int main() {  
  /* test deserialization */
  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sping_ret_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{}";
    
    log_info("json: %s\n", json.c_str());

    cuwacunu::camahjucunu::binance::ping_ret_t parsed(json);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%stime_ret_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"serverTime\": 1499827319559,12,12m,1231{},{\"serverTime\": 12}}";
    log_info("json: %s\n", json.c_str());

    cuwacunu::camahjucunu::binance::time_ret_t parsed(json);

    log_info("\t.serverTime: %ld\n", parsed.serverTime);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sdepth_ret_t%s] deserialization (1) \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"lastUpdateId\": 1027024,\"bids\": [[\"4.00000000\",\"431.00000000\"]],\"asks\": [[\"4.00000200\",\"12.00000000\"]]}";
    log_info("json: %s\n", json.c_str());
    
    cuwacunu::camahjucunu::binance::depth_ret_t parsed(json);

    log_info("\t.lastUpdateId: %ld\n", parsed.lastUpdateId);
    log_info("\t.bids.length: %ld\n", parsed.bids.size());
    log_info("\t.asks.length: %ld\n", parsed.asks.size());
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sdepth_ret_t%s] deserialization (2) \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"lastUpdateId\": 1027024,\"bids\": [[\"4.00000000\",\"441.00000000\"],[\"1.00000000\",\"2.00000000\"]],\"asks\": [[\"4.00000200\",\"12.00000000\"],[\"1.00000000\",\"2.00000000\"],[\"1.00000000\",\"2.00000000\"],[\"1.00000000\",\"2.00000000\"]]}";
    log_info("json: %s\n", json.c_str());
    
    cuwacunu::camahjucunu::binance::depth_ret_t parsed(json);

    log_info("\t.lastUpdateId: %ld\n", parsed.lastUpdateId);
    log_info("\t.bids.length: %ld\n", parsed.bids.size());
    log_info("\t.asks.length: %ld\n", parsed.asks.size());

  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%strades_ret_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "[{\"id\": 28457,\"price\": \"4.00000100\",\"qty\": \"12.00000000\",\"quoteQty\": \"48.000012\",\"time\": 1499865549590,\"isBuyerMaker\": true,\"isBestMatch\": true}]";
    log_info("json: %s\n", json.c_str());

    cuwacunu::camahjucunu::binance::trades_ret_t parsed(json);

    log_info("\t.trades.size(): %ld\n", parsed.trades.size());
    log_info("\t.trades[0].id: %ld\n", parsed.trades[0].id);
    log_info("\t.trades[0].price: %.10f\n", parsed.trades[0].price);
    log_info("\t.trades[0].qty: %.10f\n", parsed.trades[0].qty);
    log_info("\t.trades[0].quoteQty: %.10f\n", parsed.trades[0].quoteQty);
    log_info("\t.trades[0].time: %ld\n", parsed.trades[0].time);
    log_info("\t.trades[0].isBuyerMaker: %d\n", parsed.trades[0].isBuyerMaker);
    log_info("\t.trades[0].isBestMatch: %d\n", parsed.trades[0].isBestMatch);

  }
  
  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%shistoricalTrades_ret_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "[{\"id\": 2812467,\"price\": \"411.00000100\",\"qty\": \"142.00000000\",\"quoteQty\": \"4518.000012\",\"time\": 1149986664990,\"isBuyerMaker\": false,\"isBestMatch\": true}, {\"id\": 28467,\"price\": \"4.00000100\",\"qty\": \"12.00000000\",\"quoteQty\": \"48.000012\",\"time\": 1499866649690,\"isBuyerMaker\": true,\"isBestMatch\": false}]";
    log_info("json: %s\n", json.c_str());

    cuwacunu::camahjucunu::binance::historicalTrades_ret_t parsed(json);
    
    log_info("\t.trades.size(): %ld\n", parsed.trades.size());
    log_info("\t.trades[0].id: %ld\n", parsed.trades[0].id);
    log_info("\t.trades[0].price: %.10f\n", parsed.trades[0].price);
    log_info("\t.trades[0].qty: %.10f\n", parsed.trades[0].qty);
    log_info("\t.trades[0].quoteQty: %.10f\n", parsed.trades[0].quoteQty);
    log_info("\t.trades[0].time: %ld\n", parsed.trades[0].time);
    log_info("\t.trades[0].isBuyerMaker: %d\n", parsed.trades[0].isBuyerMaker);
    log_info("\t.trades[0].isBestMatch: %d\n", parsed.trades[0].isBestMatch);
    
    log_info("\t.trades[1].id: %ld\n", parsed.trades[1].id);
    log_info("\t.trades[1].price: %.10f\n", parsed.trades[1].price);
    log_info("\t.trades[1].qty: %.10f\n", parsed.trades[1].qty);
    log_info("\t.trades[1].quoteQty: %.10f\n", parsed.trades[1].quoteQty);
    log_info("\t.trades[1].time: %ld\n", parsed.trades[1].time);
    log_info("\t.trades[1].isBuyerMaker: %d\n", parsed.trades[1].isBuyerMaker);
    log_info("\t.trades[1].isBestMatch: %d\n", parsed.trades[1].isBestMatch);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sklines_ret_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "[[1499040000000,\"0.01634790\",\"0.80000000\",\"0.01575800\",\"0.01577100\",\"148976.11427815\",1499644799999,\"2434.19055334\",308,\"1756.87402397\",\"28.46694368\",\"0\"],[1499040000111,\"1.11634791\",\"1.81111111\",\"1.11575811\",\"1.11577111\",\"148976.11427815\",1499644799999,\"2434.19155334\",318,\"1756.87412397\",\"28.46694368\",\"1\"]]";
    log_info("json: %s\n", json.c_str());
    
    cuwacunu::camahjucunu::binance::klines_ret_t parsed(json);
    
    log_info("\t.klines.size(): %ld\n", parsed.klines.size());
    
    log_info("\t.klines[0].open_time: %ld\n", parsed.klines[0].open_time);
    log_info("\t.klines[0].open_price: %.10f\n", parsed.klines[0].open_price);
    log_info("\t.klines[0].high_price: %.10f\n", parsed.klines[0].high_price);
    log_info("\t.klines[0].low_price: %.10f\n", parsed.klines[0].low_price);
    log_info("\t.klines[0].close_price: %.10f\n", parsed.klines[0].close_price);
    log_info("\t.klines[0].volume: %.10f\n", parsed.klines[0].volume);
    log_info("\t.klines[0].close_time: %ld\n", parsed.klines[0].close_time);
    log_info("\t.klines[0].quote_asset_volume: %.10f\n", parsed.klines[0].quote_asset_volume);
    log_info("\t.klines[0].number_of_trades: %d\n", parsed.klines[0].number_of_trades);
    log_info("\t.klines[0].taker_buy_base_volume: %.10f\n", parsed.klines[0].taker_buy_base_volume);
    log_info("\t.klines[0].taker_buy_quote_volume: %.10f\n", parsed.klines[0].taker_buy_quote_volume);

    log_info("\t.klines[1].open_time: %ld\n", parsed.klines[1].open_time);
    log_info("\t.klines[1].open_price: %.10f\n", parsed.klines[1].open_price);
    log_info("\t.klines[1].high_price: %.10f\n", parsed.klines[1].high_price);
    log_info("\t.klines[1].low_price: %.10f\n", parsed.klines[1].low_price);
    log_info("\t.klines[1].close_price: %.10f\n", parsed.klines[1].close_price);
    log_info("\t.klines[1].volume: %.10f\n", parsed.klines[1].volume);
    log_info("\t.klines[1].close_time: %ld\n", parsed.klines[1].close_time);
    log_info("\t.klines[1].quote_asset_volume: %.10f\n", parsed.klines[1].quote_asset_volume);
    log_info("\t.klines[1].number_of_trades: %d\n", parsed.klines[1].number_of_trades);
    log_info("\t.klines[1].taker_buy_base_volume: %.10f\n", parsed.klines[1].taker_buy_base_volume);
    log_info("\t.klines[1].taker_buy_quote_volume: %.10f\n", parsed.klines[1].taker_buy_quote_volume);

  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%savgPrice_ret_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"mins\": 5,\"price\": \"9.35751834\",\"closeTime\": 1694061154503}";
    log_info("json: %s\n", json.c_str());
    
    cuwacunu::camahjucunu::binance::avgPrice_ret_t parsed(json);
    
    log_info("\t.mins: %d\n", parsed.mins);
    log_info("\t.price: %.10f\n", parsed.price);
    log_info("\t.close_time: %ld\n", parsed.close_time);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sticker_24hr_ret_t%s] (full) deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"symbol\":\"BTCUSDT\",\"priceChange\":\"-83.13000000\",\"priceChangePercent\": \"-0.317\",\"weightedAvgPrice\":\"26234.58803036\",\"openPrice\":\"26304.80000000\",\"highPrice\":\"26397.46000000\",\"lowPrice\":\"26088.34000000\",\"lastPrice\":\"26221.67000000\",\"volume\":\"18495.35066000\",\"quoteVolume\":\"485217905.04210480\",\"openTime\":1695686400000,\"closeTime\":1695772799999,\"firstId\":3220151555,\"lastId\":3220849281,\"count\":697727}";
    log_info("json: %s\n", json.c_str());
    
    cuwacunu::camahjucunu::binance::ticker_24hr_ret_t parsed(json);
    
    ASSERT(parsed.is_full, "ticker_24hr_ret_t is expected of type <tick_mini_t>");
    
    log_info("\t.symbol: %s\n", GET_TICK_FULL(parsed).symbol.c_str());
    log_info("\t.priceChange: %.10f\n", GET_TICK_FULL(parsed).priceChange);
    log_info("\t.priceChangePercent: %.10f\n", GET_TICK_FULL(parsed).priceChangePercent);
    log_info("\t.weightedAvgPrice: %.10f\n", GET_TICK_FULL(parsed).weightedAvgPrice);
    log_info("\t.openPrice: %.10f\n", GET_TICK_FULL(parsed).openPrice);
    log_info("\t.highPrice: %.10f\n", GET_TICK_FULL(parsed).highPrice);
    log_info("\t.lowPrice: %.10f\n", GET_TICK_FULL(parsed).lowPrice);
    log_info("\t.lastPrice: %.10f\n", GET_TICK_FULL(parsed).lastPrice);
    log_info("\t.volume: %.10f\n", GET_TICK_FULL(parsed).volume);
    log_info("\t.quoteVolume: %.10f\n", GET_TICK_FULL(parsed).quoteVolume);
    log_info("\t.openTime: %ld\n", GET_TICK_FULL(parsed).openTime);
    log_info("\t.closeTime: %ld\n", GET_TICK_FULL(parsed).closeTime);
    log_info("\t.firstId: %ld\n", GET_TICK_FULL(parsed).firstId);
    log_info("\t.lastId: %ld\n", GET_TICK_FULL(parsed).lastId);
    log_info("\t.count: %d\n", GET_TICK_FULL(parsed).count);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sticker_24hr_ret_t%s] (mini) deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"symbol\":\"BTCUSDT\",\"openPrice\":\"26304.80000000\",\"highPrice\":\"26397.46000000\",\"lowPrice\":\"26088.34000000\",\"lastPrice\":\"26221.67000000\",\"volume\":\"18495.35066000\",\"quoteVolume\":\"485217905.04210480\",\"openTime\":1695686400000,\"closeTime\":1695772799999,\"firstId\":3220151555,\"lastId\":3220849281,\"count\":697727}";
    log_info("json: %s\n", json.c_str());
    
    cuwacunu::camahjucunu::binance::ticker_24hr_ret_t parsed(json);
    
    ASSERT(!parsed.is_full, "ticker_24hr_ret_t is expected of type <tick_mini_t>");
    
    log_info("\t.symbol: %s\n", GET_TICK_MINI(parsed).symbol.c_str());
    log_info("\t.openPrice: %.10f\n", GET_TICK_MINI(parsed).openPrice);
    log_info("\t.highPrice: %.10f\n", GET_TICK_MINI(parsed).highPrice);
    log_info("\t.lowPrice: %.10f\n", GET_TICK_MINI(parsed).lowPrice);
    log_info("\t.lastPrice: %.10f\n", GET_TICK_MINI(parsed).lastPrice);
    log_info("\t.volume: %.10f\n", GET_TICK_MINI(parsed).volume);
    log_info("\t.quoteVolume: %.10f\n", GET_TICK_MINI(parsed).quoteVolume);
    log_info("\t.openTime: %ld\n", GET_TICK_MINI(parsed).openTime);
    log_info("\t.closeTime: %ld\n", GET_TICK_MINI(parsed).closeTime);
    log_info("\t.firstId: %ld\n", GET_TICK_MINI(parsed).firstId);
    log_info("\t.lastId: %ld\n", GET_TICK_MINI(parsed).lastId);
    log_info("\t.count: %d\n", GET_TICK_MINI(parsed).count);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sticker_price_ret_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "[{\"symbol\": \"LTCBTC\",\"price\": \"4.00000200\"}, {\"symbol\": \"LTCETH\",\"price\": \"7.00000200\"}]";
    log_info("json: %s\n", json.c_str());
    
    cuwacunu::camahjucunu::binance::ticker_price_ret_t parsed(json);
    
    log_info("\t.prices[0].symbol: %s\n", parsed.prices[0].symbol.c_str());
    log_info("\t.prices[0].price: %.10f\n", parsed.prices[0].price);
  }
  
  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sticker_bookTicker_ret_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "[{\"symbol\": \"LTCBTC\",\"bidPrice\": \"4.00000000\",\"bidQty\": \"431.00000000\",\"askPrice\": \"4.00000200\",\"askQty\": \"9.00000000\"},{\"symbol\": \"ETHBTC\",\"bidPrice\": \"0.07946700\",\"bidQty\": \"9.00000000\",\"askPrice\": \"100000.00000000\",\"askQty\": \"1000.00000000\"}]";
    log_info("json: %s\n", json.c_str());
    
    cuwacunu::camahjucunu::binance::ticker_bookTicker_ret_t parsed(json);
    
    log_info("\t.bookPrices[0].symbol: %s\n", parsed.bookPrices[0].symbol.c_str());
    log_info("\t.bookPrices[0].bidPrice: %.10f\n", parsed.bookPrices[0].bidPrice);
    log_info("\t.bookPrices[0].bidQty: %.10f\n", parsed.bookPrices[0].bidQty);
    log_info("\t.bookPrices[0].askPrice: %.10f\n", parsed.bookPrices[0].askPrice);
    log_info("\t.bookPrices[0].askQty: %.10f\n", parsed.bookPrices[0].askQty);

    log_info("\t.bookPrices[1].symbol: %s\n", parsed.bookPrices[1].symbol.c_str());
    log_info("\t.bookPrices[1].bidPrice: %.10f\n", parsed.bookPrices[1].bidPrice);
    log_info("\t.bookPrices[1].bidQty: %.10f\n", parsed.bookPrices[1].bidQty);
    log_info("\t.bookPrices[1].askPrice: %.10f\n", parsed.bookPrices[1].askPrice);
    log_info("\t.bookPrices[1].askQty: %.10f\n", parsed.bookPrices[1].askQty);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sticker_wind_ret_t%s] (full) deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "\"symbol\":\"BNBBTC\",\"priceChange\":\"-8.00000000\",\"priceChangePercent\":\"-88.889\",\"weightedAvgPrice\":\"2.60427807\",\"openPrice\":\"9.00000000\",\"highPrice\":\"9.00000000\",\"lowPrice\":\"1.00000000\",\"lastPrice\":\"1.00000000\",\"volume\":\"187.00000000\",\"quoteVolume\":\"487.00000000\",\"openTime\":1641859200000,\"closeTime\":1642031999999,\"firstId\":0,\"lastId\":60,\"count\":61";
    log_info("json: %s\n", json.c_str());
    
    cuwacunu::camahjucunu::binance::ticker_wind_ret_t parsed(json);
    
    ASSERT(parsed.is_full, "ticker_wind_ret_t is expected of type <tick_mini_t>");
    
    log_info("\t.symbol: %s\n", GET_TICK_FULL(parsed).symbol.c_str());
    log_info("\t.priceChange: %.10f\n", GET_TICK_FULL(parsed).priceChange);
    log_info("\t.priceChangePercent: %.10f\n", GET_TICK_FULL(parsed).priceChangePercent);
    log_info("\t.weightedAvgPrice: %.10f\n", GET_TICK_FULL(parsed).weightedAvgPrice);
    log_info("\t.openPrice: %.10f\n", GET_TICK_FULL(parsed).openPrice);
    log_info("\t.highPrice: %.10f\n", GET_TICK_FULL(parsed).highPrice);
    log_info("\t.lowPrice: %.10f\n", GET_TICK_FULL(parsed).lowPrice);
    log_info("\t.lastPrice: %.10f\n", GET_TICK_FULL(parsed).lastPrice);
    log_info("\t.volume: %.10f\n", GET_TICK_FULL(parsed).volume);
    log_info("\t.quoteVolume: %.10f\n", GET_TICK_FULL(parsed).quoteVolume);
    log_info("\t.openTime: %ld\n", GET_TICK_FULL(parsed).openTime);
    log_info("\t.closeTime: %ld\n", GET_TICK_FULL(parsed).closeTime);
    log_info("\t.firstId: %ld\n", GET_TICK_FULL(parsed).firstId);
    log_info("\t.lastId: %ld\n", GET_TICK_FULL(parsed).lastId);
    log_info("\t.count: %d\n", GET_TICK_FULL(parsed).count);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sticker_wind_ret_t%s] (mini) deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"symbol\":\"LTCBTC\",\"openPrice\":\"0.10000000\",\"highPrice\":\"2.00000000\",\"lowPrice\":\"0.10000000\",\"lastPrice\":\"2.00000000\",\"volume\":\"39.00000000\",\"quoteVolume\":\"13.40000000\",\"openTime\":1656986580000,\"closeTime\":1657001016795,\"firstId\":0,\"lastId\":34,\"count\":35}";
    log_info("json: %s\n", json.c_str());
    
    cuwacunu::camahjucunu::binance::ticker_wind_ret_t parsed(json);
    
    ASSERT(!parsed.is_full, "ticker_wind_ret_t is expected of type <tick_mini_t>");
    
    log_info("\t.symbol: %s\n", GET_TICK_MINI(parsed).symbol.c_str());
    log_info("\t.openPrice: %.10f\n", GET_TICK_MINI(parsed).openPrice);
    log_info("\t.highPrice: %.10f\n", GET_TICK_MINI(parsed).highPrice);
    log_info("\t.lowPrice: %.10f\n", GET_TICK_MINI(parsed).lowPrice);
    log_info("\t.lastPrice: %.10f\n", GET_TICK_MINI(parsed).lastPrice);
    log_info("\t.volume: %.10f\n", GET_TICK_MINI(parsed).volume);
    log_info("\t.quoteVolume: %.10f\n", GET_TICK_MINI(parsed).quoteVolume);
    log_info("\t.openTime: %ld\n", GET_TICK_MINI(parsed).openTime);
    log_info("\t.closeTime: %ld\n", GET_TICK_MINI(parsed).closeTime);
    log_info("\t.firstId: %ld\n", GET_TICK_MINI(parsed).firstId);
    log_info("\t.lastId: %ld\n", GET_TICK_MINI(parsed).lastId);
    log_info("\t.count: %d\n", GET_TICK_MINI(parsed).count);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%saccount_information_ret_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"makerCommission\": 15,\"takerCommission\": 15,\"buyerCommission\": 0,\"sellerCommission\": 0,\"commissionRates\": {\"maker\": \"0.00150000\",\"taker\": \"0.00150000\",\"buyer\": \"0.00000001\",\"seller\": \"0.10000000\"},\"canTrade\": true,\"canWithdraw\": true,\"canDeposit\": true,\"brokered\": false,\"requireSelfTradePrevention\": false,\"preventSor\": false,\"updateTime\": 123456789,\"accountType\": \"SPOT\",\"balances\": [{\"asset\": \"BTC\",\"free\": \"4723846.89208129\",\"locked\": \"1000.00000000\"},{\"asset\": \"LTC\",\"free\": \"4763368.68006011\",\"locked\": \"0.00000000\"}],\"permissions\": [\"SPOT\",\"TRD_GRP_002\"],\"uid\": 35493786}";
    log_info("json: %s\n", json.c_str());

    cuwacunu::camahjucunu::binance::account_information_ret_t parsed(json);

    log_info("\t.makerCommission: %d\n", parsed.makerCommission);
    log_info("\t.takerCommission: %d\n", parsed.takerCommission);
    log_info("\t.buyerCommission: %d\n", parsed.buyerCommission);
    log_info("\t.sellerCommission: %d\n", parsed.sellerCommission);
    log_info("\t.canTrade: %d\n", parsed.canTrade);
    log_info("\t.canWithdraw: %d\n", parsed.canWithdraw);
    log_info("\t.canDeposit: %d\n", parsed.canDeposit);
    log_info("\t.brokered: %d\n", parsed.brokered);
    log_info("\t.requireSelfTradePrevention: %d\n", parsed.requireSelfTradePrevention);
    log_info("\t.preventSor: %d\n", parsed.preventSor);
    log_info("\t.updateTime: %ld\n", parsed.updateTime);
    log_info("\t.uid: %ld\n", parsed.uid);
    log_info("\t.accountType: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.accountType).c_str());
    log_info("\t.permissions[0]: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.permissions[0]).c_str());
    log_info("\t.permissions[1]: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.permissions[1]).c_str());
    log_info("\t.commissionRates.maker: %.10f\n", parsed.commissionRates.maker);
    log_info("\t.commissionRates.taker: %.10f\n", parsed.commissionRates.taker);
    log_info("\t.commissionRates.buyer: %.10f\n", parsed.commissionRates.buyer);
    log_info("\t.commissionRates.seller: %.10f\n", parsed.commissionRates.seller);
    log_info("\t.balances[0].asset: %s\n", parsed.balances[0].asset.c_str());
    log_info("\t.balances[0].free: %.10f\n", parsed.balances[0].free);
    log_info("\t.balances[0].locked: %.10f\n", parsed.balances[0].locked);
    log_info("\t.balances[1].asset: %s\n", parsed.balances[1].asset.c_str());
    log_info("\t.balances[1].free: %.10f\n", parsed.balances[1].free);
    log_info("\t.balances[1].locked: %.10f\n", parsed.balances[1].locked);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%saccount_trade_list_ret_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "[{\"symbol\": \"BNBBTC\",\"id\": 28457,\"orderId\": 100234,\"orderListId\": -1,\"price\": \"4.00000100\",\"qty\": \"12.00000000\",\"quoteQty\": \"48.000012\",\"commission\": \"10.10000000\",\"commissionAsset\": \"BNB\",\"time\": 1499865549590,\"isBuyer\": true,\"isMaker\": false,\"isBestMatch\": true}]";
    log_info("json: %s\n", json.c_str());

    cuwacunu::camahjucunu::binance::account_trade_list_ret_t parsed(json);

    log_info("\t.trades[0].symbol: %s\n", parsed.trades[0].symbol.c_str());
    log_info("\t.trades[0].id: %d\n", parsed.trades[0].id);
    log_info("\t.trades[0].orderId: %d\n", parsed.trades[0].orderId);
    log_info("\t.trades[0].orderListId: %d\n", parsed.trades[0].orderListId);
    log_info("\t.trades[0].price: %.10f\n", parsed.trades[0].price);
    log_info("\t.trades[0].qty: %.10f\n", parsed.trades[0].qty);
    log_info("\t.trades[0].quoteQty: %.10f\n", parsed.trades[0].quoteQty);
    log_info("\t.trades[0].commission: %.10f\n", parsed.trades[0].commission);
    log_info("\t.trades[0].commissionAsset: %s\n", parsed.trades[0].commissionAsset.c_str());
    log_info("\t.trades[0].time: %ld\n", parsed.trades[0].time);
    log_info("\t.trades[0].isBuyer: %d\n", parsed.trades[0].isBuyer);
    log_info("\t.trades[0].isMaker: %d\n", parsed.trades[0].isMaker);
    log_info("\t.trades[0].isBestMatch: %d\n", parsed.trades[0].isBestMatch);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%squery_commision_rates_ret_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"symbol\": \"BTCUSDT\",\"standardCommission\": {\"maker\": \"0.00000010\",\"taker\": \"0.00000020\",\"buyer\": \"0.00000030\",\"seller\": \"0.00000040\" },\"taxCommission\": {\"maker\": \"0.00000112\",\"taker\": \"0.00000114\",\"buyer\": \"0.00000118\",\"seller\": \"0.00000116\" },\"discount\": {\"enabledForAccount\": true,\"enabledForSymbol\": false,\"discountAsset\": \"BNB\",\"discount\": \"0.75000000\"}}";
    log_info("json: %s\n", json.c_str());

    cuwacunu::camahjucunu::binance::query_commision_rates_ret_t parsed(json);

    log_info("\t.symbol: %s\n", parsed.symbol.c_str());
    log_info("\t.standardCommission.maker: %.10f\n", parsed.standardCommission.maker);
    log_info("\t.standardCommission.taker: %.10f\n", parsed.standardCommission.taker);
    log_info("\t.standardCommission.buyer: %.10f\n", parsed.standardCommission.buyer);
    log_info("\t.standardCommission.seller: %.10f\n", parsed.standardCommission.seller);
    log_info("\t.taxCommission.maker: %.10f\n", parsed.taxCommission.maker);
    log_info("\t.taxCommission.taker: %.10f\n", parsed.taxCommission.taker);
    log_info("\t.taxCommission.buyer: %.10f\n", parsed.taxCommission.buyer);
    log_info("\t.taxCommission.seller: %.10f\n", parsed.taxCommission.seller);
    log_info("\t.discount.enabledForAccount: %d\n", parsed.discount.enabledForAccount);
    log_info("\t.discount.enabledForSymbol: %d\n", parsed.discount.enabledForSymbol);
    log_info("\t.discount.discountAsset: %s\n", parsed.discount.discountAsset.c_str());
    log_info("\t.discount.discount: %.10f\n", parsed.discount.discount);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sorder_ack_resp_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"symbol\": \"BTCUSDT\",\"orderId\": 28,\"orderListId\": -1,\"clientOrderId\": \"6gCrw2kRUAF9CvJDGP16IP\",\"transactTime\": 1507725176595}";
    log_info("json: %s\n", json.c_str());

    cuwacunu::camahjucunu::binance::order_ack_resp_t parsed(json);

    log_info("\t.symbol: %s\n", parsed.symbol.c_str());
    log_info("\t.orderId: %d\n", parsed.orderId);
    log_info("\t.orderListId: %d\n", parsed.orderListId);
    log_info("\t.clientOrderId: %s\n", parsed.clientOrderId.c_str());
    log_info("\t.transactTime: %ld\n", parsed.transactTime);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sorder_full_resp_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"symbol\": \"BTCUSDT\",\"orderId\": 28,\"orderListId\": -1,\"clientOrderId\": \"6gCrw2kRUAF9CvJDGP16IP\",\"transactTime\": 1507725176595,\"price\": \"0.00000000\",\"origQty\": \"10.00000000\",\"executedQty\": \"10.00000000\",\"cummulativeQuoteQty\": \"10.00000000\",\"status\": \"FILLED\",\"timeInForce\": \"GTC\",\"type\": \"MARKET\",\"side\": \"SELL\",\"workingTime\": 1507725176595,\"selfTradePreventionMode\": \"NONE\",\"fills\": [{\"price\": \"4000.00000000\",\"qty\": \"1.00000000\",\"commission\": \"4.00000000\",\"commissionAsset\": \"USDT\",\"tradeId\": 56},{\"price\": \"3999.00000000\",\"qty\": \"5.00000000\",\"commission\": \"19.99500000\",\"commissionAsset\": \"USDT\",\"tradeId\": 57}]}";
    log_info("json: %s\n", json.c_str());

    cuwacunu::camahjucunu::binance::order_full_resp_t parsed(json);

    log_info("\t.result.symbol: %s\n", parsed.result.symbol.c_str());
    log_info("\t.result.orderId: %d\n", parsed.result.orderId);
    log_info("\t.result.orderListId: %d\n", parsed.result.orderListId);
    log_info("\t.result.clientOrderId: %s\n", parsed.result.clientOrderId.c_str());
    log_info("\t.result.transactTime: %ld\n", parsed.result.transactTime);
    log_info("\t.result.origQty: %.10f\n", parsed.result.origQty);
    log_info("\t.result.executedQty: %.10f\n", parsed.result.executedQty);
    log_info("\t.result.cummulativeQuoteQty: %.10f\n", parsed.result.cummulativeQuoteQty);
    log_info("\t.result.workingTime: %ld\n", parsed.result.workingTime);
    log_info("\t.result.status: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.result.status).c_str());
    log_info("\t.result.timeInForce: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.result.timeInForce).c_str());
    log_info("\t.result.type: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.result.type).c_str());
    log_info("\t.result.side: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.result.side).c_str());
    log_info("\t.result.selfTradePreventionMode: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.result.selfTradePreventionMode).c_str());
    log_info("\t.fills[0].price: %.10f\n", parsed.fills[0].price);
    log_info("\t.fills[0].qty: %.10f\n", parsed.fills[0].qty);
    log_info("\t.fills[0].commission: %.10f\n", parsed.fills[0].commission);
    log_info("\t.fills[0].commissionAsset: %s\n", parsed.fills[0].commissionAsset.c_str());
    log_info("\t.fills[0].tradeId: %d\n", parsed.fills[0].tradeId);
    log_info("\t.fills[1].price: %.10f\n", parsed.fills[1].price);
    log_info("\t.fills[1].qty: %.10f\n", parsed.fills[1].qty);
    log_info("\t.fills[1].commission: %.10f\n", parsed.fills[1].commission);
    log_info("\t.fills[1].commissionAsset: %s\n", parsed.fills[1].commissionAsset.c_str());
    log_info("\t.fills[1].tradeId: %d\n", parsed.fills[1].tradeId);
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sorder_sor_full_resp_t%s] deserialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    std::string json = "{\"symbol\": \"BTCUSDT\",\"orderId\": 2,\"orderListId\": -1,\"clientOrderId\": \"sBI1KM6nNtOfj5tccZSKly\",\"transactTime\": 1689149087774,\"price\": \"31000.00000000\",\"origQty\": \"0.50000000\",\"executedQty\": \"0.50000000\",\"cummulativeQuoteQty\": \"14000.00000000\",\"status\": \"FILLED\",\"timeInForce\": \"GTC\",\"type\": \"LIMIT\",\"side\": \"BUY\",\"workingTime\": 1689149087774,\"fills\": [{\"matchType\": \"ONE_PARTY_TRADE_REPORT\",\"price\": \"28000.00000000\",\"qty\": \"0.50000000\",\"commission\": \"0.00000000\",\"commissionAsset\": \"BTC\",\"tradeId\": -1,\"allocId\": 0},{\"matchType\": \"ONE_PARTY_TRADE_REPORT\",\"price\": \"28000.00000000\",\"qty\": \"0.50000000\",\"commission\": \"0.00000000\",\"commissionAsset\": \"BTC\",\"tradeId\": -1,\"allocId\": 0}],\"workingFloor\": \"SOR\",\"selfTradePreventionMode\": \"NONE\",\"usedSor\": true}";
    log_info("json: %s\n", json.c_str());

    cuwacunu::camahjucunu::binance::order_sor_full_resp_t parsed(json);

    log_info("\t.price: %.10f\n", parsed.price);
    log_info("\t.workingFloor: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.workingFloor).c_str());
    log_info("\t.usedSor: %d\n", parsed.usedSor);
    log_info("\t.result.symbol: %s\n", parsed.result.symbol.c_str());
    log_info("\t.result.orderId: %d\n", parsed.result.orderId);
    log_info("\t.result.orderListId: %d\n", parsed.result.orderListId);
    log_info("\t.result.clientOrderId: %s\n", parsed.result.clientOrderId.c_str());
    log_info("\t.result.transactTime: %ld\n", parsed.result.transactTime);
    log_info("\t.result.origQty: %.10f\n", parsed.result.origQty);
    log_info("\t.result.executedQty: %.10f\n", parsed.result.executedQty);
    log_info("\t.result.cummulativeQuoteQty: %.10f\n", parsed.result.cummulativeQuoteQty);
    log_info("\t.result.workingTime: %ld\n", parsed.result.workingTime);
    log_info("\t.result.status: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.result.status).c_str());
    log_info("\t.result.timeInForce: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.result.timeInForce).c_str());
    log_info("\t.result.type: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.result.type).c_str());
    log_info("\t.result.side: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.result.side).c_str());
    log_info("\t.result.selfTradePreventionMode: %s\n", cuwacunu::camahjucunu::binance::enum_to_string(parsed.result.selfTradePreventionMode).c_str());
    log_info("\t.fills[0].matchType: %s\n", parsed.fills[0].matchType.c_str());
    log_info("\t.fills[0].price: %.10f\n", parsed.fills[0].price);
    log_info("\t.fills[0].qty: %.10f\n", parsed.fills[0].qty);
    log_info("\t.fills[0].commission: %.10f\n", parsed.fills[0].commission);
    log_info("\t.fills[0].commissionAsset: %s\n", parsed.fills[0].commissionAsset.c_str());
    log_info("\t.fills[0].tradeId: %d\n", parsed.fills[0].tradeId);
    log_info("\t.fills[0].allocId: %d\n", parsed.fills[0].allocId);
    log_info("\t.fills[1].matchType: %s\n", parsed.fills[1].matchType.c_str());
    log_info("\t.fills[1].price: %.10f\n", parsed.fills[1].price);
    log_info("\t.fills[1].qty: %.10f\n", parsed.fills[1].qty);
    log_info("\t.fills[1].commission: %.10f\n", parsed.fills[1].commission);
    log_info("\t.fills[1].commissionAsset: %s\n", parsed.fills[1].commissionAsset.c_str());
    log_info("\t.fills[1].tradeId: %d\n", parsed.fills[1].tradeId);
    log_info("\t.fills[1].allocId: %d\n", parsed.fills[1].allocId);
    
  }

  return 0;
}