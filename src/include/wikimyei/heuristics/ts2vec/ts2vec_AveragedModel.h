/* ts2vec_AveragedModel.h */
#pragma once

#include <torch/torch.h>
#include <memory>
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept> // For runtime_error

#include "wikimyei/heuristics/ts2vec/encoder.h"

namespace cuwacunu {
namespace wikimyei {
namespace ts2vec {

/*
 * A C++ "AveragedModel" that parallels torch.optim.swa_utils.AveragedModel
 * from Python. It holds:
 *   - an internal copy of TSEncoder (averaged_encoder_)
 *   - a buffer n_averaged_ to track how many updates we've done
 *   - an optional bool enable_buffer_averaging_ to decide whether we average buffers or just copy them
 */
class AveragedTSEncoderImpl : public torch::nn::Module {
public:
    // --- SIMPLIFIED CONSTRUCTOR ---
    // No longer needs config args, just the source model to clone
    explicit AveragedTSEncoderImpl(
        const TSEncoder& source_encoder_wrapper, // Pass the source wrapper
        torch::Device device,
        bool enable_buffer_averaging = false                 // Default from original code
    )
        : device_(device),
          enable_buffer_averaging_(enable_buffer_averaging)
    {
        // --- START: Clone Logic ---

        // 1. Call clone() on the source wrapper.
        std::shared_ptr<torch::nn::Module> cloned_base_ptr = source_encoder_wrapper->clone();
        TORCH_CHECK(cloned_base_ptr, "source_encoder_wrapper->clone() returned null!");

        // 2. Downcast the base pointer to the specific Impl pointer.
        std::shared_ptr<TSEncoderImpl> cloned_impl_ptr =
            std::dynamic_pointer_cast<TSEncoderImpl>(cloned_base_ptr);
        TORCH_CHECK(cloned_impl_ptr, "dynamic_pointer_cast to TSEncoderImpl failed after clone()!");

        // 3. Create the wrapper for the successfully cloned Impl object.
        TSEncoder cloned_encoder_wrapper(cloned_impl_ptr);

        // 4. Register the NEW *WRAPPER* as the submodule.
        //    This holds our independent, cloned internal encoder.
        averaged_encoder_ = register_module(
            "averaged_encoder",
            cloned_encoder_wrapper
        );

        // --- END: Clone Logic ---


        // 5. Move the registered module (wrapper) to the correct device.
        averaged_encoder_->to(device_);

        // 6. Register n_averaged_ buffer.
        n_averaged_ = register_buffer(
            "n_averaged_",
            torch::zeros({1}, torch::dtype(torch::kLong).device(device_))
        );
    }

    // --- update_parameters ---
    // Accepts the SOURCE module wrapper. Uses standard -> access now.
    void update_parameters(const TSEncoder& source_encoder_wrapper) {
         torch::NoGradGuard no_grad;

         // Use standard wrapper->method() access
         auto src_params = source_encoder_wrapper->named_parameters();
         auto avg_params = averaged_encoder_->named_parameters();

         int64_t count = n_averaged_.item<int64_t>();

         // Parameter averaging logic
         if (count == 0) {
             for (const auto& item : src_params) {
                 const auto& name = item.key();
                 if(avg_params.contains(name)) {
                     avg_params[name].copy_(item.value().to(avg_params[name].device()));
                 } else {
                      std::cerr << "Warning: Parameter '" << name << "' not found in averaged model during initial copy." << std::endl;
                 }
             }
         } else {
             for (const auto& item : src_params) {
                 const auto& name = item.key();
                 if(avg_params.contains(name)) {
                     auto& avg_tensor = avg_params[name];
                     auto src_tensor_converted = item.value().to(avg_tensor.device());
                     avg_tensor.mul_(double(count) / double(count + 1))
                               .add_(src_tensor_converted / double(count + 1));
                 } else {
                      std::cerr << "Warning: Parameter '" << name << "' not found in averaged model during update." << std::endl;
                 }
             }
         }

         // --- Buffer Handling ---
         // Use standard wrapper->method() access
         auto src_buffers = source_encoder_wrapper->named_buffers();
         auto avg_buffers = averaged_encoder_->named_buffers();

         // Buffer averaging/copying logic
          if (enable_buffer_averaging_) {
            if (count == 0) {
                 for (const auto& buf_item : src_buffers) {
                      const auto& name = buf_item.key();
                      if (avg_buffers.contains(name) && name != "n_averaged_") {
                           avg_buffers[name].copy_(buf_item.value().to(avg_buffers[name].device()));
                      }
                 }
             } else {
                 for (const auto& buf_item : src_buffers) {
                     const auto& name = buf_item.key();
                      if (avg_buffers.contains(name) && name != "n_averaged_") {
                          auto& avg_buf = avg_buffers[name];
                          auto src_buf_converted = buf_item.value().to(avg_buf.device());
                           avg_buf.mul_(double(count) / double(count + 1))
                                  .add_(src_buf_converted / double(count + 1));
                     }
                 }
             }
         } else { // Just copy buffers if not averaging them
             for (const auto& buf_item : src_buffers) {
                 const auto& name = buf_item.key();
                 if (avg_buffers.contains(name) && name != "n_averaged_") {
                     avg_buffers[name].copy_(buf_item.value().to(avg_buffers[name].device()));
                 }
             }
         }

         n_averaged_ += 1; // Increment count
     }

    // --- Forward pass (remains the same) ---
    torch::Tensor forward(const torch::Tensor& x,
                          c10::optional<std::string> mask_opt = c10::nullopt)
    {
        // Calls forward on the internal averaged_encoder_ wrapper -> Impl
        return averaged_encoder_->forward(x, mask_opt);
    }

    // --- Accessor for the underlying implementation (remains the same) ---
    TSEncoderImpl& encoder() {
        return *averaged_encoder_; // operator* still gives Impl&
    }
    const TSEncoderImpl& encoder() const {
         return *averaged_encoder_; // const version
    }

private:
    torch::Device device_;
    bool enable_buffer_averaging_;
    torch::Tensor n_averaged_;

    // Store the TORCH_MODULE wrapper for the *cloned* averaged encoder
    TSEncoder averaged_encoder_{nullptr};
};

// Keep the TORCH_MODULE macro for the AveragedTSEncoder wrapper
TORCH_MODULE(AveragedTSEncoder);

} // namespace ts2vec
} // namespace wikimyei
} // namespace cuwacunu