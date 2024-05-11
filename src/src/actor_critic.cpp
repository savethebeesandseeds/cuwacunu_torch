/* settle up for multiple continious actions */
#include "actor_critic.h"

namespace cuwacunu {
ActorCriticSchema::ActorCriticSchema(cuwacunu::Environment& environment)
  : environment_(environment) {

  /* Create the Networks */
  actor = std::make_unique<cuwacunu::ActorModel>(environment_.state_size);
  critic = std::make_unique<cuwacunu::CriticModel>(environment_.state_size);

  /* Initialize optimizers */
  actor_optimizer = std::make_unique<torch::optim::Adam>(actor->ptr()->parameters(), torch::optim::AdamOptions(1e-4));
  critic_optimizer = std::make_unique<torch::optim::Adam>(critic->ptr()->parameters(), torch::optim::AdamOptions(1e-3));

  /* Initialize learning rate schedulers */
  lr_scheduler_actor = std::make_unique<torch::optim::LambdaLR>(
    *actor_optimizer.get(),
    [](unsigned epoch) { return (double) 1.0; }
  );
  lr_scheduler_critic = std::make_unique<torch::optim::LambdaLR>(
    *critic_optimizer.get(),
    [](unsigned epoch) { return (double) 1.0; }
  );

  /* Ensure the model is on the correct device */
  actor->ptr()->to(cuwacunu::kDevice);
  critic->ptr()->to(cuwacunu::kDevice);
}

void ActorCriticSchema::learn(int episodes) {
  episode_experience_t episodeBuff;

  for (int episode = 0; episode < episodes; ++episode) {
    episodeBuff = playEpisode();
    updateModels(episodeBuff);
    // analizePerfomance(episodeBuff);
  }
}

cuwacunu::episode_experience_t ActorCriticSchema::playEpisode() {
  cuwacunu::episode_experience_t episodeBuff;
  bool done;
  /* Switch to eval mode for consistent action selection behavior */
  actor->ptr()->eval();
  critic->ptr()->eval();
  
  /* Reset enviroment and networks */
  cuwacunu::state_space_t state = environment_.reset();
  critic->ptr()->reset_memory();
  actor->ptr()->reset_memory();
  episodeBuff.clear();

  do {
    cuwacunu::action_space_t action = actor->ptr()->selectAction(state, true); /* selectAction with exploration enabled */
    cuwacunu::experience_t exp = environment_.step(action);
    episodeBuff.push_back(exp);
    state = exp.next_state;
    done = exp.done;
  } while (!done);
  
  return episodeBuff;
}
/* Method to update actor and critic models based on the episodic experience */
void ActorCriticSchema::updateModels(episode_experience_t& episodeBuff) {
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
  critic->ptr()->train();
  critic->ptr()->reset_memory();
  
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
      n_step_return += gamma_pow * future_exp.reward.evaluate_reward();
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
      auto next_state_value = critic->ptr()->forward(future_exp.state.unpack().unsqueeze(0)).detach();
      n_step_return += gamma_pow * next_state_value.item().toFloat(); // Add the estimated future state value
    }

