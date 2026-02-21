/* test_vicreg_4d_projector_gpu_vs_cpu.cpp */
#include <torch/torch.h>
#include <iostream>

#include "piaabo/dutils.h"
#include "piaabo/torch_compat/torch_utils.h"
#include "wikimyei/representation/VICReg/vicreg_4d_projector.h"

int main() {
    const int embedding_dim = 64;
    const std::string mlp_spec = "8451-9547-1212-64";
    const int B = 64, T = 30;         /* batch size and time steps */
    const int warmup_iters = 1;
    const int test_iters   = 100;
    auto dtype = torch::kFloat32;
    bool synchronize = true;

    /* pick devices */
    std::vector<torch::Device> devices;
    if (torch::cuda::is_available()) { devices.emplace_back(torch::kCUDA); }
    devices.emplace_back(torch::kCPU);

    log_info("Strating test\n");
    
    /* 0) warm the device */
    WARM_UP_CUDA();

    for (auto &device : devices) {
        log_info("Testing on: %s\n", (device.is_cpu() ? "cpu" : "gpu"));

        /* 1) Measure module construction time */
        TICK(creation_);
        auto model = cuwacunu::wikimyei::vicreg_4d::VICReg_4D_Projector(embedding_dim, mlp_spec, dtype, device);
        model->eval();
        PRINT_TOCK_ms(creation_);
        
        /* 2) Prepare a dummy input and warm up */
        auto input = torch::randn({B, T, embedding_dim}, 
            torch::TensorOptions().device(device).dtype(dtype));

        /* 3) warm up forwarding */
        TICK(forward_warm_up_);
        for (int i = 0; i < warmup_iters; ++i) { auto out = model->forward(input); }
        if (!device.is_cpu() && synchronize) torch::cuda::synchronize();
        PRINT_TOCK_ms(forward_warm_up_);

        /* 4) Measure forward pass time */
        TICK(forward__);
            for (int i = 0; i < test_iters; ++i) { auto out = model->forward(input); }
            if (!device.is_cpu() && synchronize) torch::cuda::synchronize();
        PRINT_TOCK_ms(forward__);
    }

    log_info("Finishing...\n");

    return 0;
}
