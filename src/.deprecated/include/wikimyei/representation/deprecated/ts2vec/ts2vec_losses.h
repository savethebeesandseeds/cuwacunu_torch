/* losses.h */
#pragma once
#include <torch/torch.h>

namespace cuwacunu {
namespace wikimyei {
namespace ts2vec {
    
// Forward declarations
torch::Tensor instance_contrastive_loss(const torch::Tensor& z1, const torch::Tensor& z2);
torch::Tensor temporal_contrastive_loss(const torch::Tensor& z1, const torch::Tensor& z2);

// Hierarchical Contrastive Loss
inline torch::Tensor hierarchical_contrastive_loss(torch::Tensor z1, torch::Tensor z2, 
    double alpha = 0.5, int temporal_unit = 0) {
    torch::Tensor loss = torch::zeros({}, z1.options());
    int d = 0;

    while (z1.size(1) > 1) {
        if (alpha != 0.0) {
            loss += alpha * instance_contrastive_loss(z1, z2);
        }
        if ((d >= temporal_unit) && ((1.0 - alpha) != 0.0)) {
            loss += (1.0 - alpha) * temporal_contrastive_loss(z1, z2);
        }
        d += 1;

        z1 = torch::max_pool1d(z1.transpose(1, 2), /*kernel_size=*/2).transpose(1, 2);
        z2 = torch::max_pool1d(z2.transpose(1, 2), /*kernel_size=*/2).transpose(1, 2);
    }

    // Final iteration must always increment 'd', even if alpha is zero.
    if (z1.size(1) == 1) {
        if (alpha != 0.0) {
            loss += alpha * instance_contrastive_loss(z1, z2);
        }
        d += 1; // critical: ensure correct averaging
    }

    return loss / d;
}


// Instance Contrastive Loss
inline torch::Tensor instance_contrastive_loss(const torch::Tensor& z1, const torch::Tensor& z2) {
    const auto B = z1.size(0);
    // const auto T = z1.size(1);

    if (B == 1)
        return torch::tensor(0.0, z1.options());

    auto z = torch::cat({z1, z2}, /*dim=*/0);             // [2B, T, C]
    z = z.transpose(0, 1);                                // [T, 2B, C]

    auto sim = torch::matmul(z, z.transpose(1, 2));       // [T, 2B, 2B]

    auto logits_lower = sim.tril(-1).slice(2, 0, -1);     // Lower triangle, excluding diagonal
    auto logits_upper = sim.triu(1).slice(2, 1);          // Upper triangle, excluding diagonal
    auto logits = logits_lower + logits_upper;

    logits = -torch::log_softmax(logits, -1);

    auto idx = torch::arange(B, z1.device());
    auto loss = (logits.index({torch::indexing::Slice(), idx, B + idx - 1}).mean() +
                 logits.index({torch::indexing::Slice(), B + idx, idx}).mean()) / 2;

    return loss;
}

// Temporal Contrastive Loss
inline torch::Tensor temporal_contrastive_loss(const torch::Tensor& z1, const torch::Tensor& z2) {
    // const auto B = z1.size(0);
    const auto T = z1.size(1);

    if (T == 1)
        return torch::tensor(0.0, z1.options());

    auto z = torch::cat({z1, z2}, /*dim=*/1);             // [B, 2T, C]

    auto sim = torch::matmul(z, z.transpose(1, 2));       // [B, 2T, 2T]

    // Carefully reproduce slicing behavior:
    auto logits_lower = sim.tril(-1).slice(2, 0, 2*T - 1);
    auto logits_upper = sim.triu(1).slice(2, 1, 2*T);
    auto logits = logits_lower + logits_upper;

    logits = -torch::log_softmax(logits, -1);

    auto t = torch::arange(T, torch::TensorOptions().dtype(torch::kLong).device(z1.device()));

    // Explicit indexing to avoid slicing confusion:
    auto loss = (
        logits.index({torch::indexing::Slice(), t, T + t - 1}).mean() +
        logits.index({torch::indexing::Slice(), T + t, t}).mean()
    ) / 2;

    return loss;
}

} // namespace ts2vec
} // namespace wikimyei
} // namespace cuwacunu
