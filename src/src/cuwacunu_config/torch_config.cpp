#include "/cuwacunu_config/torch_config.h"

RUNTIME_WARNING("(config.cpp)[] #FIXME be aware to also seed the random number generator for libtorch.\n");
namespace cuwacunu {
namespace config {
  torch::Device kDevice = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
  torch::Dtype kType = torch::kFloat32;
} /* namespace config */
} /* namespace cuwacunu */
