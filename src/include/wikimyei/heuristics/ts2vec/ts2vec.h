/* ts2vec.h */
#pragma once

#include <torch/torch.h>
#include <optional>
#include <vector>
#include "wikimyei/heuristics/ts2vec/encoder.h"
#include "wikimyei/heuristics/ts2vec/losses.h"
#include "wikimyei/heuristics/ts2vec/ts2vect_utils.h"
#include "wikimyei/heuristics/ts2vec/ts2vec_AveragedModel.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

namespace cuwacunu {
namespace wikimyei {
namespace ts2vec {
    
/*
 * TS2Vec: A C++ implementation of the TS2Vec model, paralleling
 * the Python code snippet. It uses:
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
     * @param default_encoder_mask_mode_: type of mask for the encoder: ["binomial", "continuous", "all_true", "all_false", "mask_last"]
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
     * fit: trains the model
     * 
     * @param train_data: a 3D tensor of shape (N, T, C)
     * @param n_epochs: number of epochs (stop if >= 0)
     * @param n_iters: number of iterations (stop if >= 0)
     * @param verbose: whether to print progress
     * @return a vector of training losses (one per epoch)
     */
    std::vector<double> fit(
        torch::Tensor train_data,
        int n_epochs = -1,
        int n_iters = -1,
        bool verbose = false
    );

    /**
     * @brief Encodes a batch of time series data using the SWA-averaged model.
     * 
     * Runs inference over the input data using the stabilized (SWA/EMA) encoder.
     * Returns either full per-timestep embeddings or a pooled summary, depending
     * on the `encoding_window` parameter.
     * 
     * @param data             A 3D tensor of shape (N, T, C), where:
     *                           - N is the number of samples,
     *                           - T is the number of time steps,
     *                           - C is the number of input channels/features.
     * @param mask_mode_overwrite             A string specifying how the input is masked during encoding,
     *                         e.g., "all_true", "mask_last", etc., as used by the encoder.
     * @param encoding_window  If set to "full_series", applies global max pooling over time,
     *                         returning a single embedding vector per sequence (shape: N × 1 × C).
     *                         If unset or any other value, returns one embedding per timestep (N × T × C).
     * @param causal           If true, restricts encoding to use only past and present context (currently unused).
     * @param sliding_length   If set, enables sliding window encoding over the sequence (not yet implemented).
     * @param sliding_padding  Amount of context (padding) to include on each side when using sliding windows.
     * @param batch_size_      Optional override for the batch size during encoding.
     * 
     * @return torch::Tensor   Encoded tensor of shape (N, T, C) or (N, 1, C), depending on `encoding_window`.
     */
    torch::Tensor encode(
        torch::Tensor data,
        c10::optional<TSEncoder_MaskMode_e> mask_mode_overwrite = TSEncoder_MaskMode_e::AllTrue,
        c10::optional<std::string> encoding_window = c10::nullopt,
        bool causal = false,
        c10::optional<int> sliding_length = c10::nullopt,
        int sliding_padding = 0,
        c10::optional<int> batch_size_ = c10::nullopt
    );

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

    /* (Optional) helper function if you do specialized pooling or mask_mode logic */
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
