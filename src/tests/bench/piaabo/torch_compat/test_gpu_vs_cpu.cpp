// benchmark_mm.cpp
#include <torch/torch.h>
#include <iostream>
#include <chrono>
#include <cuda_runtime.h>

int main() {
    // Check for CUDA
    if (!torch::cuda::is_available()) {
        std::cerr << "CUDA is not available. Exiting.\n";
        return 1;
    }

    // Devices
    torch::Device cpu_dev(torch::kCPU);
    torch::Device gpu_dev(torch::kCUDA, /*index=*/0);

    // Problem size and repetitions
    const int64_t N = 4096;           // matrix dimension
    const int repeat = 20;            // number of multiplies to average

    // Allocate random matrices on CPU and copy to GPU
    auto A_cpu = torch::rand({N, N}, cpu_dev);
    auto B_cpu = torch::rand({N, N}, cpu_dev);
    auto A_gpu = A_cpu.to(gpu_dev);
    auto B_gpu = B_cpu.to(gpu_dev);

    // Warm up GPU (trigger JIT/kernel compilation etc.)
    {
        auto C = torch::mm(A_gpu, B_gpu);
        torch::cuda::synchronize();
    }

    // --- GPU timing with CUDA events ---
    cudaEvent_t start_evt, stop_evt;
    cudaEventCreate(&start_evt);
    cudaEventCreate(&stop_evt);

    cudaEventRecord(start_evt);
    for (int i = 0; i < repeat; ++i) {
        auto C = torch::mm(A_gpu, B_gpu);
    }
    cudaEventRecord(stop_evt);
    cudaEventSynchronize(stop_evt);

    float gpu_ms = 0;
    cudaEventElapsedTime(&gpu_ms, start_evt, stop_evt);
    cudaEventDestroy(start_evt);
    cudaEventDestroy(stop_evt);

    // --- CPU timing with std::chrono ---
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < repeat; ++i) {
        auto C = torch::mm(A_cpu, B_cpu);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double cpu_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // Results
    std::cout << "Matrix multiply " << repeat << "× on " << N << "×" << N << ":\n";
    std::cout << "  GPU time: " << gpu_ms  << " ms  (avg " << gpu_ms/repeat << " ms)\n";
    std::cout << "  CPU time: " << cpu_ms  << " ms  (avg " << cpu_ms/repeat << " ms)\n";

    return 0;
}
