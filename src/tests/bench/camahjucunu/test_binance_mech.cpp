#include <cassert>
#include "piaabo/dutils.h"
#include "camahjucunu/exchange/binance/biannce_mech.h"

int main() {
    
    auto exchange_mech = cuwacunu::camahjucunu::exchange::binance::binance_mech_t(
        cuwacunu::camahjucunu::exchange::binance::mech_type_e::TESTNET
    );

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s ping_ret_t %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(ping);
        std::optional<cuwacunu::camahjucunu::exchange::ping_ret_t> server_ret = 
            exchange_mech.ping(cuwacunu::camahjucunu::exchange::ping_args_t(
            ), true);
        PRINT_TOCK_ns(ping);
        assert(server_ret.has_value());

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s time_ret_t %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(time);
        std::optional<cuwacunu::camahjucunu::exchange::time_ret_t> server_ret = 
            exchange_mech.time(cuwacunu::camahjucunu::exchange::time_args_t(
            ), true);
        PRINT_TOCK_ns(time);
        assert(server_ret.has_value());

        log_info("\t.frame_id: %s\n",       server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",    server_ret.value().frame_rsp.http_status);
        log_info("\t.serverTime: %ld\n",    server_ret.value().serverTime);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s depth_ret_t %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(depth);
        std::optional<cuwacunu::camahjucunu::exchange::depth_ret_t> server_ret = 
            exchange_mech.depth(cuwacunu::camahjucunu::exchange::depth_args_t(
                "BTCTUSD"
            ), true);
        PRINT_TOCK_ns(depth);
        assert(server_ret.has_value());

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);
        
        log_info("\t.lastUpdateId: %ld\n",          server_ret.value().lastUpdateId);
        log_info("\t.bids.length: %ld\n",           server_ret.value().bids.size());
        log_info("\t.asks.length: %ld\n",           server_ret.value().asks.size());
        if(server_ret.value().bids.size() > 0) {
            log_info("\t.bids[0].price: %.10f\n",   server_ret.value().bids[0].price);
            log_info("\t.bids[0].qty: %.10f\n",     server_ret.value().bids[0].qty);
        }
        if(server_ret.value().asks.size() > 0) {
            log_info("\t.asks[0].price: %.10f\n",   server_ret.value().asks[0].price);
            log_info("\t.asks[0].qty: %.10f\n",     server_ret.value().asks[0].qty);
        }
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s klines_ret_t (interval_1s) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(klines);
        std::optional<cuwacunu::camahjucunu::exchange::klines_ret_t> server_ret = 
            exchange_mech.klines(cuwacunu::camahjucunu::exchange::klines_args_t(
                "BTCTUSD", 
                cuwacunu::camahjucunu::exchange::interval_type_e::interval_1s
            ), true);
        PRINT_TOCK_ns(klines);
        assert(server_ret.has_value());

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);
        
        log_info("\t.klines.size(): %ld\n", server_ret.value().klines.size());

        if(server_ret.value().klines.size() > 0) {
            log_info("\t.klines[0].open_time: %ld\n",                   server_ret.value().klines[0].open_time);
            log_info("\t.klines[0].open_price: %.10f\n",                server_ret.value().klines[0].open_price);
            log_info("\t.klines[0].high_price: %.10f\n",                server_ret.value().klines[0].high_price);
            log_info("\t.klines[0].low_price: %.10f\n",                 server_ret.value().klines[0].low_price);
            log_info("\t.klines[0].close_price: %.10f\n",               server_ret.value().klines[0].close_price);
            log_info("\t.klines[0].volume: %.10f\n",                    server_ret.value().klines[0].volume);
            log_info("\t.klines[0].close_time: %ld\n",                  server_ret.value().klines[0].close_time);
            log_info("\t.klines[0].quote_asset_volume: %.10f\n",        server_ret.value().klines[0].quote_asset_volume);
            log_info("\t.klines[0].number_of_trades: %d\n",             server_ret.value().klines[0].number_of_trades);
            log_info("\t.klines[0].taker_buy_base_volume: %.10f\n",     server_ret.value().klines[0].taker_buy_base_volume);
            log_info("\t.klines[0].taker_buy_quote_volume: %.10f\n",    server_ret.value().klines[0].taker_buy_quote_volume);
        }
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s klines_ret_t (interval_1d) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(klines);
        std::optional<cuwacunu::camahjucunu::exchange::klines_ret_t> server_ret = 
            exchange_mech.klines(cuwacunu::camahjucunu::exchange::klines_args_t(
                "BTCTUSD", 
                cuwacunu::camahjucunu::exchange::interval_type_e::interval_1d
            ), true);
        PRINT_TOCK_ns(klines);
        assert(server_ret.has_value());

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);
        
        log_info("\t.klines.size(): %ld\n", server_ret.value().klines.size());

        if(server_ret.value().klines.size() > 0) {
            log_info("\t.klines[0].open_time: %ld\n",                   server_ret.value().klines[0].open_time);
            log_info("\t.klines[0].open_price: %.10f\n",                server_ret.value().klines[0].open_price);
            log_info("\t.klines[0].high_price: %.10f\n",                server_ret.value().klines[0].high_price);
            log_info("\t.klines[0].low_price: %.10f\n",                 server_ret.value().klines[0].low_price);
            log_info("\t.klines[0].close_price: %.10f\n",               server_ret.value().klines[0].close_price);
            log_info("\t.klines[0].volume: %.10f\n",                    server_ret.value().klines[0].volume);
            log_info("\t.klines[0].close_time: %ld\n",                  server_ret.value().klines[0].close_time);
            log_info("\t.klines[0].quote_asset_volume: %.10f\n",        server_ret.value().klines[0].quote_asset_volume);
            log_info("\t.klines[0].number_of_trades: %d\n",             server_ret.value().klines[0].number_of_trades);
            log_info("\t.klines[0].taker_buy_base_volume: %.10f\n",     server_ret.value().klines[0].taker_buy_base_volume);
            log_info("\t.klines[0].taker_buy_quote_volume: %.10f\n",    server_ret.value().klines[0].taker_buy_quote_volume);
        }
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s avgPrice_ret_t %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(avgPrice);
        std::optional<cuwacunu::camahjucunu::exchange::avgPrice_ret_t> server_ret = 
            exchange_mech.avgPrice(cuwacunu::camahjucunu::exchange::avgPrice_args_t(
                "BTCTUSD"
            ), true);
        PRINT_TOCK_ns(avgPrice);
        assert(server_ret.has_value());

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);
        
        log_info("\t.mins: %d\n",                   server_ret.value().mins);
        log_info("\t.price: %.10f\n",               server_ret.value().price);
        log_info("\t.close_time: %ld\n",            server_ret.value().close_time);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s ticker_ret_t (single_symbol:interval_1m:FULL) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(ticker);
            std::optional<cuwacunu::camahjucunu::exchange::ticker_ret_t> server_ret = 
            exchange_mech.ticker(cuwacunu::camahjucunu::exchange::ticker_args_t(
                "BTCTUSD", 
                cuwacunu::camahjucunu::exchange::ticker_interval_e::interval_1m, 
                cuwacunu::camahjucunu::exchange::ticker_type_e::FULL
            ), true);
        PRINT_TOCK_ns(ticker);
        assert(server_ret.has_value());
        assert(server_ret.value().is_full);

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);

        log_info("\t.symbol: %s\n",                 GET_TICK_FULL(server_ret.value()).symbol.c_str());
        log_info("\t.priceChange: %.10f\n",         GET_TICK_FULL(server_ret.value()).priceChange);
        log_info("\t.priceChangePercent: %.10f\n",  GET_TICK_FULL(server_ret.value()).priceChangePercent);
        log_info("\t.weightedAvgPrice: %.10f\n",    GET_TICK_FULL(server_ret.value()).weightedAvgPrice);
        log_info("\t.openPrice: %.10f\n",           GET_TICK_FULL(server_ret.value()).openPrice);
        log_info("\t.highPrice: %.10f\n",           GET_TICK_FULL(server_ret.value()).highPrice);
        log_info("\t.lowPrice: %.10f\n",            GET_TICK_FULL(server_ret.value()).lowPrice);
        log_info("\t.lastPrice: %.10f\n",           GET_TICK_FULL(server_ret.value()).lastPrice);
        log_info("\t.volume: %.10f\n",              GET_TICK_FULL(server_ret.value()).volume);
        log_info("\t.quoteVolume: %.10f\n",         GET_TICK_FULL(server_ret.value()).quoteVolume);
        log_info("\t.openTime: %ld\n",              GET_TICK_FULL(server_ret.value()).openTime);
        log_info("\t.closeTime: %ld\n",             GET_TICK_FULL(server_ret.value()).closeTime);
        log_info("\t.firstId: %ld\n",               GET_TICK_FULL(server_ret.value()).firstId);
        log_info("\t.lastId: %d\n",                 GET_TICK_FULL(server_ret.value()).lastId);
        log_info("\t.count: %d\n",                  GET_TICK_FULL(server_ret.value()).count);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s ticker_ret_t (multiple_symbol:interval_12h:FULL) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(ticker);
        std::optional<cuwacunu::camahjucunu::exchange::ticker_ret_t> server_ret = 
            exchange_mech.ticker(cuwacunu::camahjucunu::exchange::ticker_args_t(
                std::vector<std::string>({"BTCTUSD", "BNBBTC"}),
                cuwacunu::camahjucunu::exchange::ticker_interval_e::interval_12h, 
                cuwacunu::camahjucunu::exchange::ticker_type_e::FULL
            ), true);
        PRINT_TOCK_ns(ticker);
        assert(server_ret.has_value());
        assert(server_ret.value().is_full);

        log_info("\t.frame_id: %s\n",                 server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",              server_ret.value().frame_rsp.http_status);

        log_info("\t[0].symbol: %s\n",                GET_VECT_TICK_FULL(server_ret.value())[0].symbol.c_str());
        log_info("\t[0].priceChange: %.10f\n",        GET_VECT_TICK_FULL(server_ret.value())[0].priceChange);
        log_info("\t[0].priceChangePercent: %.10f\n", GET_VECT_TICK_FULL(server_ret.value())[0].priceChangePercent);
        log_info("\t[0].weightedAvgPrice: %.10f\n",   GET_VECT_TICK_FULL(server_ret.value())[0].weightedAvgPrice);
        log_info("\t[0].openPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[0].openPrice);
        log_info("\t[0].highPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[0].highPrice);
        log_info("\t[0].lowPrice: %.10f\n",           GET_VECT_TICK_FULL(server_ret.value())[0].lowPrice);
        log_info("\t[0].lastPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[0].lastPrice);
        log_info("\t[0].volume: %.10f\n",             GET_VECT_TICK_FULL(server_ret.value())[0].volume);
        log_info("\t[0].quoteVolume: %.10f\n",        GET_VECT_TICK_FULL(server_ret.value())[0].quoteVolume);
        log_info("\t[0].openTime: %ld\n",             GET_VECT_TICK_FULL(server_ret.value())[0].openTime);
        log_info("\t[0].closeTime: %ld\n",            GET_VECT_TICK_FULL(server_ret.value())[0].closeTime);
        log_info("\t[0].firstId: %ld\n",              GET_VECT_TICK_FULL(server_ret.value())[0].firstId);
        log_info("\t[0].lastId: %d\n",                GET_VECT_TICK_FULL(server_ret.value())[0].lastId);
        log_info("\t[0].count: %d\n",                 GET_VECT_TICK_FULL(server_ret.value())[0].count);

        log_info("\t[1].symbol: %s\n",                GET_VECT_TICK_FULL(server_ret.value())[1].symbol.c_str());
        log_info("\t[1].priceChange: %.10f\n",        GET_VECT_TICK_FULL(server_ret.value())[1].priceChange);
        log_info("\t[1].priceChangePercent: %.10f\n", GET_VECT_TICK_FULL(server_ret.value())[1].priceChangePercent);
        log_info("\t[1].weightedAvgPrice: %.10f\n",   GET_VECT_TICK_FULL(server_ret.value())[1].weightedAvgPrice);
        log_info("\t[1].openPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[1].openPrice);
        log_info("\t[1].highPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[1].highPrice);
        log_info("\t[1].lowPrice: %.10f\n",           GET_VECT_TICK_FULL(server_ret.value())[1].lowPrice);
        log_info("\t[1].lastPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[1].lastPrice);
        log_info("\t[1].volume: %.10f\n",             GET_VECT_TICK_FULL(server_ret.value())[1].volume);
        log_info("\t[1].quoteVolume: %.10f\n",        GET_VECT_TICK_FULL(server_ret.value())[1].quoteVolume);
        log_info("\t[1].openTime: %ld\n",             GET_VECT_TICK_FULL(server_ret.value())[1].openTime);
        log_info("\t[1].closeTime: %ld\n",            GET_VECT_TICK_FULL(server_ret.value())[1].closeTime);
        log_info("\t[1].firstId: %ld\n",              GET_VECT_TICK_FULL(server_ret.value())[1].firstId);
        log_info("\t[1].lastId: %d\n",                GET_VECT_TICK_FULL(server_ret.value())[1].lastId);
        log_info("\t[1].count: %d\n",                 GET_VECT_TICK_FULL(server_ret.value())[1].count);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s ticker_ret_t (single_symbol:interval_1d:MINI) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(ticker);
            std::optional<cuwacunu::camahjucunu::exchange::ticker_ret_t> server_ret = 
            exchange_mech.ticker(cuwacunu::camahjucunu::exchange::ticker_args_t(
                "BTCTUSD", 
                cuwacunu::camahjucunu::exchange::ticker_interval_e::interval_1d, 
                cuwacunu::camahjucunu::exchange::ticker_type_e::MINI
            ), true);
        PRINT_TOCK_ns(ticker);
        assert(server_ret.has_value());
        assert(!server_ret.value().is_full);

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);

        log_info("\t.symbol: %s\n",                 GET_TICK_MINI(server_ret.value()).symbol.c_str());
        log_info("\t.openPrice: %.10f\n",           GET_TICK_MINI(server_ret.value()).openPrice);
        log_info("\t.highPrice: %.10f\n",           GET_TICK_MINI(server_ret.value()).highPrice);
        log_info("\t.lowPrice: %.10f\n",            GET_TICK_MINI(server_ret.value()).lowPrice);
        log_info("\t.lastPrice: %.10f\n",           GET_TICK_MINI(server_ret.value()).lastPrice);
        log_info("\t.volume: %.10f\n",              GET_TICK_MINI(server_ret.value()).volume);
        log_info("\t.quoteVolume: %.10f\n",         GET_TICK_MINI(server_ret.value()).quoteVolume);
        log_info("\t.openTime: %ld\n",              GET_TICK_MINI(server_ret.value()).openTime);
        log_info("\t.closeTime: %ld\n",             GET_TICK_MINI(server_ret.value()).closeTime);
        log_info("\t.firstId: %ld\n",               GET_TICK_MINI(server_ret.value()).firstId);
        log_info("\t.lastId: %d\n",                 GET_TICK_MINI(server_ret.value()).lastId);
        log_info("\t.count: %d\n",                  GET_TICK_MINI(server_ret.value()).count);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s ticker_ret_t (multiple_symbol:interval_7d:MINI) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(ticker);
        std::optional<cuwacunu::camahjucunu::exchange::ticker_ret_t> server_ret = 
            exchange_mech.ticker(cuwacunu::camahjucunu::exchange::ticker_args_t(
                std::vector<std::string>({"BTCTUSD", "BNBBTC"}),
                cuwacunu::camahjucunu::exchange::ticker_interval_e::interval_7d, 
                cuwacunu::camahjucunu::exchange::ticker_type_e::MINI
            ), true);
        PRINT_TOCK_ns(ticker);
        assert(server_ret.has_value());
        assert(!server_ret.value().is_full);

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);

        log_info("\t[0].symbol: %s\n",              GET_VECT_TICK_MINI(server_ret.value())[0].symbol.c_str());
        log_info("\t[0].openPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[0].openPrice);
        log_info("\t[0].highPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[0].highPrice);
        log_info("\t[0].lowPrice: %.10f\n",         GET_VECT_TICK_MINI(server_ret.value())[0].lowPrice);
        log_info("\t[0].lastPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[0].lastPrice);
        log_info("\t[0].volume: %.10f\n",           GET_VECT_TICK_MINI(server_ret.value())[0].volume);
        log_info("\t[0].quoteVolume: %.10f\n",      GET_VECT_TICK_MINI(server_ret.value())[0].quoteVolume);
        log_info("\t[0].openTime: %ld\n",           GET_VECT_TICK_MINI(server_ret.value())[0].openTime);
        log_info("\t[0].closeTime: %ld\n",          GET_VECT_TICK_MINI(server_ret.value())[0].closeTime);
        log_info("\t[0].firstId: %ld\n",            GET_VECT_TICK_MINI(server_ret.value())[0].firstId);
        log_info("\t[0].lastId: %d\n",              GET_VECT_TICK_MINI(server_ret.value())[0].lastId);
        log_info("\t[0].count: %d\n",               GET_VECT_TICK_MINI(server_ret.value())[0].count);

        log_info("\t[1].symbol: %s\n",              GET_VECT_TICK_MINI(server_ret.value())[1].symbol.c_str());
        log_info("\t[1].openPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[1].openPrice);
        log_info("\t[1].highPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[1].highPrice);
        log_info("\t[1].lowPrice: %.10f\n",         GET_VECT_TICK_MINI(server_ret.value())[1].lowPrice);
        log_info("\t[1].lastPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[1].lastPrice);
        log_info("\t[1].volume: %.10f\n",           GET_VECT_TICK_MINI(server_ret.value())[1].volume);
        log_info("\t[1].quoteVolume: %.10f\n",      GET_VECT_TICK_MINI(server_ret.value())[1].quoteVolume);
        log_info("\t[1].openTime: %ld\n",           GET_VECT_TICK_MINI(server_ret.value())[1].openTime);
        log_info("\t[1].closeTime: %ld\n",          GET_VECT_TICK_MINI(server_ret.value())[1].closeTime);
        log_info("\t[1].firstId: %ld\n",            GET_VECT_TICK_MINI(server_ret.value())[1].firstId);
        log_info("\t[1].lastId: %d\n",              GET_VECT_TICK_MINI(server_ret.value())[1].lastId);
        log_info("\t[1].count: %d\n",               GET_VECT_TICK_MINI(server_ret.value())[1].count);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s tickerTradingDay_ret_t (single_symbol:FULL) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(tickerTradingDay);
            std::optional<cuwacunu::camahjucunu::exchange::tickerTradingDay_ret_t> server_ret = 
            exchange_mech.ticker_tradingDay(cuwacunu::camahjucunu::exchange::tickerTradingDay_args_t(
                "BTCTUSD", 
                cuwacunu::camahjucunu::exchange::ticker_type_e::FULL
            ), true);
        PRINT_TOCK_ns(tickerTradingDay);
        assert(server_ret.has_value());
        assert(server_ret.value().is_full);

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);

        log_info("\t.symbol: %s\n",                 GET_TICK_FULL(server_ret.value()).symbol.c_str());
        log_info("\t.priceChange: %.10f\n",         GET_TICK_FULL(server_ret.value()).priceChange);
        log_info("\t.priceChangePercent: %.10f\n",  GET_TICK_FULL(server_ret.value()).priceChangePercent);
        log_info("\t.weightedAvgPrice: %.10f\n",    GET_TICK_FULL(server_ret.value()).weightedAvgPrice);
        log_info("\t.openPrice: %.10f\n",           GET_TICK_FULL(server_ret.value()).openPrice);
        log_info("\t.highPrice: %.10f\n",           GET_TICK_FULL(server_ret.value()).highPrice);
        log_info("\t.lowPrice: %.10f\n",            GET_TICK_FULL(server_ret.value()).lowPrice);
        log_info("\t.lastPrice: %.10f\n",           GET_TICK_FULL(server_ret.value()).lastPrice);
        log_info("\t.volume: %.10f\n",              GET_TICK_FULL(server_ret.value()).volume);
        log_info("\t.quoteVolume: %.10f\n",         GET_TICK_FULL(server_ret.value()).quoteVolume);
        log_info("\t.openTime: %ld\n",              GET_TICK_FULL(server_ret.value()).openTime);
        log_info("\t.closeTime: %ld\n",             GET_TICK_FULL(server_ret.value()).closeTime);
        log_info("\t.firstId: %ld\n",               GET_TICK_FULL(server_ret.value()).firstId);
        log_info("\t.lastId: %d\n",                 GET_TICK_FULL(server_ret.value()).lastId);
        log_info("\t.count: %d\n",                  GET_TICK_FULL(server_ret.value()).count);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s tickerTradingDay_ret_t (multiple_symbol:FULL) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(tickerTradingDay);
        std::optional<cuwacunu::camahjucunu::exchange::tickerTradingDay_ret_t> server_ret = 
            exchange_mech.ticker_tradingDay(cuwacunu::camahjucunu::exchange::tickerTradingDay_args_t(
                std::vector<std::string>({"BTCTUSD", "BNBBTC"}),
                cuwacunu::camahjucunu::exchange::ticker_type_e::FULL
            ), true);
        PRINT_TOCK_ns(tickerTradingDay);
        assert(server_ret.has_value());
        assert(server_ret.value().is_full);

        log_info("\t.frame_id: %s\n",                 server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",              server_ret.value().frame_rsp.http_status);

        log_info("\t[0].symbol: %s\n",                GET_VECT_TICK_FULL(server_ret.value())[0].symbol.c_str());
        log_info("\t[0].priceChange: %.10f\n",        GET_VECT_TICK_FULL(server_ret.value())[0].priceChange);
        log_info("\t[0].priceChangePercent: %.10f\n", GET_VECT_TICK_FULL(server_ret.value())[0].priceChangePercent);
        log_info("\t[0].weightedAvgPrice: %.10f\n",   GET_VECT_TICK_FULL(server_ret.value())[0].weightedAvgPrice);
        log_info("\t[0].openPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[0].openPrice);
        log_info("\t[0].highPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[0].highPrice);
        log_info("\t[0].lowPrice: %.10f\n",           GET_VECT_TICK_FULL(server_ret.value())[0].lowPrice);
        log_info("\t[0].lastPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[0].lastPrice);
        log_info("\t[0].volume: %.10f\n",             GET_VECT_TICK_FULL(server_ret.value())[0].volume);
        log_info("\t[0].quoteVolume: %.10f\n",        GET_VECT_TICK_FULL(server_ret.value())[0].quoteVolume);
        log_info("\t[0].openTime: %ld\n",             GET_VECT_TICK_FULL(server_ret.value())[0].openTime);
        log_info("\t[0].closeTime: %ld\n",            GET_VECT_TICK_FULL(server_ret.value())[0].closeTime);
        log_info("\t[0].firstId: %ld\n",              GET_VECT_TICK_FULL(server_ret.value())[0].firstId);
        log_info("\t[0].lastId: %d\n",                GET_VECT_TICK_FULL(server_ret.value())[0].lastId);
        log_info("\t[0].count: %d\n",                 GET_VECT_TICK_FULL(server_ret.value())[0].count);

        log_info("\t[1].symbol: %s\n",                GET_VECT_TICK_FULL(server_ret.value())[1].symbol.c_str());
        log_info("\t[1].priceChange: %.10f\n",        GET_VECT_TICK_FULL(server_ret.value())[1].priceChange);
        log_info("\t[1].priceChangePercent: %.10f\n", GET_VECT_TICK_FULL(server_ret.value())[1].priceChangePercent);
        log_info("\t[1].weightedAvgPrice: %.10f\n",   GET_VECT_TICK_FULL(server_ret.value())[1].weightedAvgPrice);
        log_info("\t[1].openPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[1].openPrice);
        log_info("\t[1].highPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[1].highPrice);
        log_info("\t[1].lowPrice: %.10f\n",           GET_VECT_TICK_FULL(server_ret.value())[1].lowPrice);
        log_info("\t[1].lastPrice: %.10f\n",          GET_VECT_TICK_FULL(server_ret.value())[1].lastPrice);
        log_info("\t[1].volume: %.10f\n",             GET_VECT_TICK_FULL(server_ret.value())[1].volume);
        log_info("\t[1].quoteVolume: %.10f\n",        GET_VECT_TICK_FULL(server_ret.value())[1].quoteVolume);
        log_info("\t[1].openTime: %ld\n",             GET_VECT_TICK_FULL(server_ret.value())[1].openTime);
        log_info("\t[1].closeTime: %ld\n",            GET_VECT_TICK_FULL(server_ret.value())[1].closeTime);
        log_info("\t[1].firstId: %ld\n",              GET_VECT_TICK_FULL(server_ret.value())[1].firstId);
        log_info("\t[1].lastId: %d\n",                GET_VECT_TICK_FULL(server_ret.value())[1].lastId);
        log_info("\t[1].count: %d\n",                 GET_VECT_TICK_FULL(server_ret.value())[1].count);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s tickerTradingDay_ret_t (single_symbol:MINI) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(tickerTradingDay);
            std::optional<cuwacunu::camahjucunu::exchange::tickerTradingDay_ret_t> server_ret = 
            exchange_mech.ticker_tradingDay(cuwacunu::camahjucunu::exchange::tickerTradingDay_args_t(
                "BTCTUSD", 
                cuwacunu::camahjucunu::exchange::ticker_type_e::MINI
            ), true);
        PRINT_TOCK_ns(tickerTradingDay);
        assert(server_ret.has_value());
        assert(!server_ret.value().is_full);

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);

        log_info("\t.symbol: %s\n",                 GET_TICK_MINI(server_ret.value()).symbol.c_str());
        log_info("\t.openPrice: %.10f\n",           GET_TICK_MINI(server_ret.value()).openPrice);
        log_info("\t.highPrice: %.10f\n",           GET_TICK_MINI(server_ret.value()).highPrice);
        log_info("\t.lowPrice: %.10f\n",            GET_TICK_MINI(server_ret.value()).lowPrice);
        log_info("\t.lastPrice: %.10f\n",           GET_TICK_MINI(server_ret.value()).lastPrice);
        log_info("\t.volume: %.10f\n",              GET_TICK_MINI(server_ret.value()).volume);
        log_info("\t.quoteVolume: %.10f\n",         GET_TICK_MINI(server_ret.value()).quoteVolume);
        log_info("\t.openTime: %ld\n",              GET_TICK_MINI(server_ret.value()).openTime);
        log_info("\t.closeTime: %ld\n",             GET_TICK_MINI(server_ret.value()).closeTime);
        log_info("\t.firstId: %ld\n",               GET_TICK_MINI(server_ret.value()).firstId);
        log_info("\t.lastId: %d\n",                 GET_TICK_MINI(server_ret.value()).lastId);
        log_info("\t.count: %d\n",                  GET_TICK_MINI(server_ret.value()).count);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s tickerTradingDay_ret_t (multiple_symbol:MINI) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(tickerTradingDay);
        std::optional<cuwacunu::camahjucunu::exchange::tickerTradingDay_ret_t> server_ret = 
            exchange_mech.ticker_tradingDay(cuwacunu::camahjucunu::exchange::tickerTradingDay_args_t(
                std::vector<std::string>({"BTCTUSD", "BNBBTC"}),
                cuwacunu::camahjucunu::exchange::ticker_type_e::MINI
            ), true);
        PRINT_TOCK_ns(tickerTradingDay);
        assert(server_ret.has_value());
        assert(!server_ret.value().is_full);

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);

        log_info("\t[0].symbol: %s\n",              GET_VECT_TICK_MINI(server_ret.value())[0].symbol.c_str());
        log_info("\t[0].openPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[0].openPrice);
        log_info("\t[0].highPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[0].highPrice);
        log_info("\t[0].lowPrice: %.10f\n",         GET_VECT_TICK_MINI(server_ret.value())[0].lowPrice);
        log_info("\t[0].lastPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[0].lastPrice);
        log_info("\t[0].volume: %.10f\n",           GET_VECT_TICK_MINI(server_ret.value())[0].volume);
        log_info("\t[0].quoteVolume: %.10f\n",      GET_VECT_TICK_MINI(server_ret.value())[0].quoteVolume);
        log_info("\t[0].openTime: %ld\n",           GET_VECT_TICK_MINI(server_ret.value())[0].openTime);
        log_info("\t[0].closeTime: %ld\n",          GET_VECT_TICK_MINI(server_ret.value())[0].closeTime);
        log_info("\t[0].firstId: %ld\n",            GET_VECT_TICK_MINI(server_ret.value())[0].firstId);
        log_info("\t[0].lastId: %d\n",              GET_VECT_TICK_MINI(server_ret.value())[0].lastId);
        log_info("\t[0].count: %d\n",               GET_VECT_TICK_MINI(server_ret.value())[0].count);

        log_info("\t[1].symbol: %s\n",              GET_VECT_TICK_MINI(server_ret.value())[1].symbol.c_str());
        log_info("\t[1].openPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[1].openPrice);
        log_info("\t[1].highPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[1].highPrice);
        log_info("\t[1].lowPrice: %.10f\n",         GET_VECT_TICK_MINI(server_ret.value())[1].lowPrice);
        log_info("\t[1].lastPrice: %.10f\n",        GET_VECT_TICK_MINI(server_ret.value())[1].lastPrice);
        log_info("\t[1].volume: %.10f\n",           GET_VECT_TICK_MINI(server_ret.value())[1].volume);
        log_info("\t[1].quoteVolume: %.10f\n",      GET_VECT_TICK_MINI(server_ret.value())[1].quoteVolume);
        log_info("\t[1].openTime: %ld\n",           GET_VECT_TICK_MINI(server_ret.value())[1].openTime);
        log_info("\t[1].closeTime: %ld\n",          GET_VECT_TICK_MINI(server_ret.value())[1].closeTime);
        log_info("\t[1].firstId: %ld\n",            GET_VECT_TICK_MINI(server_ret.value())[1].firstId);
        log_info("\t[1].lastId: %d\n",              GET_VECT_TICK_MINI(server_ret.value())[1].lastId);
        log_info("\t[1].count: %d\n",               GET_VECT_TICK_MINI(server_ret.value())[1].count);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s tickerPrice_ret_t (single_symbol) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(tickerPrice);
        std::optional<cuwacunu::camahjucunu::exchange::tickerPrice_ret_t> server_ret = 
            exchange_mech.tickerPrice(cuwacunu::camahjucunu::exchange::tickerPrice_args_t(
                std::string("BTCTUSD")
            ), true);
        PRINT_TOCK_ns(tickerPrice);

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);

        log_info("\t.prices.symbol: %s\n",          GET_OBJECT(server_ret.value().prices, cuwacunu::camahjucunu::exchange::price_t).symbol.c_str());
        log_info("\t.prices.price: %.10f\n",        GET_OBJECT(server_ret.value().prices, cuwacunu::camahjucunu::exchange::price_t).price);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s tickerPrice_ret_t (multiple_symbol) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(tickerPrice);
        std::optional<cuwacunu::camahjucunu::exchange::tickerPrice_ret_t> server_ret = 
            exchange_mech.tickerPrice(cuwacunu::camahjucunu::exchange::tickerPrice_args_t(
                std::vector<std::string>({"BTCTUSD", "BNBBTC"})
            ), true);
        PRINT_TOCK_ns(tickerPrice);

        log_info("\t.frame_id: %s\n",               server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",            server_ret.value().frame_rsp.http_status);

        log_info("\t.prices[0].symbol: %s\n",       GET_VECT_OBJECT(server_ret.value().prices, cuwacunu::camahjucunu::exchange::price_t)[0].symbol.c_str());
        log_info("\t.prices[0].price: %.10f\n",     GET_VECT_OBJECT(server_ret.value().prices, cuwacunu::camahjucunu::exchange::price_t)[0].price);
        
        log_info("\t.prices[1].symbol: %s\n",       GET_VECT_OBJECT(server_ret.value().prices, cuwacunu::camahjucunu::exchange::price_t)[1].symbol.c_str());
        log_info("\t.prices[1].price: %.10f\n",     GET_VECT_OBJECT(server_ret.value().prices, cuwacunu::camahjucunu::exchange::price_t)[1].price);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s tickerBook_ret_t (single_symbol) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(tickerBook);
        std::optional<cuwacunu::camahjucunu::exchange::tickerBook_ret_t> server_ret = 
            exchange_mech.tickerBook(cuwacunu::camahjucunu::exchange::tickerBook_args_t(
                std::string("BTCTUSD")
            ), true);
        PRINT_TOCK_ns(tickerBook);

        log_info("\t.frame_id: %s\n",                   server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",                server_ret.value().frame_rsp.http_status);

        log_info("\t.bookPrices.symbol: %s\n",          GET_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t).symbol.c_str());
        log_info("\t.bookPrices.bidPrice: %.10f\n",     GET_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t).bidPrice);
        log_info("\t.bookPrices.bidQty: %.10f\n",       GET_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t).bidQty);
        log_info("\t.bookPrices.askPrice: %.10f\n",     GET_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t).askPrice);
        log_info("\t.bookPrices.askQty: %.10f\n",       GET_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t).askQty);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s tickerBook_ret_t (multiple_symbol) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(tickerBook);
        std::optional<cuwacunu::camahjucunu::exchange::tickerBook_ret_t> server_ret = 
            exchange_mech.tickerBook(cuwacunu::camahjucunu::exchange::tickerBook_args_t(
                std::vector<std::string>({"BTCTUSD", "BNBBTC"})
            ), true);
        PRINT_TOCK_ns(tickerBook);

        log_info("\t.frame_id: %s\n",                   server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",                server_ret.value().frame_rsp.http_status);

        log_info("\t.bookPrices[0].symbol: %s\n",       GET_VECT_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t)[0].symbol.c_str());
        log_info("\t.bookPrices[0].bidPrice: %.10f\n",  GET_VECT_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t)[0].bidPrice);
        log_info("\t.bookPrices[0].bidQty: %.10f\n",    GET_VECT_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t)[0].bidQty);
        log_info("\t.bookPrices[0].askPrice: %.10f\n",  GET_VECT_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t)[0].askPrice);
        log_info("\t.bookPrices[0].askQty: %.10f\n",    GET_VECT_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t)[0].askQty);

        log_info("\t.bookPrices[1].symbol: %s\n",       GET_VECT_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t)[1].symbol.c_str());
        log_info("\t.bookPrices[1].bidPrice: %.10f\n",  GET_VECT_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t)[1].bidPrice);
        log_info("\t.bookPrices[1].bidQty: %.10f\n",    GET_VECT_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t)[1].bidQty);
        log_info("\t.bookPrices[1].askPrice: %.10f\n",  GET_VECT_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t)[1].askPrice);
        log_info("\t.bookPrices[1].askQty: %.10f\n",    GET_VECT_OBJECT(server_ret.value().bookPrices, cuwacunu::camahjucunu::exchange::bookPrice_t)[1].askQty);
    }



    return 0;
}