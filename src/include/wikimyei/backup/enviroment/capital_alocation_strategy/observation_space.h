/* observation_space.h --------------------------------------------- */
#pragma once
#include <torch/torch.h>
#include <memory>
#include <string>

#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/data/observation_sample.h"
#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d.h"

RUNTIME_WARNING("(observation_space.h)[]  constrained to VICReg_4D as the Embedding type.\n");

namespace cuwacunu {
namespace wikimyei {
namespace capital_alocation_strategy {

/**
 * @tparam Datatype_t underlying element type in your MM-dataset (e.g. kline_t)
 */
template<typename Datatype_t>
class observation_space_t
{
    /* concrete types pulled together in one place ------------------- */
    using Sampler_t    = torch::data::samplers::SequentialSampler;
    using Dataset_t    = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
    using Datasample_t     = cuwacunu::camahjucunu::data::observation_sample_t;

    using Dataloader_t = cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, Sampler_t>;
    using EmbeddingModel_t  = cuwacunu::wikimyei::vicreg_4d::VICReg_4D;

public:
    /* -------- 1) direct-ownership ctor ----------------------------- */
    observation_space_t(std::unique_ptr<Dataloader_t> dl,
                        std::unique_ptr<EmbeddingModel_t> model)
      : dl_(std::move(dl)), model_(std::move(model))
    {
        TORCH_CHECK(dl_,    "[observation_space_t] dataloader == nullptr");
        TORCH_CHECK(model_, "[observation_space_t] model     == nullptr");
        initialise();
    }

    /* -------- 2) convenience ctor  -------------------------------- */
    explicit observation_space_t(const std::string& instrument)
    {
        /* sequential MM-dataloader created from global config -------- */
        auto loader =
            cuwacunu::camahjucunu::data
                ::observation_pipeline_sequential_mm_dataloader<Datatype_t>(instrument);

        dl_ = std::make_unique<Dataloader_t>(std::move(loader));

        /* VICReg_4D via its (C,T,D) delegating ctor ----------------- */
        model_ = std::make_unique<EmbeddingModel_t>(dl_->C_, dl_->T_, dl_->D_);

        initialise();
    }

    /* ------------------------------------------------- iteration --- */
    bool step()
    {
        if (done_) return false;

        auto maybe_batch = dl_->next();
        if (!maybe_batch.has_value()) { done_ = true; return false; }

        /* collate, move to device, encode --------------------------- */
        curr_sample_ = Datasample_t::collate_fn(*maybe_batch);
        data_        = curr_sample_.features.to(device_);
        mask_        = curr_sample_.mask.to(device_);

        {
            torch::NoGradGuard g;
            encoded_ = model_->encode(data_, mask_);
        }
        return true;
    }

    void reset()                 { dl_->reset(); done_ = false; step(); }
    [[nodiscard]] bool is_done() const { return done_; }

    /* getters ------------------------------------------------------- */
    [[nodiscard]] const Datasample_t& observation()    const { return curr_sample_; }
    [[nodiscard]] torch::Tensor  data()           const { return data_; }
    [[nodiscard]] torch::Tensor  mask()           const { return mask_; }
    [[nodiscard]] torch::Tensor  representation() const { return encoded_; }

private:
    void initialise()
    {
        model_->eval();
        device_ = model_->parameters().front().device();
        step();                                // prime first batch
    }

    /* members ------------------------------------------------------- */
    std::unique_ptr<Dataloader_t> dl_;
    std::unique_ptr<EmbeddingModel_t>  model_;
    torch::Device                device_;

    Datasample_t       curr_sample_{};
    torch::Tensor data_, mask_, encoded_;
    bool          done_{false};
};

} /* namespace capital_alocation_strategy */
} /* namespace wikimyei */
} /* namespace cuwacunu */
