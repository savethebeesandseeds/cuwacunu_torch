#include <torch/torch.h>

int main() {
    torch::cuda::is_available();
    return 0;
}
