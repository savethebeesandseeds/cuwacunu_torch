/* ts2vec.h */
#pragma once

#include <torch/torch.h>
#include <optional>
#include <vector>
#include "wikimyei/heuristics/ts2vec/ts2vec_encoder.h"
#include "wikimyei/heuristics/ts2vec/ts2vec_losses.h"
#include "wikimyei/heuristics/ts2vec/ts2vect_utils.h"
#include "wikimyei/heuristics/ts2vec/ts2vec_AveragedModel.h"


THROW_COMPILE_TIME_ERROR("(ts2vec.h)[] TS2VEC Is almost done, but was deprecated for the input tensor is rank 4 and ts2vec works on rank 3 tensors.\n");

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

namespace cuwacunu {
namespace wikimyei {
namespace ts2vec {
    
/*
 * TS2Vec: A C++ implementation of the TS2Vec model, paralleling
 *   - a base TSEncoder (_net) 
 *   - an AveragedTSEncoder (_swa_net) that tracks SWA or EMA of _net
 *   - a training routine fit(...) 
 *   - an inference routine encode(...) 
 *   - optional save(...) and load(...) for the averaged model
 */
class TS2Vec : public torch::nn::Module {
public:
    /**
     * Constructor
     *
     * @param input_dims_: number of input features
     * @param output_dims_: dimension of the learned representation
     * @param hidden_dims_: dimension of hidden layers in TSEncoder
     * @param depth_: number of residual blocks in TSEncoder
     * @param device_: the device for training/inference (CPU/GPU)
     * @param lr_: learning rate
     * @param batch_size_: batch size
     * @param max_train_length_: optional maximum training length for each sample
     * @param temporal_unit_: smallest time resolution unit used in hierarchical contrast
     * @param default_encoder_mask_mode_: type of mask for the encoder: ["binomial", "continious", "all_true", "all_false", "mask_last"]
     * @param pad_mask_: Optional binary mask of shape [T, C] indicating padded or invalid positions.
     * @param enable_buffer_averaging_: If true, buffers (e.g., BatchNorm running stats) in the averaged model 
                                        (_swa_net) are updated using the same averaging formula as parameters. 
                                        If false (default), buffers are directly copied from the training model 
                                        (_net) at each update step (standard SWA usually requires a separate 
                                        BatchNorm update step after training).
    */
    TS2Vec(
        int input_dims_,
        int output_dims_ = 320,
        int hidden_dims_ = 64,
        int depth_ = 10,
        torch::Device device_ = torch::kCUDA,
        double lr_ = 0.001,
        int batch_size_ = 16,
        c10::optional<int> max_train_length_ = c10::nullopt,
        int temporal_unit_ = 0,
        TSEncoder_MaskMode_e default_encoder_mask_mode_ = TSEncoder_MaskMode_e::Binomial,
        c10::optional<torch::Tensor> pad_mask_ = c10::nullopt, 
        bool enable_buffer_averaging_ = false
    );

