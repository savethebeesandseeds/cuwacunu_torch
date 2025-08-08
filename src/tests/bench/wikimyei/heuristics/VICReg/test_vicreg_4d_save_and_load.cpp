/* test_vicreg_4d_save_and_load.cpp
 *
 * Quick smoke test for VICReg_4D::save / load.
 * --------------------------------------------------------
 */

#include <torch/torch.h>
#include <iostream>
#include <filesystem>
#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d.h"

/* handy macro - prints FAIL & exits if cond is false */
#define ASSERT_TRUE(cond, msg)                    \
    do {                                          \
        if (!(cond)) {                            \
            std::cerr << "ASSERT - " << msg       \
                      << " (line " << __LINE__    \
                      << ")\n";                   \
            return 1;                             \
        }                                         \
    } while (0)

/* ---------- helper so we don’t repeat 20 ctor args ---------- */
template <class DL>
cuwacunu::wikimyei::vicreg_4d::VICReg_4D
make_model(const DL& dl)
{
    using cuwacunu::piaabo::dconfig::config_space_t;
    using cuwacunu::wikimyei::vicreg_4d::VICReg_4D;

    return VICReg_4D(
        dl.C_, dl.T_, dl.D_,
        config_space_t::get<int>   ("VICReg", "encoding_dims"),
        config_space_t::get<int>   ("VICReg", "channel_expansion_dim"),
        config_space_t::get<int>   ("VICReg", "fused_feature_dim"),
        config_space_t::get<int>   ("VICReg", "encoder_hidden_dims"),
        config_space_t::get<int>   ("VICReg", "encoder_depth"),
        config_space_t::get        ("VICReg", "projector_mlp_spec"),
        config_space_t::get<double>("VICReg", "sim_coeff"),
        config_space_t::get<double>("VICReg", "std_coeff"),
        config_space_t::get<double>("VICReg", "cov_coeff"),
        config_space_t::get<double>("VICReg", "optimizer_base_lr"),
        config_space_t::get<double>("VICReg", "optimizer_weight_decay"),
        config_space_t::get<int>   ("VICReg", "optimizer_lr_cycle"),
        config_space_t::get<int>   ("VICReg", "optimizer_lr_warmup_epochs"),
        config_space_t::get<double>("VICReg", "optimizer_lr_min"),
        config_space_t::get<bool>  ("VICReg", "optimizer_clamp_weights"),
        config_space_t::get<int>   ("VICReg", "optimizer_threshold_reset"),
        cuwacunu::piaabo::dconfig::config_dtype ("VICReg"),
        cuwacunu::piaabo::dconfig::config_device("VICReg"),
        config_space_t::get<bool>  ("VICReg", "enable_buffer_averaging"));
}

int main()
{
    /* -------------------------------------------------- */
    /*  0) Torch & CUDA housekeeping                      */
    /* -------------------------------------------------- */
    torch::autograd::AnomalyMode::set_enabled(false);
    torch::globalContext().setBenchmarkCuDNN(true);
    torch::globalContext().setDeterministicCuDNN(false);
    WARM_UP_CUDA();

    /* -------------------------------------------------- */
    /*  1) Load config                                    */
    /* -------------------------------------------------- */
    const char* CONFIG_ROOT = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(CONFIG_ROOT);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    /* reproducibility */
    torch::manual_seed(cuwacunu::piaabo::dconfig::config_space_t::get<int>("GENERAL", "torch_seed"));

    /* -------------------------------------------------- */
    /*  2) Build dataloader (same as in your training     */
    /* -------------------------------------------------- */
    using Td = cuwacunu::camahjucunu::exchange::kline_t;
    using Q  = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Td>;
    using K  = cuwacunu::camahjucunu::data::observation_sample_t;
    using SeqSampler = torch::data::samplers::SequentialSampler;

    std::string INSTRUMENT = "BTCUSDT";

    auto dl = cuwacunu::camahjucunu::data::create_memory_mapped_dataloader<Q, K, Td, SeqSampler>(
        INSTRUMENT,                                                                                             /* instrument */
        cuwacunu::camahjucunu::BNF::observationPipeline()
            .decode(cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction()),             /* obs_inst */ 
        cuwacunu::piaabo::dconfig::config_space_t::get<bool>    ("DATA_LOADER", "dataloader_force_binarization"),    /* force_binarization */
        cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("DATA_LOADER", "dataloader_batch_size"),            /* batch_size */
        cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("DATA_LOADER", "dataloader_workers")                /* workers */
    );

    /* -------------------------------------------------- */
    /*  3) Instantiate two identical models               */
    /* -------------------------------------------------- */
    auto model_A = make_model(dl);
    auto model_B = make_model(dl);        // <- no copy-ctor needed

    /* -------------------------------------------------- */
    /*  4) Grab ONE batch & run forward                   */
    /* -------------------------------------------------- */
    auto first_it = dl.begin();
    ASSERT_TRUE(first_it != dl.end(), "empty dataloader");
    auto sample_batch = *first_it;
    auto sample       = K::collate_fn(sample_batch);

    /* keep everything on CPU so round-trip stays exact */
    auto feats = sample.features.detach().to(cuwacunu::piaabo::dconfig::config_device("VICReg"));
    auto mask  = sample.mask.detach().to(cuwacunu::piaabo::dconfig::config_device("VICReg"));

    auto out_A = model_A.encode(feats, mask).detach().cpu();

    /* -------------------------------------------------- */
    /*  5) Save & Load                                    */
    /* -------------------------------------------------- */
    const std::string ckpt = "/tmp/vicreg_smoke.ckpt";
    model_A.save(ckpt);
    model_B.load(ckpt);

    auto out_B = model_B.encode(feats, mask).detach().cpu();

    /* -------------------------------------------------- */
    /*  6) Compare                                        */
    /* -------------------------------------------------- */
    auto diff = (out_A - out_B).abs().max().item<double>();
    std::cout << "\nMax |Δ| between pre-save and post-load outputs: "
              << diff << '\n';

    ASSERT_TRUE(diff < 1e-6,
                "Outputs differ - save/load broke something!");

    std::cout << "Smoke test **PASSED** ✅\n";
    return 0;
}
