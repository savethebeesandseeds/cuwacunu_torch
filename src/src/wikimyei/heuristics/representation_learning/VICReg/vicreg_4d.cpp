/* vicreg_4d.cpp */
#include <torch/torch.h>
#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {
/* ==================================================== */
/* SAVE                                                 */
/* ==================================================== */
void VICReg_4D::save(const std::string& path)
{
    /* ---- 1. Remember current states ---------------- */
    const bool enc_was_training  = _encoder_net->is_training();
    const bool swa_was_training  = _swa_encoder_net->encoder().is_training();
    const bool proj_was_training = _projector_net->is_training();

    /* ---- 2. Move to CPU for a portable checkpoint -- */
    // this->to(torch::kCPU); // commented out

    /* ---- 3. Switch to eval() so BN running-stats are final */
    _encoder_net->eval();
    _swa_encoder_net->encoder().eval();
    _projector_net->eval();

    /* ---- 4. Pack everything into a single archive -- */
    torch::serialize::OutputArchive root;

    /* 4a. Base encoder (optional but handy for resuming) */
    {
        torch::serialize::OutputArchive a;
        _encoder_net->save(a);
        root.write("encoder_base", a);
    }

    /* 4b. SWA / EMA encoder – the one you’ll usually use at inference */
    {
        torch::serialize::OutputArchive a;
        _swa_encoder_net->encoder().save(a);
        root.write("encoder_swa", a);
    }

    /* 4c. Projector */
    {
        torch::serialize::OutputArchive a;
        _projector_net->save(a);
        root.write("projector", a);
    }

    /* 4d. Optimizer (AdamW) */
    {
        torch::serialize::OutputArchive a;
        optimizer.save(a);
        root.write("adamw", a);
    }

    root.save_to(path);

    /* ---- 5. Restore original device & modes -------- */
    // this->to(device); // commented out

    _encoder_net->train(enc_was_training);
    _swa_encoder_net->encoder().train(swa_was_training);
    _projector_net->train(proj_was_training);

    log_info("VICReg model saved to : %s \n", path.c_str());
}

/* ==================================================== */
/* LOAD                                                 */
/* ==================================================== */
void VICReg_4D::load(const std::string& path)
{
    torch::serialize::InputArchive root;
    root.load_from(path);

    /* ---- 1. Unpack the three sub-modules ------------- */
    {
        torch::serialize::InputArchive a;
        root.read("encoder_base", a);
        _encoder_net->load(a);
    }
    {
        torch::serialize::InputArchive a;
        root.read("encoder_swa", a);
        _swa_encoder_net->encoder().load(a);
    }
    {
        torch::serialize::InputArchive a;
        root.read("projector", a);
        _projector_net->load(a);
    }

    /* ---- 2. Optimizer -------------------------------- */
    {
        torch::serialize::InputArchive a;
        root.read("adamw", a);
        optimizer.load(a);
    }

    /* ---- 3. Push everything to the target device ---- */
    this->to(device);

    /* ---- 4. Ensure gradients are enabled ------------- */
    for (auto& p : this->parameters())
        p.set_requires_grad(true);

    /* print the loaded parameters */
    log_info("VICReg model loaded from : %s \n", path.c_str());
    display_model();
}

} /* namespace vicreg_4d */
} /* namespace wikimyei */
} /* namespace cuwacunu */
