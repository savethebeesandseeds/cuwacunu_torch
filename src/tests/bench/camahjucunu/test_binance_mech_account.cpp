#include <cassert>
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/exchange/binance/binance_mech_account.h"

int main() {
    /* read the configuration */
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    /* authenticate user */
    cuwacunu::piaabo::dsecurity::SecureVault.authenticate();

    /* intialize the mech */
    auto exchange_mech = cuwacunu::camahjucunu::exchange::mech::binance::binance_mech_account_t();
   
    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s account_information_ret_t %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(account_information);
        std::optional<cuwacunu::camahjucunu::exchange::account_information_ret_t> server_ret = 
            exchange_mech.account_information(cuwacunu::camahjucunu::exchange::account_information_args_t(
            ), true);
        PRINT_TOCK_ns(account_information);

        log_info("\t.frame_id: %s\n",                   server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",                server_ret.value().frame_rsp.http_status);

        log_info("\t.makerCommission: %d\n",            server_ret.value().makerCommission);
        log_info("\t.takerCommission: %d\n",            server_ret.value().takerCommission);
        log_info("\t.buyerCommission: %d\n",            server_ret.value().buyerCommission);
        log_info("\t.sellerCommission: %d\n",           server_ret.value().sellerCommission);
        log_info("\t.canTrade: %d\n",                   server_ret.value().canTrade);
        log_info("\t.canWithdraw: %d\n",                server_ret.value().canWithdraw);
        log_info("\t.canDeposit: %d\n",                 server_ret.value().canDeposit);
        log_info("\t.brokered: %d\n",                   server_ret.value().brokered);
        log_info("\t.requireSelfTradePrevention: %d\n", server_ret.value().requireSelfTradePrevention);
        log_info("\t.preventSor: %d\n",                 server_ret.value().preventSor);
        log_info("\t.updateTime: %ld\n",                server_ret.value().updateTime);
        log_info("\t.uid: %ld\n",                       server_ret.value().uid);
        log_info("\t.accountType: %s\n",                cuwacunu::camahjucunu::exchange::enum_to_string(server_ret.value().accountType).c_str());
        if(server_ret.value().permissions.size() > 0) {
            log_info("\t.permissions[0]: %s\n",             cuwacunu::camahjucunu::exchange::enum_to_string(server_ret.value().permissions[0]).c_str());
        }
        log_info("\t.commissionRates.maker: %.10f\n",   server_ret.value().commissionRates.maker);
        log_info("\t.commissionRates.taker: %.10f\n",   server_ret.value().commissionRates.taker);
        log_info("\t.commissionRates.buyer: %.10f\n",   server_ret.value().commissionRates.buyer);
        log_info("\t.commissionRates.seller: %.10f\n",  server_ret.value().commissionRates.seller);
        if(server_ret.value().balances.size() > 0) {
            log_info("\t.balances[0].asset: %s\n",          server_ret.value().balances[0].asset.c_str());
            log_info("\t.balances[0].free: %.10f\n",        server_ret.value().balances[0].free);
            log_info("\t.balances[0].locked: %.10f\n",      server_ret.value().balances[0].locked);
        }
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s account_order_history_ret_t %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(account_order_history);
        std::optional<cuwacunu::camahjucunu::exchange::account_order_history_ret_t> server_ret = 
            exchange_mech.account_order_history(cuwacunu::camahjucunu::exchange::account_order_history_args_t(
                std::string("BTCTUSD")
            ), true);
        PRINT_TOCK_ns(account_order_history);

        log_info("\t.frame_id: %s\n",                       server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",                    server_ret.value().frame_rsp.http_status);

        if(server_ret.value().orders.size() > 0) {
            log_info("\t.symbol: %s\n",                     server_ret.value().orders[0].symbol.c_str()); 
            log_info("\t.orderId: %ld\n",                   server_ret.value().orders[0].orderId);
            log_info("\t.orderListId: %d\n",                server_ret.value().orders[0].orderListId);
            log_info("\t.clientOrderId: %s\n",              server_ret.value().orders[0].clientOrderId.c_str()); 
            log_info("\t.price: %.10f\n",                   server_ret.value().orders[0].price);
            log_info("\t.origQty: %.10f\n",                 server_ret.value().orders[0].origQty);
            log_info("\t.executedQty: %.10f\n",             server_ret.value().orders[0].executedQty);
            log_info("\t.cummulativeQuoteQty: %.10f\n",     server_ret.value().orders[0].cummulativeQuoteQty);
            log_info("\t.status: %s\n",                     cuwacunu::camahjucunu::exchange::enum_to_string(server_ret.value().orders[0].status).c_str());
            log_info("\t.timeInForce: %s\n",                cuwacunu::camahjucunu::exchange::enum_to_string(server_ret.value().orders[0].timeInForce).c_str());
            log_info("\t.type: %s\n",                       cuwacunu::camahjucunu::exchange::enum_to_string(server_ret.value().orders[0].type).c_str());
            log_info("\t.side: %s\n",                       cuwacunu::camahjucunu::exchange::enum_to_string(server_ret.value().orders[0].side).c_str());
            log_info("\t.selfTradePreventionMode: %s\n",    cuwacunu::camahjucunu::exchange::enum_to_string(server_ret.value().orders[0].selfTradePreventionMode).c_str());
            log_info("\t.stopPrice: %.10f\n",               server_ret.value().orders[0].stopPrice);
            log_info("\t.icebergQty: %.10f\n",              server_ret.value().orders[0].icebergQty);
            log_info("\t.time: %ld\n",                      server_ret.value().orders[0].time);
            log_info("\t.updateTime: %ld\n",                server_ret.value().orders[0].updateTime);
            log_info("\t.isWorking: %d\n",                  server_ret.value().orders[0].isWorking);
            log_info("\t.workingTime: %ld\n",               server_ret.value().orders[0].workingTime);
            log_info("\t.origQuoteOrderQty: %.10f\n",       server_ret.value().orders[0].origQuoteOrderQty);
            log_info("\t.preventedMatchId: %d\n",           server_ret.value().orders[0].preventedMatchId);
            log_info("\t.preventedQuantity: %.10f\n",       server_ret.value().orders[0].preventedQuantity);
            log_info("\t.trailingDelta: %ld\n",             server_ret.value().orders[0].trailingDelta);
            log_info("\t.trailingTime: %ld\n",              server_ret.value().orders[0].trailingTime);
            log_info("\t.strategyId: %d\n",                 server_ret.value().orders[0].strategyId);
            log_info("\t.strategyType: %d\n",               server_ret.value().orders[0].strategyType);
        }
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s account_trade_list_ret_t %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(account_trade_list);
        std::optional<cuwacunu::camahjucunu::exchange::account_trade_list_ret_t> server_ret = 
            exchange_mech.account_trade_list(cuwacunu::camahjucunu::exchange::account_trade_list_args_t(
                std::string("BTCTUSD")
            ), true);
        PRINT_TOCK_ns(account_trade_list);

        log_info("\t.frame_id: %s\n",                       server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",                    server_ret.value().frame_rsp.http_status);

        if(server_ret.value().trades.size() > 0) {
            log_info("\t.trades[0].symbol: %s\n",           server_ret.value().trades[0].symbol.c_str());
            log_info("\t.trades[0].id: %d\n",               server_ret.value().trades[0].id);
            log_info("\t.trades[0].orderId: %d\n",          server_ret.value().trades[0].orderId);
            log_info("\t.trades[0].orderListId: %d\n",      server_ret.value().trades[0].orderListId);
            log_info("\t.trades[0].price: %.10f\n",         server_ret.value().trades[0].price);
            log_info("\t.trades[0].qty: %.10f\n",           server_ret.value().trades[0].qty);
            log_info("\t.trades[0].quoteQty: %.10f\n",      server_ret.value().trades[0].quoteQty);
            log_info("\t.trades[0].commission: %.10f\n",    server_ret.value().trades[0].commission);
            log_info("\t.trades[0].commissionAsset: %s\n",  server_ret.value().trades[0].commissionAsset.c_str());
            log_info("\t.trades[0].time: %ld\n",            server_ret.value().trades[0].time);
            log_info("\t.trades[0].isBuyer: %d\n",          server_ret.value().trades[0].isBuyer);
            log_info("\t.trades[0].isMaker: %d\n",          server_ret.value().trades[0].isMaker);
            log_info("\t.trades[0].isBestMatch: %d\n",      server_ret.value().trades[0].isBestMatch);
        }
    }

    {
        log_info("--- --- --- --- --- --- --- --- --- --- --- --- --- %s account_commission_rates_ret_t %s --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
        TICK(account_commission_rates);
        std::optional<cuwacunu::camahjucunu::exchange::account_commission_rates_ret_t> server_ret = 
            exchange_mech.account_commission_rates(cuwacunu::camahjucunu::exchange::account_commission_rates_args_t(
                std::string("BTCTUSD")
            ), true);
        PRINT_TOCK_ns(account_commission_rates);

        log_info("\t.frame_id: %s\n",                   server_ret.value().frame_rsp.frame_id.c_str());
        log_info("\t.http_status: %d\n",                server_ret.value().frame_rsp.http_status);
        
        log_info("\t.commissionDesc.symbol: %s\n",                       server_ret.value().commissionDesc.symbol.c_str());
        log_info("\t.commissionDesc.standardCommission.maker: %.10f\n",  server_ret.value().commissionDesc.standardCommission.maker);
        log_info("\t.commissionDesc.standardCommission.taker: %.10f\n",  server_ret.value().commissionDesc.standardCommission.taker);
        log_info("\t.commissionDesc.standardCommission.buyer: %.10f\n",  server_ret.value().commissionDesc.standardCommission.buyer);
        log_info("\t.commissionDesc.standardCommission.seller: %.10f\n", server_ret.value().commissionDesc.standardCommission.seller);
        log_info("\t.commissionDesc.taxCommission.maker: %.10f\n",       server_ret.value().commissionDesc.taxCommission.maker);
        log_info("\t.commissionDesc.taxCommission.taker: %.10f\n",       server_ret.value().commissionDesc.taxCommission.taker);
        log_info("\t.commissionDesc.taxCommission.buyer: %.10f\n",       server_ret.value().commissionDesc.taxCommission.buyer);
        log_info("\t.commissionDesc.taxCommission.seller: %.10f\n",      server_ret.value().commissionDesc.taxCommission.seller);
        log_info("\t.commissionDesc.discount.enabledForAccount: %d\n",   server_ret.value().commissionDesc.discount.enabledForAccount);
        log_info("\t.commissionDesc.discount.enabledForSymbol: %d\n",    server_ret.value().commissionDesc.discount.enabledForSymbol);
        log_info("\t.commissionDesc.discount.discountAsset: %s\n",       server_ret.value().commissionDesc.discount.discountAsset.c_str());
        log_info("\t.commissionDesc.discount.discount: %.10f\n",         server_ret.value().commissionDesc.discount.discount);
    }


    return 0;
}