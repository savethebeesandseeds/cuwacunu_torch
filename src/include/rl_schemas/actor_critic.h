#pragma once
/* settle up for multiple continious actions */
#include "../dtypes.h"

#define RL_GAMMA 0.99 // reinforcement learning future gains discount (γ)

namespace cuwacunu {
class ActorCriticSchema {
private:
    std::shared_ptr<cuwacunu::ActorModel> actor_;
    std::shared_ptr<cuwacunu::CriticModel> critic_;
    std::shared_ptr<cuwacunu::Environment> environment_;
    std::shared_ptr<torch::optim::Optimizer> optimizer_actor;
    std::shared_ptr<torch::optim::Optimizer> optimizer_critic;
    std::shared_ptr<torch::optim::lr_scheduler::LambdaLR> lr_scheduler_actor;
    std::shared_ptr<torch::optim::lr_scheduler::LambdaLR> lr_scheduler_critic;
public:
    ActorCriticSchema(
        std::shared_ptr<cuwacunu::ActorModel> actor,
        std::shared_ptr<cuwacunu::CriticModel> critic,
        std::shared_ptr<cuwacunu::Environment> environment)
    : actor_(std::move(actor)), critic_(std::move(critic)), environment_(std::move(environment)), gamma_(RL_GAMMA) {
        // Ensure the model is on the correct device
        actor_->to(cuwacunu::kDevice);
        critic_->to(cuwacunu::kDevice);
        
        // Initialize optimizers
        optimizer_actor = std::make_shared<torch::optim::Adam>(actor_->parameters(), torch::optim::AdamOptions(1e-4));
        optimizer_critic = std::make_shared<torch::optim::Adam>(critic_->parameters(), torch::optim::AdamOptions(1e-3));

        // Initialize learning rate schedulers
        lr_scheduler_actor = std::make_shared<torch::optim::LambdaLR>(
            optimizer_actor.get(),
            [](int epoch) { return 1.0; }
        );
        lr_scheduler_critic = std::make_shared<torch::optim::LambdaLR>(
            optimizer_critic.get(),
            [](int epoch) { return 1.0; }
        );
    }


    void train(int episodes, int batchSize, bool useExperienceReplay = true) {
        torch::Tensor state;
        torch::Tensor action;
        cuwacunu::experience_t exp;
        cuwacunu::experienceBatch_t expBatch;
        cuwacunu::ExperienceBuffer experienceBuffer(batchSize);

        for (int episode = 0; episode < episodes; ++episode) {
            state = environment_->reset();
            
            do {
                expBatch = {};
                action = actor_->selectAction(state);
                exp = environment_->step(action);

                // Add experience to buffer
                experienceBuffer.addExperience(exp);

                if (useExperienceReplay) {
                    // Experience replay mode: Sample a batch from the buffer to update models
                    expBatch = experienceBuffer.sampleBatch();
                } else {
                    // Online training mode: Update models directly with the most recent experience batch_size = 1
                    expBatch.states = exp.state.unsqueeze(0);
                    expBatch.actions = exp.action.unsqueeze(0);
                    expBatch.rewards = torch::tensor({exp.reward});
                    expBatch.next_states = exp.next_state.unsqueeze(0);
                    expBatch.dones = torch::tensor({static_cast<double>(exp.done)});    
                }
                // train on experience
                updateModels(expBatch);

                state = exp.next_state;
            } while (!exp.done);
        }
    }

private:
    // Method to update actor and critic models based on the experience
    void updateModels(cuwacunu::experienceBatch_t& expBatch) {
        ... what if actions where categorical and or continious and so then different updates depending more on checking that .sum(-1, true)
        // Assuming 'expBatch.states', 'expBatch.actions', 'expBatch.rewards', 'expBatch.next_states', and 'expBatch.dones' are all batched tensors
        auto current_values = critic_->forward(expBatch.states);
        auto next_values = critic_->forward(expBatch.next_states) * (1 - expBatch.dones); // Apply mask for dones to zero-out values of terminal states
        auto expected_values = expBatch.rewards + gamma_ * expBatch.next_values.detach();
        auto advantages = expected_values - current_values;

        // For the actor update, we need to re-derive 'means' and 'log_stds' based on current policy
        auto normal_dists = actor_->getActionDistribution(expBatch.states);
        auto log_probs = normal_dists.log_prob(expBatch.actions).sum(-1, true); // Calculate log_prob for each action in the batch
        auto actor_loss = -(log_probs * advantages.detach()).mean(); // Aggregate over the batch

        auto critic_loss = torch::mse_loss(current_values, expected_values.detach());

        optimizer_actor->zero_grad();
        actor_loss.backward();
        optimizer_actor->step();

        optimizer_critic->zero_grad();
        critic_loss.backward();
        optimizer_critic->step();

        // update the learning rates
        lr_scheduler_actor->step();
        lr_scheduler_critic->step();
    }
};
}
