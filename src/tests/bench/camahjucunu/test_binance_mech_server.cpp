#include <cassert>
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/exchange/binance/binance_mech_server.h"

int main() {
    /* read the configuration */
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::iitepi::config_space_t::change_config_file(config_folder);
    cuwacunu::iitepi::config_space_t::update_config();

    /* intialize the mech */
    auto exchange_mech = cuwacunu::camahjucunu::exchange::mech::binance::binance_mech_server_t();

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

    return 0;
}