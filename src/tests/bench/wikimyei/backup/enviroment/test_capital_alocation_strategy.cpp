/* test_vicreg_4d_train.cpp */
#include <torch/torch.h>
#include <torch/autograd.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d.h"

int main() {
    using Datatype_t = cuwacunu::camahjucunu::exchange::kline_t;
    auto train_loader = cuwacunu::camahjucunu::data::observation_pipeline_sequential_mm_dataloader<Datatype_t>("BTCUSDT");
    auto shuffle_loader = cuwacunu::camahjucunu::data::observation_pipeline_random_mm_dataloader<Datatype_t>("BTCUSDT");
    
    VICReg_4D(
        int C_, // n channels
        int T_, // n timesteps
        int D_  // n features
    )
    
    while (!obs_space.is_done()) {
        auto obs = obs_space.get_observation();
        auto rep = obs_space.get_representation();
        /* ... feed VicReg, RL-agent, etc. ... */
        obs_space.step();
    }
}