    /* Store the n-step return as a tensor */
    n_step_targets.push_back(torch::tensor({n_step_return}, cuwacunu::kType).to(cuwacunu::kDevice));
  }

  /* critic update - part 2, updates based on the pre-calculated n-step targets */
  torch::Tensor critic_loss = torch::zeros({1}, cuwacunu::kType).to(cuwacunu::kDevice);
  int count = 0;

  /* loop over episode */
  for (size_t t = 0; t < episodeBuff.size(); ++t) {
    auto& exp = episodeBuff[t];
    auto current_value = critic->ptr()->forward(exp.state.unpack().unsqueeze(0)); /* Current state value */
    auto target_value = n_step_targets[t]; /* Pre-calculated n-step target */

    /* Accumulate critic loss */
    critic_loss += torch::mse_loss(current_value, target_value);

    /* update every UPDATE_BLOCK_SIZE or in the last */
    
    if (++count % UPDATE_BLOCK_SIZE == 0 || t == episodeBuff.size() - 1) {
      /* Normalize and update after a block of experiences or at the end */
      critic_loss /= static_cast<float>(count);

      /* backpropagate train */
      critic_optimizer->zero_grad();
      critic_loss.backward();
      critic_optimizer->step();
      
      /* Reset for next update block */
      critic_loss = torch::zeros({1}, cuwacunu::kType).to(cuwacunu::kDevice);
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
  torch::Tensor actor_loss = torch::zeros({1}, cuwacunu::kType).to(cuwacunu::kDevice);
  count = 0;

  /* Ensure the models are in the expected mode */
  critic->ptr()->eval();
  critic->ptr()->reset_memory();
  actor->ptr()->train();
  actor->ptr()->reset_memory();

  /* compute the Generalized Advantage Estimation (GAE) */
  torch::Tensor advantages = torch::zeros({(long int) episodeBuff.size()}, cuwacunu::kType).to(cuwacunu::kDevice);
  torch::Tensor gae = torch::zeros({1}, cuwacunu::kType).to(cuwacunu::kDevice);
  
  for (int t = episodeBuff.size() - 1; t >= 0; --t) {
    auto& exp = episodeBuff[t];
    auto current_value = critic->ptr()->forward(exp.state.unpack().unsqueeze(0)).item().toFloat();
    auto next_value = critic->ptr()->forward(exp.next_state.unpack().unsqueeze(0));
    float delta = exp.reward.evaluate_reward() + TD_GAMMA * next_value.item().toFloat() * (1 - exp.done) - current_value;
    gae = delta + TD_GAMMA * TD_LAMBDA * gae * (1 - exp.done);
    advantages[t] = gae + current_value; /* You may need to adjust this based on how you calculate or store values */
  }

  /* lambda function, utility to compute the PPO update target */
  auto computePPOLoss = [](const torch::Tensor& oldLogProb, const torch::Tensor& newLogProb, const torch::Tensor& advantage) -> torch::Tensor {
    auto ratio = torch::exp(newLogProb - oldLogProb);
    auto obj_clip = torch::clamp(ratio, 1.0f - PPO_EPSILON, 1.0f + PPO_EPSILON) * advantage;
    auto obj = torch::min(ratio * advantage, obj_clip);
    
    return -obj.mean();
  };
  
  /* Loop over episode */
  int index = 0; /* To access the corresponding advantage for each experience */
  for (const auto& exp : episodeBuff) {
    auto state = exp.state.unpack().unsqueeze(0);
    auto advantage = advantages[index];

    /* Calculate current policy's log probability for the taken action taken under the previous policy */
    auto oldLogits = exp.action.logits;
    
    /* Calculate current policy's log probability for the taken action taken under the current policy */
    auto newLogits = actor->ptr()->forward(state); /* Output for categorical action taken spaces */
    
    /* Categorical: base_symb */
    {
      actor_loss += computePPOLoss(
        /* oldLogProb   */ oldLogits.base_symb_dist().log_prob(torch::tensor({exp.action.base_symb}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* newLogProb   */ newLogits.base_symb_dist().log_prob(torch::tensor({exp.action.base_symb}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* advantage    */ advantage);
      /* Entropy bonus (base_symb) */
      actor_loss += (-ENTROPY_ALPHA * newLogits.base_symb_dist().entropy().mean()) / 6.0;
    }
    /* Categorical: target_symb */
    {
      actor_loss += computePPOLoss(
        /* oldLogProb   */ oldLogits.target_symb_dist().log_prob(torch::tensor({exp.action.target_symb}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* newLogProb   */ newLogits.target_symb_dist().log_prob(torch::tensor({exp.action.target_symb}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* advantage    */ advantage);
      /* Entropy bonus (target_symb) */
      actor_loss += (-ENTROPY_ALPHA * newLogits.target_symb_dist().entropy().mean()) / 6.0;
    }
    /* Continious: confidence */
    {
      actor_loss += computePPOLoss(
        /* oldLogProb  */ oldLogits.confidence_dist().log_prob(torch::tensor({exp.action.confidence}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* newLogProb  */ newLogits.confidence_dist().log_prob(torch::tensor({exp.action.confidence}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* advantage   */ advantage);
      /* Entropy bonus (confidence) */
      actor_loss += (-ENTROPY_ALPHA * newLogits.confidence_dist().entropy().mean()) / 6.0;
    }
    /* Continious: urgency */
    {
      actor_loss += computePPOLoss(
        /* oldLogProb  */ oldLogits.urgency_dist().log_prob(torch::tensor({exp.action.urgency}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* newLogProb  */ newLogits.urgency_dist().log_prob(torch::tensor({exp.action.urgency}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* advantage   */ advantage);
      /* Entropy bonus (urgency) */
      actor_loss += (-ENTROPY_ALPHA * newLogits.urgency_dist().entropy().mean()) / 6.0;
    }
    /* Continious: threshold */
    {
      actor_loss += computePPOLoss(
        /* oldLogProb */  oldLogits.threshold_dist().log_prob(torch::tensor({exp.action.threshold}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* newLogProb */  newLogits.threshold_dist().log_prob(torch::tensor({exp.action.threshold}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* advantage   */ advantage);
      /* Entropy bonus (threshold) */
      actor_loss += (-ENTROPY_ALPHA * newLogits.threshold_dist().entropy().mean()) / 6.0;
    }
    /* Continious: delta */
    {
      actor_loss += computePPOLoss(
        /* oldLogProb  */ oldLogits.delta_dist().log_prob(torch::tensor({exp.action.delta}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* newLogProb  */ newLogits.delta_dist().log_prob(torch::tensor({exp.action.delta}, cuwacunu::kType).to(cuwacunu::kDevice)),
        /* advantage   */ advantage);
      /* Entropy bonus (delta) */
      actor_loss += (-ENTROPY_ALPHA * newLogits.delta_dist().entropy().mean()) / 6.0;
    }

    /* update the actor in batches */
    if (++count % UPDATE_BLOCK_SIZE == 0 || index == (int) episodeBuff.size() - 1) {
      /* Normalize loss by the number of acounted experiences */
      actor_loss /= static_cast<float>(count);

      /* Update the actor */
      actor_optimizer->zero_grad();
      actor_loss.backward();
      actor_optimizer->step();

      /* Reset for next update block */
      actor_loss = torch::zeros({1}, cuwacunu::kType).to(cuwacunu::kDevice);
      count = 0;
    }

    /* Move to the next GAE advantage */
    index++;
  }
}
} /* namespace cuwacunu */
