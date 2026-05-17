#pragma once

#include <torch/torch.h>

#include <string>

#include "iitepi/iitepi.h"

namespace cuwacunu {
namespace piaabo {
namespace tensor {
namespace torch {

::torch::Dtype
config_dtype(const cuwacunu::iitepi::contract_hash_t &contract_hash,
             const std::string &section = "GENERAL");

::torch::Device
config_device(const cuwacunu::iitepi::contract_hash_t &contract_hash,
              const std::string &section = "GENERAL");

} // namespace torch
} // namespace tensor
} // namespace piaabo
} // namespace cuwacunu
