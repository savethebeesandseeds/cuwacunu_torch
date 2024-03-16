#pragma once
/* settle up for multiple continious actions */
#include "../dtypes.h"

#define N_STEP_TD 10 // steps look ahead in n-step Temporal Difference
#define TD_GAMMA 0.99 // Temporal difference GAMMA
#define UPDATE_BLOCK_SIZE 50 // size of the learning batch

#define TD_LAMBDA 0.95 // Generalized Advantage Estimation (GAE) Lambda
#define PPO_EPSILON 0.3 // Proximal Policy Optimization clipping Epsilon
#define ENTROPY_ALPHA 0.01 // Scale of the Entropy Bonus

namespace cuwacunu {
class ActorCriticSchema {
private:
  std::shared_ptr<cuwacunu::ActorModel> actor;
  std::shared_ptr<cuwacunu::CriticModel> critic;
  std::shared_ptr<cuwacunu::Environment> environment_;
  std::shared_ptr<torch::optim::Optimizer> optimizer_actor;
  std::shared_ptr<torch::optim::Optimizer> optimizer_critic;
  std::shared_ptr<torch::optim::lr_scheduler::LambdaLR> lr_scheduler_actor;
  std::shared_ptr<torch::optim::lr_scheduler::LambdaLR> lr_scheduler_critic;
public:
  ActorCriticSchema(
    std::shared_ptr<cuwacunu::Environment> environment)
  : environment_(std::move(environment)) {
    // Create the Networks
    actor = std::make_shared<cuwacunu::ActorModel>(environment_->state_size(), environment_->action_dim());
    critic = std::make_shared<cuwacunu::CriticModel>(environment_->state_size());

    // Initialize optimizers
    optimizer_actor = std::make_shared<torch::optim::Adam>(actor->parameters(), torch::optim::AdamOptions(1e-4));
    optimizer_critic = std::make_shared<torch::optim::Adam>(critic->parameters(), torch::optim::AdamOptions(1e-3));

    // Initialize learning rate schedulers
    lr_scheduler_actor = std::make_shared<torch::optim::LambdaLR>(
      optimizer_actor.get(),
      [](int epoch) { return 1.0; }
    );
    lr_scheduler_critic = std::make_shared<torch::optim::LambdaLR>(
      optimizer_critic.get(),
      [](int epoch) { return 1.0; }
    );

    // Ensure the model is on the correct device
    actor->to(cuwacunu::kDevice);
    critic->to(cuwacunu::kDevice);

    // remmber actor->eval(); critic->eval();
  }


