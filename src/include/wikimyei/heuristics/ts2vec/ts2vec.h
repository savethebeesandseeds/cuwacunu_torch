/* ts2vec.h */
#pragma once

#include <torch/torch.h>
#include <optional>
#include <vector>
#include "wikimyei/heuristics/ts2vec/encoder.h"
#include "wikimyei/heuristics/ts2vec/losses.h"
#include "wikimyei/heuristics/ts2vec/utils.h"
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
     * @param encoder_mask_mode_: type of mask for the encoder: ["binomial", "continuous", "all_true", "all_false", "mask_last"]
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
        std::optional<int> max_train_length_ = std::nullopt,
        int temporal_unit_ = 0,
        std::string encoder_mask_mode_="binomial",
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
     * encode: run inference with the SWA/EMA model
     * 
     * @param data: a 3D tensor (N, T, C)
     * @param mask: string specifying mask type (for your TSEncoder usage)
     * @param encoding_window: optional string for pooling, e.g. "full_series"
     * @param causal: if true, do not use future context
     * @param sliding_length: optional sliding window length
     * @param sliding_padding: context padding on each side
     * @param batch_size_: optional custom batch size
     * @return a tensor of encoded features
     */
    torch::Tensor encode(
        torch::Tensor data,
        std::string mask = "all_true",
        std::optional<std::string> encoding_window = std::nullopt,
        bool causal = false,
        std::optional<int> sliding_length = std::nullopt,
        int sliding_padding = 0,
        std::optional<int> batch_size_ = std::nullopt
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
    // device, training hyperparams
    torch::Device device;
    double lr;
    int batch_size;
    std::optional<int> max_train_length;
    int temporal_unit;
    std::string encoder_mask_mode;

    // The base TSEncoder (trainable model)
    TSEncoder _net{nullptr};

    // The SWA/EMA version of the TSEncoder
    // using our custom AveragedTSEncoder
    AveragedTSEncoder _swa_net{nullptr};

    // AdamW optimizer for the base TSEncoder
    torch::optim::AdamW optimizer;

    // (Optional) helper function if you do specialized pooling or mask logic
    torch::Tensor _eval_with_pooling(
        torch::Tensor x,
        const std::string& mask,
        std::optional<std::string> encoding_window
    );
};

} // namespace ts2vec
} // namespace wikimyei
} // namespace cuwacunu
