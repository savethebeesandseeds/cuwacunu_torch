/* ts2vec.cpp */
#include "wikimyei/heuristics/ts2vec/ts2vec.h"

namespace cuwacunu {
namespace wikimyei {
namespace ts2vec {
// ---------------------------------------------
// Constructor
// ---------------------------------------------
TS2Vec::TS2Vec(
    int input_dims_,
    int output_dims_,
    int hidden_dims_,
    int depth_,
    torch::Device device_,
    double lr_,
    int batch_size_,
    std::optional<int> max_train_length_,
    int temporal_unit_,
    std::string encoder_mask_mode_,
    bool enable_buffer_averaging_
) : device(device_),
    lr(lr_),
    batch_size(batch_size_),
    max_train_length(max_train_length_),
    temporal_unit(temporal_unit_),
    // Initialize the base TSEncoder
    _net(register_module("_net", 
        cuwacunu::wikimyei::ts2vec::TSEncoder(
            input_dims_, 
            output_dims_, 
            hidden_dims_, 
            depth_, 
            encoder_mask_mode_
        )
    )),
    // Create the SWA model (averaged copy of _net); "true" means we can do buffer averaging if desired
    _swa_net(register_module("_swa_net",
        cuwacunu::wikimyei::ts2vec::AveragedTSEncoder(
            _net,
            device,
            enable_buffer_averaging_
        )
    )),
    // Setup the optimizer for the base model
    optimizer(_net->parameters(), torch::optim::AdamWOptions(lr_))
{
    // Move the base model to the device
    _net->to(device);
    // By default, the SWA model is already moved to the device in its constructor
}

// ---------------------------------------------
// fit
// ---------------------------------------------
std::vector<double> TS2Vec::fit(
    torch::Tensor train_data,
    int n_epochs,
    int n_iters,
    bool verbose
) {
    // Ensure the input is 3D
    TORCH_CHECK(train_data.dim() == 3, "train_data must be 3-dimensional (N,T,C).");

    // 1) Possibly split if a maximum length is set
    if (max_train_length.has_value()) {
        int64_t sections = train_data.size(1) / max_train_length.value();
        // If each sample is longer than max_train_length, we can split it
        if (sections >= 2) {
            // This utility presumably splits along dim=1, returns concatenated results
            train_data = split_with_nan(train_data, sections, /*dim=*/1);
        }
    }
    // 2) Centerize if the first or last timesteps are entirely NaN
    {
        auto temporal_missing = torch::isnan(train_data).all(-1).any(0);
        if (temporal_missing[0].item<bool>() ||
        temporal_missing[temporal_missing.size(0)-1].item<bool>()) {
            train_data = centerize_vary_length_series(train_data);
        }
    }
    
    // 3) Remove all‐NaN samples
    {
        auto sample_nan_mask = torch::isnan(train_data).all(/*dim2=*/2).all(/*dim1=*/1);
        train_data = train_data.index({~sample_nan_mask});
    }
    

    // 4) Build dataset & DataLoader
    //    "TensorDataset" → "Stack transform" → random sampler → DataLoader
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

    
    // 5) Training loop
    int epoch_count = 0;
    int iter_count = 0;
    bool stop_loop = false;
    std::vector<double> loss_log;
    
    // Set base net & SWA net to training mode
    _net->train();
    _swa_net->encoder().train();
    
    while (!stop_loop) {
        if (n_epochs >= 0 && epoch_count >= n_epochs) {
            break;
        }
        

        double cum_loss = 0.0;
        int epoch_iters = 0;

        for (auto& batch : *dataloader) {
            // If iteration limit is reached
            if (n_iters >= 0 && iter_count >= n_iters) {
                stop_loop = true;
                break;
            }

            // Prepare input batch
            auto x = batch.data.to(device);
            optimizer.zero_grad();

            // Optionally do random cropping if x.size(1) > max_train_length, etc.
            // (Below is your typical random sampling for the hierarchical contrastive loss)

            // We'll sample some cropping boundaries
            int64_t ts_l = x.size(1);
            int64_t min_crop = (1 << (temporal_unit + 1));
            int64_t crop_l = torch::randint(min_crop, ts_l + 1, {1}).item<int64_t>();
            int64_t crop_left = torch::randint(ts_l - crop_l + 1, {1}).item<int64_t>();
            int64_t crop_right = crop_left + crop_l;

            // Possibly define other offsets
            // e.g. crop_eleft, crop_eright, etc. 
            // The code can vary, but let's keep it consistent with your snippet:

            int64_t crop_eleft = torch::randint(crop_left + 1, {1}).item<int64_t>();
            int64_t crop_eright = torch::randint(crop_right, ts_l + 1, {1}).item<int64_t>();

            auto crop_offset = torch::randint(
                -crop_eleft,
                ts_l - crop_eright + 1,
                {x.size(0)},
                torch::TensorOptions().dtype(torch::kLong).device(device)
            );

            // 6) Forward pass for out1
            auto slice1 = take_per_row(x, crop_offset + crop_eleft, crop_right - crop_eleft);
            auto out1 = _net->forward(slice1);

            // Now replicate the Python slicing: out1 = out1[:, -crop_l:]
            // We want the last crop_l frames
            int64_t T1 = out1.size(1);
            int64_t start_pos_1 = T1 - crop_l;
            out1 = out1.slice(/*dim=*/1, /*start=*/start_pos_1, /*end=*/T1);

            // 7) Forward pass for out2
            auto slice2 = take_per_row(x, crop_offset + crop_left, crop_eright - crop_left);
            auto out2 = _net->forward(slice2);

            // out2 = out2[:, :crop_l]
            int64_t T2 = out2.size(1);
            // but we only want up to crop_l
            out2 = out2.slice(/*dim=*/1, /*start=*/0, /*end=*/std::min<int64_t>(crop_l, T2));

            // 8) Compute your hierarchical contrastive loss
            auto loss = hierarchical_contrastive_loss(out1, out2, temporal_unit);
            loss.backward();
            optimizer.step();

            // 9) Update SWA parameters
            _swa_net->update_parameters(_net);

            // Accumulate loss
            cum_loss += loss.item<double>();
            epoch_iters++;
            iter_count++;
        } // end for dataloader

        
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

// ---------------------------------------------
// _eval_with_pooling (Optional helper)
// ---------------------------------------------
torch::Tensor TS2Vec::_eval_with_pooling(
    torch::Tensor x,
    const std::string& mask,
    std::optional<std::string> encoding_window
) {
    // In practice, you can do special masking or partial forward passes
    // Here we simply do the forward pass with the SWA model:
    auto out = _swa_net->forward(x.to(device), mask);

    // If encoding_window == "full_series", do a global max pool over time
    if (encoding_window && *encoding_window == "full_series") {
        // out shape is (N, T, C)
        out = torch::nn::functional::max_pool1d(
            out.transpose(1, 2),
            torch::nn::functional::MaxPool1dFuncOptions(out.size(1))
        ).transpose(1, 2);
    }
    return out.cpu();
}


// ---------------------------------------------
// encode
// ---------------------------------------------
torch::Tensor TS2Vec::encode(
    torch::Tensor data,
    std::string mask,
    std::optional<std::string> encoding_window,
    bool causal,
    std::optional<int> sliding_length,
    int sliding_padding,
    std::optional<int> batch_size_
) {
    // Put the SWA/EMA model in eval mode for inference
    bool was_training = _swa_net->encoder().is_training();
    _swa_net->encoder().eval();

    // Build a DataLoader
    auto dataset = torch::data::datasets::TensorDataset(data)
        .map(torch::data::transforms::Stack<torch::data::Example<torch::Tensor, void>>());

    auto ds_size = dataset.size().value();
    int eff_batch_size = batch_size_.has_value() ?
                         batch_size_.value() : batch_size;
    torch::data::samplers::SequentialSampler sampler(ds_size);
    torch::data::DataLoaderOptions loader_options(eff_batch_size);

    auto dataloader = torch::data::make_data_loader(
        std::move(dataset),
        sampler,
        loader_options
    );

    std::vector<torch::Tensor> outputs;
    {
        // no_grad during inference
        torch::NoGradGuard no_grad;

        for (auto& batch : *dataloader) {
            auto x = batch.data; // shape (B, T, C)

            // If you do sliding window logic, implement it here. Otherwise:
            // Just call the helper to do forward + optional pooling
            auto out = _eval_with_pooling(x, mask, encoding_window);
            // Possibly handle 'causal', 'sliding_length', etc.
            outputs.push_back(out);
        }
    }

    // Restore the training state if necessary
    if (was_training) {
        _swa_net->encoder().train();
    }

    // Merge results
    return torch::cat(outputs, /*dim=*/0);
}

/**
* ----------------------------------------------------
* save / load
* ----------------------------------------------------
*/
// void TS2Vec::save(const std::string& filepath) {
//    // Save just the SWA model's state_dict
//    auto sd = _swa_net->module().state_dict();
//    torch::save(sd, filepath);
// }

// void TS2Vec::load(const std::string& filepath) {
//    torch::OrderedDict<std::string, torch::Tensor> sd;
//    torch::load(sd, filepath);
//    _swa_net->module().load_state_dict(sd);
//    _swa_net->to(device);
// }

} // namespace ts2vec
} // namespace wikimyei
} // namespace cuwacunu

