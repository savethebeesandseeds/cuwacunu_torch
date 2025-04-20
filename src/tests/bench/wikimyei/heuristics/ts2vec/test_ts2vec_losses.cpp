#include <torch/torch.h>
#include <iostream>
#include "wikimyei/heuristics/ts2vec/ts2vec_losses.h"


void run_test(int B, int T, int C, double alpha = 0.5, int temporal_unit = 0) {
    torch::manual_seed(0);
    auto opts = torch::TensorOptions().dtype(torch::kFloat32);
    auto z1 = torch::randn({B, T, C}, opts);
    auto z2 = torch::randn({B, T, C}, opts);

    auto loss = cuwacunu::wikimyei::ts2vec::hierarchical_contrastive_loss(z1, z2, alpha, temporal_unit);
    std::cout << "B=" << B << ", T=" << T << ", C=" << C
              << ", alpha=" << alpha << ", temporal_unit=" << temporal_unit
              << " -> Loss: " << loss.item<float>() << std::endl;
}

int main() {
    std::cout << "C++ Tests:" << std::endl;
    run_test(2, 4, 3);
    run_test(1, 4, 3);  // B=1
    run_test(2, 1, 3);  // T=1
    run_test(4, 4, 3);
    run_test(2, 8, 3);
    run_test(2, 4, 3, 1.0);
    run_test(2, 4, 3, 0.0);
    run_test(2, 4, 3, 0.5, 1.0);
    run_test(2, 4, 3, -1.0);
    run_test(2, 4, 3, 0.5, -1.0);
    run_test(2, 4, 3);  // run again to check consistency
}
