/* observation_space.h --------------------------------------------- */
#pragma once
#include <torch/torch.h>
#include <memory>
#include <string>

#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/data/observation_sample.h"          // observation_sample_t
#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d.h"

RUNTIME_WARNING("(observation_space.h)[]  constrained to VICReg_4D as the Embedding type.\n");

namespace cuwacunu {
namespace wikimyei {
namespace capital_alocation_strategy {

/**
 * @tparam Td underlying element type in your MM-dataset (e.g. kline_t)
 */
template<typename Td>
class observation_space_t
{
    /* concrete types pulled together in one place ------------------- */
    using SamplerT    = torch::data::samplers::SequentialSampler;
    using DatasetT    = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Td>;
    using SampleT     = cuwacunu::camahjucunu::data::observation_sample_t;

    using DataLoaderT = cuwacunu::camahjucunu::data::MemoryMappedDataLoader<DatasetT, SampleT, Td, SamplerT>;
    using EmbeddingT  = cuwacunu::wikimyei::vicreg_4d::VICReg_4D;

public:
    /* -------- 1) direct-ownership ctor ----------------------------- */
    observation_space_t(std::unique_ptr<DataLoaderT> dl,
                        std::unique_ptr<EmbeddingT> model)
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
                ::observation_pipeline_sequential_mm_dataloader<Td>(instrument);

        dl_ = std::make_unique<DataLoaderT>(std::move(loader));

        /* VICReg_4D via its (C,T,D) delegating ctor ----------------- */
        model_ = std::make_unique<EmbeddingT>(dl_->C_, dl_->T_, dl_->D_);

        initialise();
    }

    /* ------------------------------------------------- iteration --- */
    bool step()
    {
        if (done_) return false;

        auto maybe_batch = dl_->next();
        if (!maybe_batch.has_value()) { done_ = true; return false; }

        /* collate, move to device, encode --------------------------- */
        curr_sample_ = SampleT::collate_fn(*maybe_batch);
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
    [[nodiscard]] const SampleT& observation()    const { return curr_sample_; }
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
    std::unique_ptr<DataLoaderT> dl_;
    std::unique_ptr<EmbeddingT>  model_;
    torch::Device                device_;

    SampleT       curr_sample_{};
    torch::Tensor data_, mask_, encoded_;
    bool          done_{false};
};

} /* namespace capital_alocation_strategy */
} /* namespace wikimyei */
} /* namespace cuwacunu */
