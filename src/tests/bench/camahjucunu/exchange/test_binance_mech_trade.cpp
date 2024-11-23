#include <cassert>
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/binance/binance_mech_trade.h"

int main() {

    /* read the configuration */
    const char* config_folder = "/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    /* authenticate user */
    cuwacunu::piaabo::dsecurity::SecureVault.authenticate();

    /* intialize the mech */
    auto exchange_mech = cuwacunu::camahjucunu::exchange::mech::binance::binance_mech_trade_t();
    
    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s orderMarket_ret_t (test::no_await) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        /* prepare the args */
        cuwacunu::camahjucunu::exchange::orderMarket_args_t args(
                "ETHBTC", cuwacunu::camahjucunu::exchange::order_side_e::SELL, cuwacunu::camahjucunu::exchange::order_type_e::MARKET
            );
        args.quantity = 0.01;
        /* trade */
        TICK(orderMarket);
        std::optional<cuwacunu::camahjucunu::exchange::orderMarket_ret_t> server_ret = 
            exchange_mech.orderMarket(args, true, false);
        PRINT_TOCK_ns(orderMarket);

        std::this_thread::sleep_for(std::chrono::seconds(2)); /* await to retrive response */
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s orderMarket_ret_t (test::await) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        /* prepare the args */
        cuwacunu::camahjucunu::exchange::orderMarket_args_t args(
                "ETHBTC", cuwacunu::camahjucunu::exchange::order_side_e::SELL, cuwacunu::camahjucunu::exchange::order_type_e::MARKET
            );
        args.quantity = 0.01;
        args.newClientOrderId = "TEST-ORDER";
        /* trade */
        TICK(orderMarket);
        std::optional<cuwacunu::camahjucunu::exchange::orderMarket_ret_t> server_ret = 
            exchange_mech.orderMarket(args, true, true);
        PRINT_TOCK_ns(orderMarket);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s orderMarket_ret_t (no_test::await) %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        /* prepare the args */
        cuwacunu::camahjucunu::exchange::orderMarket_args_t args(
                "ETHBTC", cuwacunu::camahjucunu::exchange::order_side_e::SELL, cuwacunu::camahjucunu::exchange::order_type_e::MARKET
            );
        args.quantity = 0.01;
        args.newClientOrderId = "TEST-ORDER";
        /* trade */
        TICK(orderMarket);
        std::optional<cuwacunu::camahjucunu::exchange::orderMarket_ret_t> server_ret = 
            exchange_mech.orderMarket(args, false, true);
        PRINT_TOCK_ns(orderMarket);
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s orderStatus_ret_t %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        cuwacunu::camahjucunu::exchange::orderStatus_args_t args("ETHBTC");
        args.origClientOrderId = "TEST-ORDER";
        TICK(orderStatus);
        std::optional<cuwacunu::camahjucunu::exchange::orderStatus_ret_t> server_ret = 
            exchange_mech.orderStatus(args, true);
        PRINT_TOCK_ns(orderStatus);
    }

    return 0;
}