    /**
     * @brief Trains the TS2Vec model using hierarchical contrastive learning.
     * 
     * This function performs training over a time series dataset using a contrastive loss
     * computed between two differently cropped views of each input. It supports optional
     * length constraints and preprocessing to handle missing values. During training,
     * an SWA-averaged model (`_swa_net`) is updated to improve generalization at inference time.
     * 
     * The training loop samples random crop regions at each iteration to generate diverse
     * views and uses the averaged contrastive loss to optimize the encoder.
     *
     * @tparam Dl The dataloader type. 
     * 
     * @param train_data   A 3D tensor of shape (N, T, C), where:
     *                       - N is the number of series,
     *                       - T is the number of time steps,
     *                       - C is the number of channels/features.
     * @param n_epochs     Number of training epochs to run (set to -1 to ignore).
     * @param n_iters      Maximum number of training iterations (set to -1 to ignore).
     * @param verbose      Whether to print the average loss at each epoch.
     * 
     * @return std::vector<double>  A log of average training losses, one per epoch.
     */
    template<typename Dl>
    std::vector<double> fit(
        Dl& dataloader,
        int n_epochs,
        int n_iters,
        bool verbose
    ) {
        /* Training loop */
        int epoch_count = 0;
        int iter_count = 0;
        bool stop_loop = false;
        std::vector<double> loss_log;
        
        /* Set base net & SWA net to training mode */
        _net->train();
        _swa_net->encoder().train();
        
        while (!stop_loop) {
            if (n_epochs >= 0 && epoch_count >= n_epochs) {
                break;
            }
            
    
            double cum_loss = 0.0;
            int epoch_iters = 0;
    
            for (auto& batch : dataloader.inner()) {
                /* If iteration limit is reached */
                if (n_iters >= 0 && iter_count >= n_iters) {
                    stop_loop = true;
                    break;
                }
    
                /* Prepare input batch */
                auto x = batch.data.to(device);
                optimizer.zero_grad();
    
                /* Optionally do random cropping if x.size(1) > max_train_length, etc. */
                /* (Below is our typical random sampling for the hierarchical contrastive loss) */
    
                /* We'll sample some cropping boundaries */
                int64_t T = x.size(1);
                int64_t min_crop = (1 << (temporal_unit + 1));
                int64_t crop_l = torch::randint(min_crop, T + 1, {1}).item<int64_t>();
                int64_t crop_left = torch::randint(T - crop_l + 1, {1}).item<int64_t>();
                int64_t crop_right = crop_left + crop_l;
    
                /* Possibly define other offsets */
                /* e.g. crop_eleft, crop_eright, etc.  */
                /* The code can vary, but let's keep it consistent with our snippet: */
    
                int64_t crop_eleft = torch::randint(crop_left + 1, {1}).item<int64_t>();
                int64_t crop_eright = torch::randint(crop_right, T + 1, {1}).item<int64_t>();
    
                auto crop_offset = torch::randint(
                    -crop_eleft,
                    T - crop_eright + 1,
                    {x.size(0)},
                    torch::TensorOptions().dtype(torch::kLong).device(device)
                );
    
                /* Forward pass for out1 */
                auto slice1 = take_per_row(x, crop_offset + crop_eleft, crop_right - crop_eleft);
                auto out1 = _net->forward(slice1);
    
                /* slicing: out1 = out1[:, -crop_l:] */
                /* We want the last crop_l frames */
                int64_t T1 = out1.size(1);
                int64_t start_pos_1 = T1 - crop_l;
                out1 = out1.slice(/*dim=*/1, /*start=*/start_pos_1, /*end=*/T1);
    
                /* Forward pass for out2 */
                auto slice2 = take_per_row(x, crop_offset + crop_left, crop_eright - crop_left);
                auto out2 = _net->forward(slice2);
    
                /* out2 = out2[:, :crop_l] */
                int64_t T2 = out2.size(1);
                /* but we only want up to crop_l */
                out2 = out2.slice(/*dim=*/1, /*start=*/0, /*end=*/std::min<int64_t>(crop_l, T2));
    
                /* Compute our hierarchical contrastive loss */
                auto loss = hierarchical_contrastive_loss(out1, out2, temporal_unit);
                loss.backward();
                optimizer.step();
    
                /* Update SWA parameters */
                _swa_net->update_parameters(_net);
    
                /* Accumulate loss */
                cum_loss += loss.template item<double>();
                epoch_iters++;
                iter_count++;
            } /* end for dataloader */
    
            
            if (!stop_loop && epoch_iters > 0) {
                double avg_loss = cum_loss / static_cast<double>(epoch_iters);
                loss_log.push_back(avg_loss);
                
                if (verbose) {
                    std::cout << "[Epoch #" << epoch_count << "] Loss = "
                    << avg_loss << std::endl;
                }
            }
            
            epoch_count++;
        }
    
        return loss_log;
    }

