#include "dutils.h"
#include "simulated_market_enviroment.h"
#include "actor_critic.h"

int main() {
    auto market_env = cuwacunu::Environment();
    log_info("market_env.state_size: %ld\n",market_env.state_size);
    auto rl_schema = cuwacunu::ActorCriticSchema(market_env);

    log_info("waka\n");

    return 0;
}
