#include <cassert>
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/binance/binance_mech_data.h"


int main() {
    /* read the configuration */
    const char* config_folder = "/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    /* intialize the mech */
    auto exchange_mech = cuwacunu::camahjucunu::exchange::mech::binance::binance_mech_data_t();

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s klines_ret_t (...) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(klines);
        std::optional<cuwacunu::camahjucunu::exchange::klines_ret_t> server_ret = 
            exchange_mech.klines(cuwacunu::camahjucunu::exchange::klines_args_t(
                "BTCTUSD",                                                      // std::string symbol
                cuwacunu::camahjucunu::exchange::interval_type_e::interval_1m,  // interval_type_e interval
                // std::optional<long> startTime
                // std::optional<long> endTime
                // std::optional<std::string> timeZone
                // std::optional<int> limit

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

    return 0;
}