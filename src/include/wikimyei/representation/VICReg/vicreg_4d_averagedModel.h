/* vicreg_4d_averagedModel.h */
#pragma once

#include <torch/torch.h>
#include <memory>
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept> /* For runtime_error */

#include "wikimyei/representation/VICReg/vicreg_4d_encoder.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

/*
 * A C++ "AveragedModel" that parallels torch.optim.swa_utils.AveragedModel
 *   - an internal copy of VICReg_4D_Encoder (averaged_encoder_)
 *   - a float32 buffer n_averaged_ to track how many updates we've done
 *   - an optional bool enable_buffer_averaging_ to decide whether we average buffers or just copy them
 */
class StochasticWeightAverage_EncoderImpl : public torch::nn::Module {
public:
    /* Clone the given encoder and optionally enable buffer averaging. */
    explicit StochasticWeightAverage_EncoderImpl(
        const VICReg_4D_Encoder& source_encoder_wrapper,
        bool enable_buffer_averaging = false,
        torch::Dtype dtype_ = torch::kFloat32,
        torch::Device device_ = torch::kCPU
    ) : enable_buffer_averaging_(enable_buffer_averaging),
        dtype(dtype_), device(device_),

        /* averaged_encoder_: build‑move‑return in a lambda */
        averaged_encoder_(register_module(
            "averaged_encoder",
            ([&] {
                auto clone = std::dynamic_pointer_cast<VICReg_4D_EncoderImpl>(source_encoder_wrapper->clone());
                TORCH_CHECK(clone, "[vicreg_4d_AveragedModel.h](StochasticWeightAverage_EncoderImpl) clone() of the source encoder returned null!");
                clone->to(device_, dtype_); 
                return VICReg_4D_Encoder{clone};}
            )()
        )),

        /* buffer born on the right device/dtype */
        n_averaged_(register_buffer(
            "n_averaged_",
            torch::zeros({1},
                torch::TensorOptions().dtype(dtype_).device(device_))))
    {        
        /* move to device and type */
        this->to(device, dtype);
    }

    /* Parameter & (optional) buffer averaging */
    void update_parameters(const VICReg_4D_Encoder& source_encoder_wrapper) {
        torch::NoGradGuard no_grad;

        auto src_params = source_encoder_wrapper->named_parameters();
        auto avg_params = averaged_encoder_->named_parameters();
        int64_t count = n_averaged_.item<int64_t>();

        /* --- Parameter logic --- */
        if (count == 0) {
            for (auto& item : src_params) {
                auto name = item.key();
                if (avg_params.contains(name)) {
                    avg_params[name].copy_(item.value().to(avg_params[name].device()));
                }
            }
        } else {
            for (auto& item : src_params) {
                auto name = item.key();
                if (avg_params.contains(name)) {
                    auto& avg_t = avg_params[name];
                    auto src_t = item.value()
                                    .to(avg_t.dtype())
                                    .to(avg_t.device());
                    auto alpha = torch::full({}, double(count) / (count + 1), avg_t.options());
                    avg_t.mul_(alpha).add_(src_t / double(count + 1));                                    
                }
            }
        }

        /* --- Buffer logic --- */
        auto src_bufs = source_encoder_wrapper->named_buffers();
        auto avg_bufs = averaged_encoder_->named_buffers();

        if (enable_buffer_averaging_) {
            if (count == 0) {
                for (auto& buf_item : src_bufs) {
                    auto name = buf_item.key();
                    if (name != "n_averaged_" && avg_bufs.contains(name))
                        avg_bufs[name].copy_(buf_item.value().to(avg_bufs[name].device()));
                }
            } else {
                for (auto& buf_item : src_bufs) {
                    auto name = buf_item.key();
                    if (name != "n_averaged_" && avg_bufs.contains(name)) {
                        auto& avg_b = avg_bufs[name];
                        auto src_b = buf_item.value().to(avg_b.device());
                        avg_b.mul_(double(count) / (count + 1))
                             .add_(src_b / double(count + 1));
                    }
                }
            }
        } else {
            /* simple copy */
            for (auto& buf_item : src_bufs) {
                auto name = buf_item.key();
                if (name != "n_averaged_" && avg_bufs.contains(name))
                    avg_bufs[name].copy_(buf_item.value().to(avg_bufs[name].device()));
            }
        }

        /* increment counter */
        n_averaged_ += 1;
    }

    /* Forward simply delegates to the averaged encoder */
    torch::Tensor forward(
        const torch::Tensor& x_input,
        c10::optional<torch::Tensor> x_mask = c10::nullopt
    ) {
        return averaged_encoder_->forward(x_input, x_mask);
    }

    /* Access underlying Impl */
    VICReg_4D_EncoderImpl& encoder() {
        return *averaged_encoder_;
    }
    const VICReg_4D_EncoderImpl& encoder() const {
        return *averaged_encoder_;
    }

private:
    bool enable_buffer_averaging_;
    torch::Dtype dtype;
    torch::Device device;
    
    VICReg_4D_Encoder averaged_encoder_{nullptr};
    torch::Tensor n_averaged_;
};

/* Keep the TORCH_MODULE wrapper */
TORCH_MODULE(StochasticWeightAverage_Encoder);

} /* namespace vicreg_4d */
} /* namespace wikimyei */
} /* namespace cuwacunu */
