#include "dutils.h"
#include "simulated_market_enviroment.h"
#include "actor_critic.h"

int main() {
    log_info("Starting...\n");

    auto market_env = cuwacunu::Environment();
    auto rl_schema = cuwacunu::ActorCriticSchema(market_env);

    rl_schema.learn(1);

    log_info("Finishing...\n");

    return 0;
}
