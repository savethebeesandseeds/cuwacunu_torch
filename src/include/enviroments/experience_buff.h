#pragma once
#include "../dtypes.h"

namespace cuwacunu {
class ExperienceBuffer {
private:
    int batch_size_;
    std::vector<cuwacunu::experience_t> experienceMemory;
    std::default_random_engine generator; // For random sampling

public:
    ExperienceBuffer(int batch_size)
    : batch_size_(batch_size), experienceMemory({}) {}

    virtual ~ExperienceBuffer() {}

    void addExperience(cuwacunu::experience_t exp) {
        experienceMemory.push_back(exp);
    }

    cuwacunu::experienceBatch_t sampleBatch() {
        
        ... experience replay makes little sense in on-policy learning, revisate to avoid this class
        ... make the batch be the entire episode
        ... sample a batch for on-policy learning!



        std::vector<torch::Tensor> states, actions, rewards, next_states;
        std::vector<uint8_t> dones; // Use uint8_t for boolean representation in tensor

        // Ensure there's enough experiences to sample
        int available_samples = std::min(batch_size_, static_cast<int>(experienceMemory.size()));
        
        std::uniform_int_distribution<int> distribution(0, experienceMemory.size() - 1);

        for (int i = 0; i < available_samples; ++i) {
            int index = distribution(generator); // Randomly select an index
            auto& exp = experienceMemory[index];
            states.push_back(exp.state);
            actions.push_back(exp.action);
            rewards.push_back(torch::tensor(exp.reward, torch::TensorOptions().device(cuwacunu::kDevice)));
            next_states.push_back(exp.next_state);
            dones.push_back(static_cast<uint8_t>(exp.done));
        }

        auto batch_states = torch::stack(states).to(cuwacunu::kDevice);
        auto batch_actions = torch::stack(actions).to(cuwacunu::kDevice);
        auto batch_rewards = torch::stack(rewards).to(torch::kFloat32).to(cuwacunu::kDevice);
        auto batch_next_states = torch::stack(next_states).to(cuwacunu::kDevice);;
        auto batch_dones = torch::tensor(dones, torch::TensorOptions().dtype(torch::kFloat32).device(cuwacunu::kDevice));

        return cuwacunu::experienceBatch_t{batch_states, batch_actions, batch_rewards, batch_next_states, batch_dones};
    }
};
}