  // ... entropy for cathegotical head
  // ... be careful when using experience replay
  // ... remember the leason about memory layers and experience replay buffer
  // ... remember to go from network.train() to network.eval()
  void train(int episodes) {
    episode_experience_t episodeBuff;

    for (int episode = 0; episode < episodes; ++episode) {
      episodeBuff = playEpisode();
      updateModels(episodeBuff);
      analizePerfomance(episodeBuff);
    }
  }

private:
  void analizePerfomance (episode_experience_t& episodeBuff) {
    ...
  }
  cuwacunu::episode_experience_t playEpisode() {
    cuwacunu::state_space_t state;
    cuwacunu::action_space_t action;
    cuwacunu::experience_t exp;
    cuwacunu::episode_experience_t episodeBuff;

    // Switch to eval mode for consistent action selection behavior
    actor.eval();
    critic.eval();
    
    // Reset enviroment and networks
    state = environment_->reset();
    critic->reset_memory();
    actor->reset_memory();
    episodeBuff.clear();

    do {
      action = actor->selectAction(state, true); /* selectAction with exploration enabled */
      exp = environment_->step(action);
      episodeBuff.push_back(exp);
      state = exp.next_state;
    } while (!exp.done);
    
    return episodeBuff;
  }
  // Method to update actor and critic models based on the episodic experience
  void updateModels(episode_experience_t& episodeBuff) {
    /*
     * (1.) Critic Update:
     *
     * Executes the n-step Temporal Difference (TD) update for the critic, The update involves:
     * 
     * - Computing n-step TD targets by considering a sequence of future rewards, 
     *    thereby incorporating delayed rewards into the value estimation. 
     * 
     * - Calculating the loss as the Mean Squared Error (MSE) between the critic's 
     *    current value estimates and the n-step TD targets. 
     * 
     * - Applying gradient descent based on the calculated loss to update the critic's parameters. 
     *
     * ------
     * #FIXME implement TD(λ) eligibility traces, instead of n-step TD.
     */
    /* Ensure the model is in training mode */
    critic.train();
    critic.reset_memory();
    
    /* critic update - part 1, pre-compute the n-step TD targets */
    std::vector<torch::Tensor> n_step_targets;
    /* calculate n-step returns and targets for each experience */
    for (size_t i = 0; i < episodeBuff.size(); ++i) {
      float gamma_pow = 1.0; /* To keep track of gamma^step */
      float n_step_return = 0.0;
      bool episodeEnded = false;
      /* Unified loop to accumulate rewards and check for episode end */
      for (int step = 0; step < N_STEP_TD && (i + step) < episodeBuff.size(); ++step) {
        const auto& future_exp = episodeBuff[i + step];
        /* Accumulate discounted reward */
        n_step_return += gamma_pow * future_exp.reward.item<float>();
        /* Update the discount factor for the next step */
        gamma_pow *= TD_GAMMA;
        if (future_exp.done) {
          /* Mark that the episode ends within this n-step window */
          episodeEnded = true;
          break;
        }
      }

      /* If the episode hasn't ended within this n-step lookahead, add the estimated value of the state at n steps ahead */
      if (!episodeEnded && (i + N_STEP_TD) < episodeBuff.size()) {
        const auto& future_exp = episodeBuff[i + N_STEP_TD];
        auto next_state_value = critic->forward(future_exp.state.unsqueeze(0)).detach();
        n_step_return += gamma_pow * next_state_value.item<float>(); // Add the estimated future state value
      }

      /* Store the n-step return as a tensor */
      n_step_targets.push_back(torch::tensor({n_step_return}, torch::kType).to(cuwacunu::kDevice));
    }

    /* critic update - part 2, updates based on the pre-calculated n-step targets */
    torch::Tensor critic_loss = torch::zeros({1}, torch::kType).to(cuwacunu::kDevice);
    int count = 0;

    for (size_t t = 0; t < episodeBuff.size(); ++t) {
      auto& exp = episodeBuff[t];
      auto current_value = critic->forward(exp.state.unsqueeze(0)); /* Current state value */
      auto target_value = n_step_targets[t]; /* Pre-calculated n-step target */

      /* Accumulate critic loss */
      critic_loss += torch::mse_loss(current_value, target_value);

      /* update every UPDATE_BLOCK_SIZE or in the last */
      
      if (++count % UPDATE_BLOCK_SIZE == 0 || t == episodeBuff.size() - 1) {
        /* Normalize and update after a block of experiences or at the end */
        critic_loss /= static_cast<float>(count);

        /* backpropagate train */
        critic_optimizer.zero_grad();
        critic_loss.backward();
        critic_optimizer.step();
        
        /* Reset for next update block */
        critic_loss = torch::zeros({1}, torch::kType).to(cuwacunu::kDevice);
        count = 0; /* Reset count for next block */
      }
    }

    /*
     * (2.) Actor Update:
     *
     * Enhances the actor's policy using the Proximal Policy Optimization (PPO) algorithm, which involves:
     *
     * - Utilizing the Generalized Advantage Estimation (GAE) to compute advantage scores, 
     *    offering a trade-off between variance and bias in the estimation of policy gradients.
     * 
     * - Calculating the PPO loss by comparing the probability ratio of the current and previous policies,
     *    applying clipping to maintain the updates within a predefined range, hence preventing 
     *    destabilizing large policy updates.
     * 
     * - Incorporating an entropy bonus to the loss, promoting exploration by penalizing overly 
     *    deterministic policies, ensuring a comprehensive exploration of the action space.
     *
     * - Normalizing the actor's loss over a batch of experiences to stabilize the learning process,
     *    and employing gradient descent to update the actor's policy parameters, steering the policy
     *    towards higher expected rewards.
     *
     * ------
     * #TODO Normalize the advantage
     * #TODO Evaluate the impact of different GAE lambda values on the trade-off between bias and variance.
     * #TODO Consider adaptive strategies for the entropy coefficient to balance exploration and exploitation throughout training.
     */
    torch::Tensor actor_loss = torch::zeros({1}, torch::kType).to(cuwacunu::kDevice);
    // Ensure the models are in the expected mode
    critic.eval();
    critic.reset_memory();
    actor.train();
    actor.reset_memory();

    // compute the Generalized Advantage Estimation (GAE)
    torch::Tensor advantages = torch::zeros({episodeBuff.size()}, torch::kType).to(cuwacunu::kDevice);
    torch::Tensor gae = torch::zeros({1}, torch::kType).to(cuwacunu::kDevice);
    
    for (int t = episodeBuff.size() - 1; t >= 0; --t) {
      auto& exp = episodeBuff[t];
      auto current_value = critic->forward(exp.state.unsqueeze(0)).item<float>();
      auto next_value = critic->forward(exp.next_state.unsqueeze(0));
      float delta = exp.reward + TD_GAMMA * next_value.item<float>() * (1 - exp.done.item<float>()) - current_value;
      gae = delta + TD_GAMMA * TD_LAMBDA * gae * (1 - exp.done.item<float>());
      advantages[t] = gae + current_value; // You may need to adjust this based on how you calculate or store values
    }

    /* lambda function, utility to compute the PPO update target */
    auto computePPOLoss = [](const torch::Tensor& oldLogProb, const torch::Tensor& newLogProb, const torch::Tensor& advantage) -> torch::Tensor {
      auto ratio = torch::exp(newLogProb - oldLogProb);
      auto obj_clip = torch::clamp(ratio, 1.0f - PPO_EPSILON, 1.0f + PPO_EPSILON) * advantage;
      auto obj = torch::min(ratio * advantage, obj_clip);
      
      return -obj.mean();
    };

    torch::Tensor actor_loss = torch::zeros({1}, torch::kType).to(cuwacunu::kDevice);
    count = 0;

    // Loop over episode
    int index = 0; // To access the corresponding advantage for each experience
    for (const auto& exp : episodeBuff) {
      auto state = exp.state.unsqueeze(0);
      auto advantage = advantages[index];

      // Calculate current policy's log probability for the taken action taken under the previous policy
      auto oldLogits = exp.action.logits;
      
      // Calculate current policy's log probability for the taken action taken under the current policy
      auto newLogits = actor->forward(state); // Output for categorical action taken spaces
      
      /* Categorical: base_symb */
      {
        actor_loss += computePPOLoss(
        /* oldLogProb   */ oldLogits.base_symb_dist().log_prob(exp.action.base_symb),
        /* newLogProb   */ newLogits.base_symb_dist().log_prob(exp.action.base_symb),
        /* advantage    */ advantage);
      }
      /* Categorical: target_symb */
      {
        actor_loss += computePPOLoss(
        /* oldLogProb   */ oldLogits.target_symb_dist().log_prob(exp.action.target_symb),
        /* newLogProb   */ newLogits.target_symb_dist().log_prob(exp.action.target_symb),
        /* advantage    */ advantage);
      }
      /* Continious: confidence */
      {
        actor_loss += computePPOLoss(
          /* oldLogProb  */ oldLogits.confidence_dist().log_prob(exp.action.confidence),
          /* newLogProb  */ newLogits.confidence_dist().log_prob(exp.action.confidence),
          /* advantage   */ advantage);
      }
      /* Continious: urgency */
      {
        actor_loss += computePPOLoss(
          /* oldLogProb  */ oldLogits.urgency_dist().log_prob(exp.action.urgency),
          /* newLogProb  */ newLogits.urgency_dist().log_prob(exp.action.urgency),
          /* advantage   */ advantage);
      }
      /* Continious: threshold */
      {
        actor_loss += computePPOLoss(
          /* oldLogProb */  oldLogits.threshold_dist().log_prob(exp.action.threshold),
          /* newLogProb */  newLogits.threshold_dist().log_prob(exp.action.threshold),
          /* advantage   */ advantage);
      }
      /* Continious: delta */
      {
        actor_loss += computePPOLoss(
          /* oldLogProb  */ oldLogits.delta_dist().log_prob(exp.action.delta),
          /* newLogProb  */ newLogits.delta_dist().log_prob(exp.action.delta),
          /* advantage   */ advantage);
      }
      /* Entropy bonus */
      actor_loss += -ENTROPY_ALPHA * (newLogits.probs() * torch::log(newLogits.probs() + 1e-9)).sum(1).mean();


      /* update the actor in batches */
      if (++count % UPDATE_BLOCK_SIZE == 0 || index == episodeBuff.size() - 1) {
        /* Normalize loss by the number of acounted experiences */
        actor_loss /= static_cast<float>(count);

        /* Update the actor */
        actor_optimizer.zero_grad();
        actor.backward();
        actor_optimizer.step();

        /* Reset for next update block */
        actor_loss = torch::zeros({1}, torch::kType).to(cuwacunu::kDevice);
        count = 0;
      }

      /* Move to the next GAE advantage */
      index++;
    }
  }
};
}
