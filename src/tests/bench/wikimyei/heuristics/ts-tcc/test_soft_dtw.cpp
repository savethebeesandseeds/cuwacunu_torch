#include <torch/torch.h>
#include <iostream>
#include <vector>
#include <cmath>

#include "piaabo/dutils.h"
#include "wikimyei/heuristics/ts-tcc/soft_dtw.h"

/* A helper function to print a 2D tensor (for debugging) */
void print_matrix(const torch::Tensor& mat, const std::string& name) {
    std::cout << name << ":\n" << mat << "\n";
}

int main() {
    /* Set the seed for reproducibility */
    torch::manual_seed(0);

    /* Test 1: Identical sequences */
    {
        std::cout << "=== Test 1: Identical Sequences ===\n";
        /* int64_t B = 1; /* one sequence */
        int64_t T = 5; /* length 5 */
        int64_t E = 3; /* embedding dimension 3 */

        /* Create a simple sequence: a linear ramp in each feature */
        torch::Tensor seq_a = torch::linspace(0, 1, T).unsqueeze(-1).repeat({1, E});
        /* seq_a shape: (T, E) */
        /* Make it batch: (B, T, E) */
        seq_a = seq_a.unsqueeze(0); /* (1, 5, 3) */

        /* seq_b identical to seq_a */
        torch::Tensor seq_b = seq_a.clone();

        /* Compute alignment */
        TICK(alignment_compute_);
        auto alignment = cuwacunu::wikimyei::ts_tcc::compute_alignment_matrix_softdtw(seq_a, seq_b, 0.1);
        PRINT_TOCK_ns(alignment_compute_);

        print_matrix(alignment.squeeze(0), "Alignment (Identical)");

        /* Expectation: alignment should be close to a diagonal pattern. */
        /* You can inspect the printed matrix. The maximum value in each row  */
        /* should be near the diagonal element. */
    }

    /* Test 2: Slightly Different Sequences */
    {
        std::cout << "=== Test 2: Shifted Sequences ===\n";
        /* int64_t B = 1; */
        int64_t T = 5;
        int64_t E = 3;

        torch::Tensor seq_a = torch::linspace(0, 1, T).unsqueeze(-1).repeat({1, E});
        seq_a = seq_a.unsqueeze(0); /* (1, 5, 3) */

        /* seq_b: shifted by one time step. For instance, roll seq_a by 1 step. */
        torch::Tensor seq_b = torch::cat({seq_a.index({torch::indexing::Slice(), torch::indexing::Slice(1,torch::indexing::None), torch::indexing::Slice()}), 
                                          seq_a.index({torch::indexing::Slice(), torch::indexing::Slice(torch::indexing::None,1), torch::indexing::Slice()})}, 1);
        /* Now seq_b[t] corresponds to seq_a[t-1] for t>0, wrapped around at the start. */

        TICK(alignment_compute_);
        auto alignment = cuwacunu::wikimyei::ts_tcc::compute_alignment_matrix_softdtw(seq_a, seq_b, 0.1);
        PRINT_TOCK_ns(alignment_compute_);
        print_matrix(alignment.squeeze(0), "Alignment (Shifted)");

        /* Expectation: The alignment should not be strictly diagonal. The highest probabilities  */
        /* should be somewhat off the main diagonal, showing that the algorithm tries to "warp"  */
        /* one sequence to match the other. */
    }

    /* Test 3: Check sums */
    {
        std::cout << "=== Test 3: Check Row Sums ===\n";
        int64_t B = 1;
        int64_t T = 5;
        int64_t E = 2;

        torch::Tensor seq_a = torch::rand({B, T, E});
        torch::Tensor seq_b = torch::rand({B, T, E});

        
        TICK(alignment_compute_);
        auto alignment = cuwacunu::wikimyei::ts_tcc::compute_alignment_matrix_softdtw(seq_a, seq_b, 0.1);
        PRINT_TOCK_ns(alignment_compute_);
        
        /* Check row sums: */
        auto row_sums = alignment.sum(-1); /* sum over columns => (B, T) */
        print_matrix(row_sums, "Row sums of alignment");

        /* Row sums might not perfectly sum to 1 if the implementation is raw gradients of soft-DTW. */
        /* If you implemented normalization, you'd expect sums close to 1.  */
        /* If not normalized, they should still be finite and meaningful. */
    }

    /* Test 4: Multiple sequences in the same batch */
    {
        std::cout << "=== Test 4: Multiple Sequences in Batch ===\n";
        int64_t B = 2;
        int64_t T = 5;
        int64_t E = 3;

        /* Create two identical sequences and store them in a batch */
        torch::Tensor base = torch::linspace(0, 1, T).unsqueeze(-1).repeat({1, E}); /* (T,E) */
        torch::Tensor seq_a = base.unsqueeze(0).repeat({B,1,1}); /* (B=2, T=5, E=3), both identical */
        torch::Tensor seq_b = seq_a.clone();

        TICK(alignment_compute_);
        auto alignment = cuwacunu::wikimyei::ts_tcc::compute_alignment_matrix_softdtw(seq_a, seq_b, 0.1);
        PRINT_TOCK_ns(alignment_compute_);

        /* Print each batch element separately */
        for (int i = 0; i < B; i++) {
            print_matrix(alignment[i], "Alignment (Batch element " + std::to_string(i) + ")");
        }

        /* Expect: Each batch element should produce a similar diagonal-like alignment since seq_a == seq_b. */
    }

    /* Test 5: Different gamma values */
    {
        std::cout << "=== Test 5: Varying Gamma ===\n";
        /* int64_t B = 1; */
        int64_t T = 5;
        int64_t E = 3;

        torch::Tensor seq_a = torch::linspace(0, 1, T).unsqueeze(-1).repeat({1, E}).unsqueeze(0);
        torch::Tensor seq_b = seq_a.clone();

        /* Small gamma */
        TICK(alignment_compute_);
        auto alignment_small_gamma = cuwacunu::wikimyei::ts_tcc::compute_alignment_matrix_softdtw(seq_a, seq_b, 0.001);
        PRINT_TOCK_ns(alignment_compute_);
        print_matrix(alignment_small_gamma.squeeze(0), "Alignment (gamma=0.001)");

        /* Larger gamma */
        TICK(alignment_compute_2_);
        auto alignment_large_gamma = cuwacunu::wikimyei::ts_tcc::compute_alignment_matrix_softdtw(seq_a, seq_b, 1.0);
        PRINT_TOCK_ns(alignment_compute_2_);
        print_matrix(alignment_large_gamma.squeeze(0), "Alignment (gamma=1.0)");

        /* Expectation: With gamma=0.001, a sharper (nearly diagonal) alignment. */
        /* With gamma=1.0, a more uniform distribution across other timesteps. */
    }


    /* Test 6: Longer sequences */
    {
        std::cout << "=== Test 6: Longer Sequences ===\n";
        int64_t B = 1;
        int64_t T = 20;
        int64_t E = 2;

        torch::Tensor seq_a = torch::rand({B, T, E});
        torch::Tensor seq_b = torch::rand({B, T, E});

        TICK(alignment_compute_);
        auto alignment = cuwacunu::wikimyei::ts_tcc::compute_alignment_matrix_softdtw(seq_a, seq_b, 0.1);
        PRINT_TOCK_ns(alignment_compute_);
        print_matrix(alignment.squeeze(0), "Alignment (Random 20-length seqs)");

        /* Check for NaNs: */
        bool has_nan = alignment.isnan().any().item<bool>();
        std::cout << "Contains NaNs? " << (has_nan ? "Yes" : "No") << "\n";

        /* Expect: No NaNs, a valid probability-like distribution of alignments. */
    }


    /* Test 7: Tiny Problem (T=2) with manual expectations */
    {
        std::cout << "=== Test 7: Tiny Problem Manual Check ===\n";
        /* int64_t B = 1; */
        /* int64_t T = 2; */
        /* int64_t E = 1; */

        /* seq_a: [ [0], [1] ] */
        /* seq_b: [ [0], [1] ] */
        torch::Tensor seq_a = torch::tensor({{0.0}, {1.0}}).unsqueeze(0); /* (1,2,1) */
        torch::Tensor seq_b = seq_a.clone();

        TICK(alignment_compute_);
        auto alignment = cuwacunu::wikimyei::ts_tcc::compute_alignment_matrix_softdtw(seq_a, seq_b, 0.1);
        PRINT_TOCK_ns(alignment_compute_);
        print_matrix(alignment.squeeze(0), "Alignment (Tiny)");

        /* Since they are identical and length=2: */
        /* The best alignment is likely near the diagonal:  */
        /* alignment ~ [[high, very low], */
        /*              [very low, high]] */
    }




    std::cout << "Tests completed. Visually inspect outputs for correctness.\n";
    return 0;
}
