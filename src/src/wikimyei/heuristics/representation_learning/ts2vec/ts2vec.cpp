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
/* _eval_with_pooling */
/* --------------------------------------------- */

/**
 * @brief Evaluates the input time series using the SWA-averaged encoder network
 *        and optionally applies temporal pooling to produce a fixed-size representation.
 *
 * This function is used during inference (e.g., encoding time series) to extract
 * representations using the smoothed `_swa_net`, rather than the raw `_net`.
 *
 * It mirrors the original `_eval_with_pooling` precisely:
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
        /* while ((1 << p) + 1 < out.size(1)) */
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
        /* In, dim=-1 => in C++ with [N, T, C], that is dim=2. */
        out = torch::cat(reprs, /*dim=*/-1);
    }
    else if (encoding_window)
    {
        /* If encoding_window is neither "full_series" nor "multiscale", */
        /* we interpret it as an integer string (same as the the Original 'isinstance(encoding_window, int)' case). */
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

        /* If w is even, remove the last time step (out = out[:, :-1, :] in the Original). */
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
* ----------------------------------------------------
* save / load
* ----------------------------------------------------
*/
/* void TS2Vec::save(const std::string& filepath) { */
/*    Save just the SWA model's state_dict */
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

