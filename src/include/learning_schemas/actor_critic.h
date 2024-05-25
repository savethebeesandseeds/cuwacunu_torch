#pragma once
#include "dtypes.h"
#include "dutils.h"
#include "actor.h"
#include "critic.h"
#include "simulated_market_enviroment.h"
#include "torch_compat/lambda_lr_scheduler.h"

#define N_STEP_TD 10 // steps look ahead in n-step Temporal Difference
#define TD_GAMMA 0.99 // Temporal difference GAMMA
#define UPDATE_BLOCK_SIZE 50 // size of the learning batch

#define TD_LAMBDA 0.95 // Generalized Advantage Estimation (GAE) Lambda
#define PPO_EPSILON 0.3 // Proximal Policy Optimization clipping Epsilon
#define ENTROPY_ALPHA 0.01 // Scale of the Entropy Bonus; Increse to promote exploration

... make it inherit from abstract
namespace cuwacunu {
namespace learning_schemas {
class ActorCriticSchema {
private:
  std::unique_ptr<cuwacunu::ActorModel> actor;
  std::unique_ptr<cuwacunu::CriticModel> critic;
  cuwacunu::Environment& environment_;
  std::unique_ptr<torch::optim::Optimizer> actor_optimizer;
  std::unique_ptr<torch::optim::Optimizer> critic_optimizer;
  std::unique_ptr<torch::optim::LambdaLR> lr_scheduler_actor;
  std::unique_ptr<torch::optim::LambdaLR> lr_scheduler_critic;
public:
  ActorCriticSchema(cuwacunu::Environment& environment);
  void learn(int episodes);

private:
  cuwacunu::episode_experience_space_t playEpisode();
  /* Method to update actor and critic models based on the episodic experience */
  void updateModels(episode_experience_space_t& episodeBuff);
};
} /* namespace learning_schemas */
} /* namespace cuwacunu */
