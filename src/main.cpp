#include "piaabo/dutils.h"
#include "simulated_market_enviroment.h"
#include "actor_critic.h"

using namespace cuwacunu;

// std::cin.clear(); this might help with the libtorch initial error message 

int main() {
    update_config();
    
    log_info("Starting...\n");
    (*environment_config)["ACTIVE_SYMBOLS"]
    update_config();


    auto market_env = cuwacunu::Environment();
    auto rl_schema = cuwacunu::ActorCriticSchema(market_env);

    rl_schema.learn(1);

    log_info("Finishing...\n");

    return 0;
}