    /**
     * @brief Encodes a batch of time series data using the SWA-averaged model.
     * 
     * This method runs the input data through the encoder in evaluation mode,
     * optionally applying temporal pooling depending on the `encoding_window` setting.
     * Uses a DataLoader for efficient batching and returns the encoded outputs.
     * 
     * @tparam Dl The dataloader type. 
     *
     * @param data             Input tensor of shape (N, T, C).
     * @param mask_mode_overwrite        Masking mode to use during encoding.
     * @param encoding_window  Optional setting for pooling over time (e.g., "full_series", "multiscale", "2", "6", "3"...).
     * @param causal           causal handling (currently unused).
     * @param sliding_padding  Padding to apply if using sliding windows.
     * @param n_samples        Number of samples in the dataset of the dataloader.
     * @param sliding_length   sliding window encoding (currently unused).
     * @param batch_size_      Optional override for batch size during encoding.
     * 
     * @return torch::Tensor   Concatenated tensor of encoded outputs.
     */
    template<typename Dl>
    torch::Tensor encode(
        Dl& dataloader,
        c10::optional<TSEncoder_MaskMode_e> mask_mode_overwrite,
        c10::optional<std::string> encoding_window,
        bool causal,
        int sliding_padding,
        c10::optional<int> n_samples,
        c10::optional<int> sliding_length,
        c10::optional<int> batch_size_
    ) {
        // Store current training state and set encoder to eval
        bool was_training = _swa_net->encoder().is_training();
        _swa_net->encoder().eval();

        // Decide on batch size
        int eff_batch_size = batch_size_.has_value() ? batch_size_.value() : batch_size;

        // Prepare a vector to collect outputs from each batch
        std::vector<torch::Tensor> outputs;

        {
            // Disable grad during inference
            torch::NoGradGuard no_grad;

            // Loop over DataLoader
            for (auto& batch : dataloader.inner())
            {
                // batch.data has shape (B, T, C)
                auto x = batch.data.to(device);
                int64_t B = x.size(0);
                int64_t T = x.size(1);
                // int64_t C = x.size(2);

                // If no sliding window is requested, do a single pass
                if (!sliding_length.has_value()) {
                    // => call our existing helper
                    auto out = _eval_with_pooling(x, mask_mode_overwrite, encoding_window);

                    // if encoding_window == 'full_series', out = out.squeeze(1).
                    // Because our `_eval_with_pooling` already returns shape [B, 1, C] after "full_series",
                    // we replicate the `.squeeze(1)` step here:
                    if (encoding_window.has_value() && encoding_window.value() == "full_series") {
                        // If `_eval_with_pooling` ends with shape (B, 1, C),
                        // then out.squeeze(1) => (B, C).
                        out = out.squeeze(1);
                    }
                    outputs.push_back(out);
                }
                else {
                    // --------------- Sliding window path ---------------
                    TORCH_CHECK(n_samples.has_value(), "When sliding_length is given n_samples argument must also have a value");

                    int s_len = sliding_length.value();
                    // We'll gather the partial outputs in 'reprs' for each sub-slice
                    std::vector<torch::Tensor> reprs;

                    // if n_samples < batch_size, accumulate slices
                    // until we can form a full batch pass. Then split the result back.
                    // We'll replicate that logic:
                    bool do_accumulate = (n_samples.value() < eff_batch_size);

                    std::vector<torch::Tensor> calc_buffer;
                    int64_t calc_buffer_count = 0; // # sequences so far in buffer

                    // Slide over [0..T..s_len]
                    // for i in range(0, T, sliding_length):
                    for (int64_t i = 0; i < T; i += s_len) {
                        // left = i - sliding_padding
                        // right = i + sliding_length + (sliding_padding if not causal else 0)
                        int64_t left  = i - sliding_padding;
                        int64_t right = i + s_len + (causal ? 0 : sliding_padding);

                        // clamp [left, right] to [0, T]
                        int64_t clamped_left  = std::max<int64_t>(left, 0);
                        int64_t clamped_right = std::min<int64_t>(right, T);

                        // x[:, clamped_left:clamped_right, :]
                        auto x_slice = x.index({
                            torch::indexing::Slice(),
                            torch::indexing::Slice(clamped_left, clamped_right),
                            torch::indexing::Slice()
                        });

                        //   pad_left  = -left if left<0 else 0
                        //   pad_right = right - T if right>T else 0
                        int64_t pad_left  = (left < 0) ? -left : 0;
                        int64_t pad_right = (right > T) ? (right - T) : 0;

                        // total length after padding
                        int64_t total_len = x_slice.size(1) + pad_left + pad_right;

                        // We'll create a new (B, total_len, features) tensor filled with NaN,
                        // then copy x_slice into the center region:
                        auto x_sliding = torch::full(
                            {B, total_len, x_slice.size(2)},
                            std::numeric_limits<float>::quiet_NaN(),
                            x_slice.options()
                        );
                        // place the slice in [pad_left, pad_left + slice_len)
                        int64_t slice_len = x_slice.size(1);
                        x_sliding.index_put_({
                            torch::indexing::Slice(),
                            torch::indexing::Slice(pad_left, pad_left + slice_len),
                            torch::indexing::Slice()
                        }, x_slice);

                        // Now x_sliding is our padded slice
                        if (do_accumulate) {
                            // Accumulate in calc_buffer
                            calc_buffer.push_back(x_sliding);
                            calc_buffer_count += B;

                            // If we've reached or exceeded the desired batch size:
                            if (calc_buffer_count >= eff_batch_size) {
                                // combine into one big batch
                                auto big_input = torch::cat(calc_buffer, /*dim=*/0); // shape (calc_buffer_count, total_len, C)
                                calc_buffer.clear();
                                calc_buffer_count = 0;

                                // Now we call _eval_with_pooling, but we only want to keep
                                // the center portion [sliding_padding, sliding_padding+s_len)
                                // from the time dimension. 
                                // Our `_eval_with_pooling` takes an optional pair for slicing:
                                auto out = _eval_with_pooling(
                                    big_input,
                                    mask_mode_overwrite,
                                    encoding_window,
                                    c10::optional<std::pair<int, int>>{
                                        {sliding_padding, sliding_padding + s_len}
                                    }
                                );

                                // out is shape (calc_buffer_count, ?, D)
                                // but we just split it back into sub-chunks of size B
                                // so each chunk is (B, ?, D)
                                auto splitted = out.split(B, 0);
                                for (auto &s : splitted) {
                                    reprs.push_back(s);
                                }
                            }
                        }
                        else {
                            // If we don't need to accumulate, do a direct pass
                            auto out = _eval_with_pooling(
                                x_sliding,
                                mask_mode_overwrite,
                                encoding_window,
                                c10::optional<std::pair<int, int>>{
                                    {sliding_padding, sliding_padding + s_len}
                                }
                            );
                            reprs.push_back(out);
                        }
                    } // end sliding over time

                    // If there's anything left in the buffer, process it now
                    if (do_accumulate && calc_buffer_count > 0) {
                        auto big_input = torch::cat(calc_buffer, /*dim=*/0);
                        calc_buffer.clear();
                        calc_buffer_count = 0;

                        auto out = _eval_with_pooling(
                            big_input,
                            mask_mode_overwrite,
                            encoding_window,
                            c10::optional<std::pair<int, int>>{
                                {sliding_padding, sliding_padding + s_len}
                            }
                        );

                        // split out => sub-chunks of size B
                        auto splitted = out.split(B, 0);
                        for (auto &s : splitted) {
                            reprs.push_back(s);
                        }
                    }

                    // Now we have many slices in 'reprs' => each is (B, time_chunk, D).
                    // Original code does out = torch.cat(reprs, dim=1) => (B, total_time, D)
                    auto cat_reprs = torch::cat(reprs, /*dim=*/1);

                    // If encoding_window == 'full_series', the Original code does a final max-pool
                    // at the end. But note our _eval_with_pooling also does a pool if "full_series".
                    // The Original code: 
                    //    out = torch.cat(reprs, dim=1)
                    //    if encoding_window == 'full_series':
                    //        out = F.max_pool1d(out.transpose(1,2), kernel_size=out.size(1)).squeeze(1)
                    // => The user’s `_eval_with_pooling(...)` already does the global pool if "full_series",
                    //    but we want to replicate the Original's final step. 
                    // => The Original snippet says "If encoding_window == 'full_series', out = out.squeeze(1)."
                    //    This is because _eval_with_pooling returned shape (B, 1, D). 
                    //    We want (B, D).
                    //
                    // In practice, our _eval_with_pooling will do the pool, returning shape (B, 1, C).
                    // So we do a final .squeeze(1) to match Original's shape.
                    if (encoding_window.has_value() && encoding_window.value() == "full_series") {
                        cat_reprs = cat_reprs.squeeze(1);
                    }

                    outputs.push_back(cat_reprs);
                } // end if sliding
            } // end for dataloader
        }

        // Restore training mode
        if (was_training) {
            _swa_net->encoder().train();
        }

        // Concatenate all batch outputs (along dim=0)
        auto final_out = torch::cat(outputs, /*dim=*/0);

        return final_out;
    }


    /**
     * save: saves the SWA model's state_dict
     */
    void save(const std::string& filepath);

    /**
     * load: loads the SWA model's state_dict
     */
    void load(const std::string& filepath);

public:
    /* device, training hyperparams */
    torch::Device device;
    double lr;
    int batch_size;
    c10::optional<int> max_train_length;
    int temporal_unit;

    /* The base TSEncoder (trainable model) */
    TSEncoder _net{nullptr};

    /* The SWA/EMA version of the TSEncoder */
    /* using our custom AveragedTSEncoder */
    AveragedTSEncoder _swa_net{nullptr};

    /* AdamW optimizer for the base TSEncoder */
    torch::optim::AdamW optimizer;

    /* (Optional) helper function for specialized pooling or mask_mode logic */
    torch::Tensor _eval_with_pooling(
        torch::Tensor x,
        c10::optional<TSEncoder_MaskMode_e> mask_mode_overwrite = c10::nullopt,
        c10::optional<std::string> encoding_window = c10::nullopt,
        c10::optional<std::pair<int,int>> slicing = c10::nullopt
    );
};

} /* namespace ts2vec */
} /* namespace wikimyei */
} /* namespace cuwacunu */
