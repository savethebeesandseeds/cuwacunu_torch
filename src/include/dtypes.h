#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <cmath>
#include <torch/torch.h>

namespace cuwacunu {

torch::Device kDevice = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;

typedef enum instrument {
    CONST = 0,
    SINE = 1,
    COUNT_INSTSRUMENTS
} instrument_t;
static const std::string const CURRENCY_STRING[] = {
	std::string("CONST"),
	std::string("SINE")
};
static const std::vector<torch::Tensor> CURRENCY_TOKENIZER = {
    torch::tensor({1, 0}, torch::dtype(torch::kInt32).device(cuwacunu::kDevice)), // CONST
    torch::tensor({0, 1}, torch::dtype(torch::kInt32).device(cuwacunu::kDevice))  // SINE
};
#define ABSOLUTE_BASE_CURRENCY CONST

struct currency_space_t {
    instrument_t symb;        // Currency identifier
    torch::Tensor token;      // Tokenization of currency
    torch::Tensor value;      // Value in ABSOLUTE_BASE_CURRENCY
    torch::Tensor diff;       // First diff of value
    currency_space_t(instrument_t symb, double value, double diff = 0) // default constructor
    : symb(symb), token(CURRENCY_TOKENIZER[static_cast<int>(symb)]),
      value(torch::tensor({value}, torch::dtype(torch::kFloat32).device(cuwacunu::kDevice))),
      diff(torch::tensor({diff}, torch::dtype(torch::kFloat32).device(cuwacunu::kDevice))) {}
};
struct position_space_t {
    currency_space_t base_currency;
    double amount;            // Quantity held of the currency
    position_space_t(instrument_t symb, double value, double diff = 0, double amount = 0) // default constructor
    : base_currency(symb, value, diff), amount(amount) {}
};
struct wallet_space_t {
    std::vector<position_space_t> portafolio;   //  |   stakes in all aviable instruments
    double total_value;                         //  |   sum over all positions amount converted to ABSOLUTE_BASE_CURRENCY
};
struct action_space_t {
    torch::Tensor confidence; // Confidence that an order will close
    torch::Tensor threshold;  // Activation value to close the order (in base_currency)
    torch::Tensor urgency;    // Importance of closing the order
    torch::Tensor delta;      // (negative) sell, (positive) buy, amount of the executed transaction
    torch::Tensor target;     // tokenizer vector encoding where to redistribute the value once the order is closed e.g. [1.0, 1.0, 0.0]
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
struct state_space_t {
    mechanic_order_t upper;        // upper bound
    mechanic_order_t lower;        // lower bound
    wallet_t wallet;
};
struct experience_t {
    state_space_t state;
    action_space_t act;
    double reward;
    state_space_t next_state;
    bool done;
};
struct experienceBatch_t {
    std::vector<state_space_t> states;
    std::vector<action_space_t> actions;
    torch::Tensor rewards;
    std::vector<state_space_t> next_states;
    torch::Tensor dones;
};
}