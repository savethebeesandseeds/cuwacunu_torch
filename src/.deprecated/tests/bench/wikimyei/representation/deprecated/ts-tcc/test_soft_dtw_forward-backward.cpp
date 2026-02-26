#include <torch/torch.h>
#include <iostream>
#include <vector>
#include <cmath>

#include "piaabo/dutils.h"
#include "wikimyei/heuristics/ts-tcc/soft_dtw.h"

// A helper function to print a 2D (or 1D) tensor
void print_tensor(const torch::Tensor& mat, const std::string& name) {
    std::cout << name << ":\n" << mat << "\n";
}

int main() {
    // Set the seed for reproducibility
    torch::manual_seed(0);

    // Optionally enable anomaly detection
    torch::autograd::AnomalyMode::set_enabled(true);

    /* Test 1: Identical sequences */
    {
        std::cout << "=== Test 1: Identical Sequences ===\n";
        int64_t T = 5; // length
        int64_t E = 1; // embedding dim

        // Create a simple sequence: a linear ramp in each feature
        // shape => [1, 5, 3]
        torch::Tensor seq_a = torch::linspace(0, 1, T, torch::dtype(torch::kFloat32))
                                .unsqueeze(-1).repeat({1, E})
                                .unsqueeze(0);

        // seq_b identical
        torch::Tensor seq_b = seq_a.clone();

        print_tensor(seq_a.squeeze(0), "seq_a (Identical)");
        print_tensor(seq_b.squeeze(0), "seq_b (Identical)");

        // Compute alignment
        TICK(alignment_compute_);
        auto [cost, alignment, R] = cuwacunu::wikimyei::ts_tcc::softdtw_alignment(seq_a, seq_b, 0.1);
        PRINT_TOCK_ns(alignment_compute_);

        print_tensor(R.squeeze(0), "R (Identical)");
        print_tensor(alignment.squeeze(0), "Alignment (Identical)");
        print_tensor(alignment.sum(-1), "Alignment (Identical)");
        print_tensor(cost.squeeze(0), "Cost (Identical)");
    }

    // /* Test 2: Slightly Different Sequences (Shifted) */
    // {
    //     std::cout << "=== Test 2: Shifted Sequences ===\n";
    //     int64_t T = 5;
    //     int64_t E = 3;

    //     torch::Tensor seq_a = torch::linspace(0, 1, T, torch::dtype(torch::kFloat32))
    //                             .unsqueeze(-1).repeat({1, E})
    //                             .unsqueeze(0); // [1, 5, 3]

    //     // shift seq_a by 1 step to create seq_b
    //     torch::Tensor seq_b = torch::cat({
    //         seq_a.index({torch::indexing::Slice(),
    //                      torch::indexing::Slice(1, torch::indexing::None),
    //                      torch::indexing::Slice()}),
    //         seq_a.index({torch::indexing::Slice(),
    //                      torch::indexing::Slice(torch::indexing::None, 1),
    //                      torch::indexing::Slice()})
    //     }, /*dim=*/1);

    //     TICK(alignment_compute_);
    //     auto [cost, alignment, R] = cuwacunu::wikimyei::ts_tcc::softdtw_alignment(seq_a, seq_b, 0.1);
    //     PRINT_TOCK_ns(alignment_compute_);

    //     print_tensor(alignment.squeeze(0), "Alignment (Shifted)");
    //     print_tensor(alignment.sum(-1), "Alignment (Shifted)");
    //     print_tensor(cost.squeeze(0), "Cost (Shifted)");
    // }

    // /* Test 3: Check sums */
    // {
    //     std::cout << "=== Test 3: Check Row Sums ===\n";
    //     int64_t B = 1;
    //     int64_t T = 5;
    //     int64_t E = 2;

    //     torch::Tensor seq_a = torch::rand({B, T, E});
    //     torch::Tensor seq_b = torch::rand({B, T, E});

    //     TICK(alignment_compute_);
    //     auto [cost, alignment, R] = cuwacunu::wikimyei::ts_tcc::softdtw_alignment(seq_a, seq_b, 0.1);
    //     PRINT_TOCK_ns(alignment_compute_);

    //     // sum over columns => shape [B, T]
    //     auto row_sums = alignment.sum(/*dim=*/-1);

    //     print_tensor(row_sums, "Row sums of alignment");
    //     print_tensor(cost,  "Cost (Random seqs for Row Sum check)");
    // }

    // /* Test 4: Multiple sequences in the same batch */
    // {
    //     std::cout << "=== Test 4: Multiple Sequences in Batch ===\n";
    //     int64_t B = 2;
    //     int64_t T = 5;
    //     int64_t E = 3;

    //     torch::Tensor base = torch::linspace(0, 1, T, torch::dtype(torch::kFloat32))
    //                             .unsqueeze(-1).repeat({1, E});
    //     // [B=2, T=5, E=3]
    //     torch::Tensor seq_a = base.unsqueeze(0).repeat({B, 1, 1});
    //     torch::Tensor seq_b = seq_a.clone();

    //     TICK(alignment_compute_);
    //     auto [cost, alignment, R] = cuwacunu::wikimyei::ts_tcc::softdtw_alignment(seq_a, seq_b, 0.1);
    //     PRINT_TOCK_ns(alignment_compute_);

    //     // Print each batch element
    //     for (int i = 0; i < B; i++) {
    //         // alignment[i] => shape [5,5]
    //         print_tensor(alignment[i], "Alignment (Batch element " + std::to_string(i) + ")");
    //         // cost[i] => single double cost
    //         std::cout << "Cost (Batch element " << i << "): " << cost[i].item<double>() << "\n";
    //     }
    // }

    // /* Test 5: Different gamma values */
    // {
    //     std::cout << "=== Test 5: Varying Gamma ===\n";
    //     int64_t T = 5;
    //     int64_t E = 3;

    //     torch::Tensor seq_a = torch::linspace(0, 1, T, torch::dtype(torch::kFloat32))
    //                             .unsqueeze(-1).repeat({1, E})
    //                             .unsqueeze(0);
    //     torch::Tensor seq_b = seq_a.clone();

    //     // small gamma
    //     {
    //         TICK(alignment_compute_);
    //         auto [cost, alignment, R] = cuwacunu::wikimyei::ts_tcc::softdtw_alignment(seq_a, seq_b, 0.001);
    //         PRINT_TOCK_ns(alignment_compute_);

    //         print_tensor(alignment.squeeze(0), "Alignment (gamma=0.001)");
    //         print_tensor(alignment.sum(-1), "Alignment (gamma=0.001)");
    //         print_tensor(cost.squeeze(0),           "Cost (gamma=0.001)");
    //     }

    //     // larger gamma
    //     {
    //         TICK(alignment_compute_2_);
    //         auto [cost, alignment, R] = cuwacunu::wikimyei::ts_tcc::softdtw_alignment(seq_a, seq_b, 1.0);
    //         PRINT_TOCK_ns(alignment_compute_2_);

    //         print_tensor(alignment.squeeze(0), "Alignment (gamma=1.0)");
    //         print_tensor(alignment.sum(-1), "Alignment (gamma=1.0)");
    //         print_tensor(cost.squeeze(0),            "Cost (gamma=1.0)");
    //     }
    // }

    // /* Test 6: Longer sequences */
    // {
    //     std::cout << "=== Test 6: Longer Sequences ===\n";
    //     int64_t B = 1;
    //     int64_t T = 20;
    //     int64_t E = 2;

    //     torch::Tensor seq_a = torch::rand({B, T, E});
    //     torch::Tensor seq_b = torch::rand({B, T, E});

    //     TICK(alignment_compute_);
    //     auto [cost, alignment, R] = cuwacunu::wikimyei::ts_tcc::softdtw_alignment(seq_a, seq_b, 0.1);
    //     PRINT_TOCK_ns(alignment_compute_);

    //     print_tensor(alignment.squeeze(0), "Alignment (Random 20-length seqs)");
    //     print_tensor(alignment.sum(-1), "Alignment (Random 20-length seqs)");
    //     print_tensor(cost.squeeze(0), "Cost (Random 20-length seqs)");

    //     bool has_nan = alignment.isnan().any().item<bool>();
    //     std::cout << "Contains NaNs? " << (has_nan ? "Yes" : "No") << "\n";
    // }

    // /* Test 7: Tiny Problem (T=2) with manual expectations */
    // {
    //     std::cout << "=== Test 7: Tiny Problem Manual Check ===\n";
    //     // seq_a: [ [0.0], [1.0] ]
    //     // seq_b: [ [0.0], [1.0] ]
    //     torch::Tensor seq_a = torch::tensor({{0.0f}, {1.0f}}).unsqueeze(0); // [1,2,1]
    //     torch::Tensor seq_b = seq_a.clone();

    //     TICK(alignment_compute_);
    //     auto [cost, alignment, R] = cuwacunu::wikimyei::ts_tcc::softdtw_alignment(seq_a, seq_b, 0.1);
    //     PRINT_TOCK_ns(alignment_compute_);

    //     print_tensor(alignment.squeeze(0), "Alignment (Tiny)");
    //     print_tensor(alignment.sum(-1), "Alignment (Tiny)");
    //     print_tensor(cost.squeeze(0),  "Cost (Tiny)");
    // }

    // /* Test 8: Gradient Backpropagation */
    // {
    //     std::cout << "=== Test 8: Gradient Backprop ===\n";
    //     // We'll use random sequences of shape [1,5,3].
    //     auto seq_a = torch::rand({1, 5, 3}, torch::dtype(torch::kFloat32));
    //     auto seq_b = torch::rand({1, 5, 3}, torch::dtype(torch::kFloat32));

    //     // Mark them as requiring grad
    //     seq_a.requires_grad_(true);
    //     seq_b.requires_grad_(true);

    //     // Compute Soft-DTW cost + alignment
    //     TICK(alignment_compute_grad_);
    //     auto [cost, alignment, R] = cuwacunu::wikimyei::ts_tcc::softdtw_alignment(seq_a, seq_b, 0.1);
    //     PRINT_TOCK_ns(alignment_compute_grad_);

    //     // cost => shape [1]; turn it into a single scalar for backward
    //     auto loss = cost.sum();
    //     std::cout << "Loss: " << loss.item<double>() << "\n";

    //     // Now do the backward pass
    //     loss.backward();

    //     std::cout << "Gradient wrt seq_a:\n" << seq_a.grad() << "\n";
    //     std::cout << "Gradient wrt seq_b:\n" << seq_b.grad() << "\n";
    // }

    std::cout << "Tests completed. Visually inspect outputs for correctness.\n";
    return 0;
}
