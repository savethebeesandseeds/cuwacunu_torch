#pragma once
#include "../dtypes.h"

#define STATE_SIZE 1
#define ACTION_SIZE 1
currency_space_t;
position_space_t;
wallet_space_t;
action_space_t;
order_space_t;
mechanic_order_t;
state_space_t;
#define FOR_ALL_INSTRUMENTS(inst) for (cuwacunu::instrument_t inst = USDT; inst < COUNT; inst = static_cast<cuwacunu::instrument_t>(inst + 1))
namespace cuwacunu {
class Broker {
private:
    static double ctime;
    static std::vector<currency_space_t> currencies;
    static std::vector<std::function<double()>> delta_value_lambdas;
public:
    static class _init {public:_init(){Broker::init();}}_initializer;
    static void init() {
        Broker::ctime = 0;
        delta_value_lambdas = {
            []() -> double { return 0.0; },                                         // update delta for CONST
            []() -> double { return std::cos(2 * M_PI * (0.25) * Broker::ctime); }  // update delta for SINE diff(sin) = cos
        };
        FOR_ALL_INSTRUMENTS(inst) { Broker::currencies.push_back(currency_space_t(inst, 1.0, 0)); }
    }
    static currency_space_t retrieve_currency(instrument_t inst) {
        return Broker::currencies.at((size_t)inst);
    }
    static void step() {
        ++ctime;
        FOR_ALL_INSTRUMENTS(inst) {
            double dv = delta_value_lambdas[(int)inst]();
            Broker::currencies[(size_t)inst].diff = dv;     // replace the diff
            Broker::currencies[(size_t)inst].value += dv;   // udpate the value
        }
    }
    static double exchange(order_space_t ord) {

    }
};
Broker::_init Broker::_initializer;







struct action_space_t {
    torch::Tensor confidence; // Confidence that an order will close
    torch::Tensor threshold;  // Activation value to close the order (in base_currency)
    torch::Tensor urgency;    // Importance of closing the order
    torch::Tensor delta;      // (negative) sell, (positive) buy, amount of the executed transaction
    action_space_t(double confidence, double threshold, double urgency, double delta) // default constructor
    : confidence(torch::tensor({confidence}, torch::dtype(torch::kFloat32).device(cuwacunu::kDevice))),
      threshold(torch::tensor({threshold}, torch::dtype(torch::kFloat32).device(cuwacunu::kDevice))),
      urgency(torch::tensor({urgency}, torch::dtype(torch::kFloat32).device(cuwacunu::kDevice))),
      delta(torch::tensor({delta}, torch::dtype(torch::kFloat32).device(cuwacunu::kDevice))) {}
};
struct order_space_t {
    double close_at;            //  |   interval([0, inf])  | target settlement price (in target_currency)
    bool liquidated;            //  |   bool                | flag to indicate if the order has been fulfiled
    double profit;              //  |   interval([-inf, inf])| 0 of the order has not closed and signed reflecting the profit or losses in base currency
    currency_space_t base_currency;  //  |   currency_space_t   | the currency of the holding capital to be converted into target_currency once the order settle
    currency_space_t target_currency;//  |   currency_space_t   | the type of coin that i will convert the base_currency into once the order is settle
};
struct mechanic_order_t {
    action_space_t act;         // action space, comming from the base policy network 
    order_space_t ord;          // order space, transforms the action space into an executable instruction
};

class Environment {
public:
    wallet_space_t wallet;
    std::vector<mechanic_order_t> mech_orders;
    Environment() {
        initialize_wallet()
    }
    virtual ~Environment() {}
    void initialize_wallet() {
        ...
    }
    torch::Tensor reset() {
        return torch::zeros(STATE_SIZE);
    }
    mechanic_order_t mechanize_order_from_action(action_space_t action) {
        ... convert the action in unfulfiled order
    }
    void exchange_mechanic_orders() {
        double outcome = 0; // net value in ABSOLUTE_BASE_CURRENCY
        .. filter out of value orders
        .. close orders
        for(size_t idx=0; idx < mech_orders.size(); idx++) {
            outcome += Broker::exchange(mech_orders[idx].ord);
        }
        return outcome;
    }
    cuwacunu::experience_t step(action_space_t action) {
        Broker::step();
        mech_orders.push_back(mechanize_order_from_action(action));
        double reward = exchange_mechanic_orders();

        return torch::zeros(STATE_SIZE);
    }
};
}
