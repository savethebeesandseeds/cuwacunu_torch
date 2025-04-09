/* ts2vec.cpp */
#include "wikimyei/heuristics/ts2vec/ts2vec.h"

namespace cuwacunu {
namespace wikimyei {
namespace ts2vec {
/* --------------------------------------------- */
/* Constructor */
/* --------------------------------------------- */
TS2Vec::TS2Vec(
    int input_dims_,
    int output_dims_,
    int hidden_dims_,
    int depth_,
    torch::Device device_,
    double lr_,
    int batch_size_,
    c10::optional<int> max_train_length_,
    int temporal_unit_,
    TSEncoder_MaskMode_e default_encoder_mask_mode_,
    c10::optional<torch::Tensor> pad_mask_, 
    bool enable_buffer_averaging_
) : device(device_),
    lr(lr_),
    batch_size(batch_size_),
    max_train_length(max_train_length_),
    temporal_unit(temporal_unit_),
    /* Initialize the base TSEncoder */
    _net(register_module("_net", 
        cuwacunu::wikimyei::ts2vec::TSEncoder(
            input_dims_, 
            output_dims_, 
            hidden_dims_, 
            depth_, 
            default_encoder_mask_mode_,
            pad_mask_
        )
    )),
    /* Create the SWA model (averaged copy of _net); "true" means we can do buffer averaging if desired */
    _swa_net(register_module("_swa_net",
        cuwacunu::wikimyei::ts2vec::AveragedTSEncoder(
            _net,
            device,
            enable_buffer_averaging_
        )
    )),
    /* Setup the optimizer for the base model */
    optimizer(_net->parameters(), torch::optim::AdamWOptions(lr_))
{
    /* Move the base model to the device */
    _net->to(device);
    /* By default, the SWA model is already moved to the device in its constructor */
}

/* --------------------------------------------- */
/* fit */
/* --------------------------------------------- */
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
std::vector<double> TS2Vec::fit(
    torch::Tensor train_data,
    int n_epochs,
    int n_iters,
    bool verbose
) {
    /* Ensure the input is 3D */
    TORCH_CHECK(train_data.dim() == 3, "train_data must be 3-dimensional (N,T,C).");

    /* 1) Possibly split if a maximum length is set */
    if (max_train_length.has_value()) {
        int64_t sections = train_data.size(1) / max_train_length.value();
        /* If each sample is longer than max_train_length, we can split it */
        if (sections >= 2) {
            /* This utility presumably splits along dim=1, returns concatenated results */
            train_data = split_with_nan(train_data, sections, /*dim=*/1);
        }
    }
    /* 2) Centerize if the first or last timesteps are entirely NaN */
    {
        auto temporal_missing = torch::isnan(train_data).all(-1).any(0);
        if (temporal_missing[0].item<bool>() ||
        temporal_missing[temporal_missing.size(0)-1].item<bool>()) {
            train_data = centerize_vary_length_series(train_data);
        }
    }
    
    /* 3) Remove all‐NaN samples */
    {
        auto sample_nan_mask = torch::isnan(train_data).all(/*dim2=*/2).all(/*dim1=*/1);
        train_data = train_data.index({~sample_nan_mask});
    }
    

    /* 4) Build dataset & DataLoader */
    /*    "TensorDataset" → "Stack transform" → random sampler → DataLoader */
    auto dataset = torch::data::datasets::TensorDataset(train_data)
        .map(torch::data::transforms::Stack<torch::data::Example<torch::Tensor, void>>());
    
    auto ds_size = dataset.size().value();
    torch::data::samplers::RandomSampler loader_sampler(ds_size);
    torch::data::DataLoaderOptions loader_options(
        std::min<int64_t>(batch_size, ds_size)
    );
    loader_options.drop_last(true);
    
    auto dataloader = torch::data::make_data_loader(
        std::move(dataset),
        loader_sampler,
        loader_options
    );

    
    /* 5) Training loop */
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

        for (auto& batch : *dataloader) {
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

            /* 6) Forward pass for out1 */
            auto slice1 = take_per_row(x, crop_offset + crop_eleft, crop_right - crop_eleft);
            auto out1 = _net->forward(slice1);

            /* Now replicate the Python slicing: out1 = out1[:, -crop_l:] */
            /* We want the last crop_l frames */
            int64_t T1 = out1.size(1);
            int64_t start_pos_1 = T1 - crop_l;
            out1 = out1.slice(/*dim=*/1, /*start=*/start_pos_1, /*end=*/T1);

            /* 7) Forward pass for out2 */
            auto slice2 = take_per_row(x, crop_offset + crop_left, crop_eright - crop_left);
            auto out2 = _net->forward(slice2);

            /* out2 = out2[:, :crop_l] */
            int64_t T2 = out2.size(1);
            /* but we only want up to crop_l */
            out2 = out2.slice(/*dim=*/1, /*start=*/0, /*end=*/std::min<int64_t>(crop_l, T2));

            /* 8) Compute our hierarchical contrastive loss */
            auto loss = hierarchical_contrastive_loss(out1, out2, temporal_unit);
            loss.backward();
            optimizer.step();

            /* 9) Update SWA parameters */
            _swa_net->update_parameters(_net);

            /* Accumulate loss */
            cum_loss += loss.item<double>();
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

/* --------------------------------------------- */
/* _eval_with_pooling */
/* --------------------------------------------- */

/**
 * @brief Evaluates the input time series using the SWA-averaged encoder network
 *        and optionally applies temporal pooling to produce a fixed-size representation.
 *
 * This function is used during inference (e.g., encoding time series) to extract
 * representations using the smoothed `_swa_net`, rather than the raw `_net`.
 *
 * It mirrors the original Python `_eval_with_pooling` precisely:
 *    1. Runs a forward pass through `_swa_net`.
 *    2. Depending on `encoding_window`:
 *       - "full_series": global max-pool over the entire time dimension.
 *       - "multiscale": repeatedly max-pool with increasing kernel sizes.
 *       - Integer: uses that integer as a kernel_size, with stride=1 and padding=kernel_size//2.
 *         If the kernel_size is even, the last timestep in dimension 1 is removed.
 *       - Else (no window given or unrecognized string): return the raw per-step representations.
 *    3. If `slicing` is provided, it slices the time dimension in `[start, end)`.
 *
 * @param x              A 3D input tensor of shape (N, T, C), where:
 *                         - N = batch size,
 *                         - T = number of timesteps,
 *                         - C = number of channels/features per timestep.
 * @param mask_mode_overwrite      An optional mask mode enum, passed to the SWA encoder’s forward pass.
 * @param encoding_window Optional string controlling how the output is pooled:
 *                         - "full_series": global max pool over time (producing shape N,1,C).
 *                         - "multiscale": multiple max pools with exponentially growing kernel.
 *                         - stringified integer (e.g. "5"): max pool with the specified kernel size.
 *                         - no value (or unrecognized string): no pooling.
 * @param slicing        An optional pair of (start, end) indices to slice the time dimension.
 *
 * @return torch::Tensor A 3D tensor of shape (N, T', C') after pooling and/or slicing,
 *                       where T' depends on the pooling strategy and slicing.
 */
 torch::Tensor TS2Vec::_eval_with_pooling(
    torch::Tensor x,
    c10::optional<TSEncoder_MaskMode_e> mask_mode_overwrite,
    c10::optional<std::string> encoding_window,
    c10::optional<std::pair<int,int>> slicing
) {
    /* 1) Forward pass through the SWA-based encoder. The output is expected to have shape [N, T, C]. */
    /*    Adjust the device usage or forward signature as needed for our environment. */
    auto out = _swa_net->forward(x.to(device), mask_mode_overwrite);

    /* 2) Check encoding_window to decide how (or if) we apply pooling. */
    if (encoding_window && *encoding_window == "full_series")
    {
        /* Global max-pool over the time dimension. */
        /* The output shape is assumed [N, T, C], so transpose to [N, C, T] for max_pool1d. */
        out = torch::nn::functional::max_pool1d(
            out.transpose(1, 2),
            /* kernel_size = out.size(1) => pool over the entire T dimension */
            torch::nn::functional::MaxPool1dFuncOptions(out.size(1))
        ).transpose(1, 2);
    }
    else if (encoding_window && *encoding_window == "multiscale")
    {
        /* Multiscale pooling: repeatedly apply max_pool1d with kernel sizes (2^(p+1) + 1), */
        /* each time with stride=1 and padding=2^p, until the kernel would exceed T. */
        int p = 0;
        std::vector<torch::Tensor> reprs;

        /* The time dimension is out.size(1) in [N, T, C]. */
        /* Replicates the Python: while ((1 << p) + 1 < out.size(1)) */
        while ((1 << p) + 1 < out.size(1)) {
            int kernel_size = (1 << (p + 1)) + 1;
            int pad         = (1 << p);

            /* max_pool1d requires [N, C, T], so we transpose first. */
            auto t_out = torch::nn::functional::max_pool1d(
                out.transpose(1, 2),
                torch::nn::functional::MaxPool1dFuncOptions(kernel_size)
                    .stride(1)
                    .padding(pad)
            ).transpose(1, 2);

            reprs.push_back(t_out);
            p++;
        }

        /* Concatenate the pooled representations along the last dimension. */
        /* In Python, dim=-1 => in C++ with [N, T, C], that is dim=2. */
        out = torch::cat(reprs, /*dim=*/-1);
    }
    else if (encoding_window)
    {
        /* If encoding_window is neither "full_series" nor "multiscale", */
        /* we interpret it as an integer string (same as the Python 'isinstance(encoding_window, int)' case). */
        int w;
        try {
            w = std::stoi(*encoding_window);
        } catch (const std::invalid_argument&) {
            throw std::runtime_error(
                "encoding_window must be 'full_series', 'multiscale', or a valid integer string"
            );
        }

        /* Apply max_pool1d with kernel_size = w, stride=1, and padding=w/2 (floor division). */
        out = torch::nn::functional::max_pool1d(
            out.transpose(1, 2),
            torch::nn::functional::MaxPool1dFuncOptions(w)
                .stride(1)
                .padding(w / 2)
        ).transpose(1, 2);

        /* If w is even, remove the last time step (out = out[:, :-1, :] in Python). */
        if (w % 2 == 0) {
            auto dim_size = out.size(1);
            out = out.index({
                torch::indexing::Slice(),
                torch::indexing::Slice(0, dim_size - 1),
                torch::indexing::Slice()
            });
        }
    }

    if (slicing.has_value()) {
        auto start = slicing->first;
        auto end   = slicing->second;
        out = out.index({
            torch::indexing::Slice(),
            torch::indexing::Slice(start, end),
            torch::indexing::Slice()
        });
    }

    return out;
}


/**
 * @brief Encodes a batch of time series data using the SWA-averaged model.
 * 
 * This method runs the input data through the encoder in evaluation mode,
 * optionally applying temporal pooling depending on the `encoding_window` setting.
 * Uses a DataLoader for efficient batching and returns the encoded outputs.
 * 
 * @param data             Input tensor of shape (N, T, C).
 * @param mask_mode_overwrite        Masking mode to use during encoding.
 * @param encoding_window  Optional setting for pooling over time (e.g., "full_series", "multiscale", "2", "6", "3"...).
 * @param causal           Placeholder for future causal handling (currently unused).
 * @param sliding_length   Placeholder for future sliding window encoding (currently unused).
 * @param sliding_padding  Padding to apply if using sliding windows.
 * @param batch_size_      Optional override for batch size during encoding.
 * 
 * @return torch::Tensor   Concatenated tensor of encoded outputs.
 */
 torch::Tensor TS2Vec::encode(
    torch::Tensor data,
    c10::optional<TSEncoder_MaskMode_e> mask_mode_overwrite,
    c10::optional<std::string> encoding_window,
    bool causal,
    c10::optional<int> sliding_length,
    int sliding_padding,
    c10::optional<int> batch_size_
) {
    // 1) Store current training state and set encoder to eval
    bool was_training = _swa_net->encoder().is_training();
    _swa_net->encoder().eval();

    // 2) Data shape expected: (n_samples, T, C)
    //    In Python: n_samples = data.shape[0], T = data.shape[1]
    auto sizes = data.sizes();
    int64_t n_samples = sizes[0];
    int64_t T      = sizes[1];
    // int64_t C = sizes[2]; // if you need it

    // 3) Decide on batch size
    int eff_batch_size = batch_size_.has_value() ? batch_size_.value() : batch_size;

    // 4) Build a DataLoader
    auto dataset = torch::data::datasets::TensorDataset(data)
        .map(torch::data::transforms::Stack<torch::data::Example<torch::Tensor, void>>());
    auto ds_size = dataset.size().value();
    torch::data::samplers::SequentialSampler sampler(ds_size);
    torch::data::DataLoaderOptions loader_opts(eff_batch_size);
    auto dataloader = torch::data::make_data_loader(std::move(dataset), sampler, loader_opts);

    // 5) Prepare a vector to collect outputs from each batch
    std::vector<torch::Tensor> outputs;

    {
        // Disable grad during inference
        torch::NoGradGuard no_grad;

        // 6) Loop over DataLoader
        for (auto& batch : *dataloader)
        {
            // batch.data has shape (B, T, C)
            auto x = batch.data.to(device);
            int64_t B = x.size(0);

            // If no sliding window is requested, do a single pass
            if (!sliding_length.has_value()) {
                // => call our existing helper
                auto out = _eval_with_pooling(x, mask_mode_overwrite, encoding_window);

                // In Python: if encoding_window == 'full_series', out = out.squeeze(1).
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

                int s_len = sliding_length.value();
                // We'll gather the partial outputs in 'reprs' for each sub-slice
                std::vector<torch::Tensor> reprs;

                // Python approach: if n_samples < batch_size, accumulate slices
                // until we can form a full batch pass. Then split the result back.
                // We'll replicate that logic:
                bool do_accumulate = (n_samples < eff_batch_size);

                std::vector<torch::Tensor> calc_buffer;
                int64_t calc_buffer_count = 0; // # sequences so far in buffer

                // Slide over [0..T..s_len]
                // Python: for i in range(0, T, sliding_length):
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

                    // In Python, they do "torch_pad_nan" to pad left/right. We'll do the same:
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
                            // from the time dimension. The Python code does slicing=slice(...).
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
                // Python code does out = torch.cat(reprs, dim=1) => (B, total_time, D)
                auto cat_reprs = torch::cat(reprs, /*dim=*/1);

                // If encoding_window == 'full_series', the Python code does a final max-pool
                // at the end. But note our _eval_with_pooling also does a pool if "full_series".
                // The Python code: 
                //    out = torch.cat(reprs, dim=1)
                //    if encoding_window == 'full_series':
                //        out = F.max_pool1d(out.transpose(1,2), kernel_size=out.size(1)).squeeze(1)
                // => The user’s `_eval_with_pooling(...)` already does the global pool if "full_series",
                //    but we want to replicate the Python's final step. 
                // => The python snippet says "If encoding_window == 'full_series', out = out.squeeze(1)."
                //    This is because _eval_with_pooling returned shape (B, 1, D). 
                //    We want (B, D).
                //
                // In practice, our _eval_with_pooling will do the pool, returning shape (B, 1, C).
                // So we do a final .squeeze(1) to match Python's shape.
                if (encoding_window.has_value() && encoding_window.value() == "full_series") {
                    cat_reprs = cat_reprs.squeeze(1);
                }

                outputs.push_back(cat_reprs);
            } // end if sliding
        } // end for dataloader
    }

    // 7) Restore training mode
    if (was_training) {
        _swa_net->encoder().train();
    }

    // 8) Concatenate all batch outputs (along dim=0)
    auto final_out = torch::cat(outputs, /*dim=*/0);

    // Python returns output.numpy(). If you want to produce a CPU float32 tensor:
    //   final_out = final_out.to(torch::kCPU);
    //   final_out = final_out.to(torch::kFloat);
    // 
    // but if our environment is purely Torch C++, you likely just return the Tensor as is:
    return final_out;
}

/**
* ----------------------------------------------------
* save / load
* ----------------------------------------------------
*/
/* void TS2Vec::save(const std::string& filepath) { */
/*    Save just the SWA model's state_dict 
/*    auto sd = _swa_net->module().state_dict(); */
/*    torch::save(sd, filepath); */
/* } */

/* void TS2Vec::load(const std::string& filepath) { */
/*    torch::OrderedDict<std::string, torch::Tensor> sd; */
/*    torch::load(sd, filepath); */
/*    _swa_net->module().load_state_dict(sd); */
/*    _swa_net->to(device); */
/* } */

} /* namespace ts2vec */
} /* namespace wikimyei */
} /* namespace cuwacunu */

