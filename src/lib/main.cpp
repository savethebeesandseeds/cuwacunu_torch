#include <iostream>
#include <torch/torch.h>
#include <cuda_runtime.h>

void cuda_runtime_aviability() {
    int nDevices;
    cudaGetDeviceCount(&nDevices);
    for (int i = 0; i < nDevices; i++) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);
        std::cout << "Device Number: " << i << std::endl;
        std::cout << "  Device name: " << prop.name << std::endl;
        std::cout << "  Compute capability: " << prop.major << "." << prop.minor << std::endl;
    }
}

int main() {
    if(torch::cuda::is_available()) {
        fprintf(stdout, "CUDA is available \n");
        cuda_runtime_aviability();
    } else {
        fprintf(stdout, "CUDA is NOT available \n");
    }
    return 0;
}
