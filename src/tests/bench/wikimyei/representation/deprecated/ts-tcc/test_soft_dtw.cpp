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
        
        double gamma = 0.1;

        auto softdtw = cuwacunu::wikimyei::ts_tcc::SoftDTW(gamma /*gamma*/, false /*normalize*/);

        // Create a simple sequence: a linear ramp in each feature
        // shape => [1, 5, 3]
        torch::Tensor seq_a = torch::linspace(0, 1, T, torch::dtype(torch::kFloat32))
                                .unsqueeze(-1).repeat({1, E})
                                .unsqueeze(0);

        // seq_b identical
        torch::Tensor seq_b = seq_a.clone();

        /* compute the cost */
        {
            auto [cost, D] = softdtw->forward(seq_a, seq_b);
            print_tensor(cost, "SoftDTW cost: ");
            print_tensor(D, "SoftDTW D: ");
        }

        // If you want gradient wrt x:
        {
            seq_a.requires_grad_(true);
            auto [cost_with_grad, D] = softdtw->forward(seq_a, seq_b);
            print_tensor(cost_with_grad, "Cost with grad: ");
            cost_with_grad.sum().backward();  // backprop
            auto grad_seq_a = seq_a.grad();
            print_tensor(grad_seq_a, "Grad wrt seq_a: ");
            print_tensor(D, "SoftDTW D with grad: ");
        }

        return 0;
    }

    std::cout << "Tests completed. Visually inspect outputs for correctness.\n";
    return 0;
}
