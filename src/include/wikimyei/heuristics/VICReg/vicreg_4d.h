/* vicreg_4d.h */

#pragma once

#include <torch/torch.h>
#include <optional>
#include <vector>
#include "wikimyei/heuristics/VICReg/vicreg_4d_encoder.h"
#include "wikimyei/heuristics/VICReg/vicreg_4d_losses.h"
#include "wikimyei/heuristics/VICReg/vicreg_4d_Projector.h"
#include "wikimyei/heuristics/VICReg/vicreg_4d_AveragedModel.h"
#include "wikimyei/heuristics/VICReg/vicreg_4d_Augmentations.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {
    
/*
 * VICReg_4D: A C++ implementation of the VICReg model, expanded for rank 3 tensor inputs + batch = rank 4 tensors, paralleling
 *  invariance, variance, and covariance regularization
 *   - a base VICReg_4D_Encoder (_encoder_net) 
 *   - an StochasticWeightAverage_Encoder (_swa_encoder_net) that tracks SWA or EMA of _encoder_net
 *   - a training routine fit(...) 
 *   - an inference routine encode(...) 
 *   - optional save(...) and load(...) for the averaged model
 */
class VICReg_4D : public torch::nn::Module {
public:
    /* Class Variables */
    int C; // n channels
    int T; // n timesteps
    int D; // n features
    double lr;
    double sim_coeff;
    double std_coeff;
    double cov_coeff;
    torch::Dtype dtype;
    torch::Device device;


    /* The base VICReg_4D_Encoder (trainable model) */
    VICReg_4D_Encoder _encoder_net{nullptr};

    /* The SWA/EMA version of the VICReg_4D_Encoder */
    StochasticWeightAverage_Encoder _swa_encoder_net{nullptr};

    /* The Projector from representation to optimizacion lattice */
    VICReg_4D_Projector _projector_net{nullptr};

    /* The aug module, for sample viriances in selfsupervised training */
    VICReg_4D_Augmentation aug;

    /* AdamW optimizer for the networks */
    torch::optim::AdamW optimizer;

    /**
     * Constructor
     *
     * @param C_: number of channels in the input
     * @param T_: number of timesteps in the input
     * @param D_: number of features or dimensionality of the input
     * @param encoding_dims_: dimension of the learned representation
     * @param encoder_hidden_dims_: dimension of hidden layers in VICReg_4D_Encoder
     * @param encoder_depth_: number of residual blocks in VICReg_4D_Encoder
     * @param device_: the device for training/inference (CPU/GPU)
     * @param lr_: learning rate
     * @param sim_coeff: Weight for the invariance (MSE) loss component.
     * @param std_coeff: Weight for the variance regularization component.
     * @param cov_coeff: Weight for the covariance regularization component.
     * @param batch_size_: batch size
     * @param enable_buffer_averaging_: If true, buffers (e.g., BatchNorm running stats) in the averaged model 
                                        (_swa_encoder_net) are updated using the same averaging formula as parameters. 
                                        If false (default), buffers are directly copied from the training model 
                                        (_encoder_net) at each update step (standard SWA usually requires a separate 
                                        BatchNorm update step after training).
    */
    VICReg_4D(
        int C_, // n channels
        int T_, // n timesteps
        int D_, // n features
        int encoding_dims_ = 320,
        int channel_expansion_dim_ = 64,
        int fused_feature_dim_ = 128,
        int encoder_hidden_dims_ = 64,
        int encoder_depth_ = 10,
        std::string projector_mlp_spec_ = "8192-8192-8192", // layer specification for the projector
        double sim_coeff_ = 25.0,   // Invariance loss coeficient
        double std_coeff_ = 25.0,   // Variance loss coeficient
        double cov_coeff_ = 1.0,    // Covriance loss coeficient
        double lr_ = 0.001,
        torch::Dtype dtype_ = torch::kFloat32,
        torch::Device device_ = torch::kCPU,
        bool enable_buffer_averaging_ = false
    ) : 
        C(C_),
        T(T_),
        D(D_),
        lr(lr_),
        sim_coeff(sim_coeff_),
        std_coeff(std_coeff_),
        cov_coeff(cov_coeff_),
        dtype(dtype_),
        device(device_),
        /* Initialize the base VICReg_4D_Encoder */
        _encoder_net(register_module("_encoder_net", 
            cuwacunu::wikimyei::vicreg_4d::VICReg_4D_Encoder(
                C,
                T,
                D,
                encoding_dims_,
                channel_expansion_dim_,
                fused_feature_dim_,
                encoder_hidden_dims_,
                encoder_depth_,
                dtype_,
                device_
            )
        )),
        /* Create the SWA model (averaged copy of _encoder_net); "true" means we can do buffer averaging if desired */
        _swa_encoder_net(register_module("_swa_encoder_net",
            cuwacunu::wikimyei::vicreg_4d::StochasticWeightAverage_Encoder(
                _encoder_net,
                enable_buffer_averaging_,
                dtype_,
                device_
            )
        )),
        
        /* Create the projector model */
        _projector_net(register_module("_projector_net",
            cuwacunu::wikimyei::vicreg_4d::VICReg_4D_Projector(
                encoding_dims_,
                projector_mlp_spec_,
                dtype_,
                device_
            )
        )),

        /* set up the aug */
        aug{},
        
        /* ---------- set up the optimizer ------------- */
        optimizer(
            std::vector<torch::optim::OptimizerParamGroup>{
                torch::optim::OptimizerParamGroup(_encoder_net->parameters()),
                torch::optim::OptimizerParamGroup(_projector_net->parameters())
            },
            torch::optim::AdamWOptions(lr_)              // defaults
        )
    {}

    /**
     * @brief Trains the VICReg_4D model using hierarchical contrastive learning.
     * 
     * This function performs training over a time series dataset using a contrastive loss
     * computed between two differently cropped views of each input. It supports optional
     * length constraints and preprocessing to handle missing values. During training,
     * an SWA-averaged model (`_swa_encoder_net`) is updated to improve generalization at inference time.
     * 
     * The training loop samples random crop regions at each iteration to generate diverse
     * views and uses the averaged contrastive loss to optimize the encoder.
     *
     * @tparam Q The dataset type. This dataset should be compatible with torch::data::Dataset and return samples of type K from its `get(...)` method.
     * @tparam K The sample type returned by the dataset (e.g., a struct containing features and mask tensors).
     * @tparam Td The underlying data type used by the dataset, if required.
     * 
     * @param dataloader   A dataloader yielding 4D tensors of shape (B, C, T, D). 
     * @param n_epochs     Number of training epochs to run (set to -1 to ignore).
     * @param n_iters      Maximum number of training iterations (set to -1 to ignore).
     * @param verbose      Whether to print the average loss at each epoch.
     * 
     * @return std::vector<double>  A log of average training losses, one per epoch.
     */
    template<typename Q, typename K, typename Td>
    std::vector<double> fit(
        cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Q, K, Td, torch::data::samplers::RandomSampler>& dataloader,
        int n_epochs = -1,
        int n_iters = -1,
        int swa_start_iter = 1000, 
        bool verbose = false
    ) {
        /* Training loop */
        int epoch_count = 0;
        int iter_count = 0;
        bool stop_loop = false;
        std::vector<double> loss_log;
        
        /* Set base net & SWA net to training mode */
        _encoder_net->train();
        _swa_encoder_net->encoder().train();
        
        while (!stop_loop) {
            if (n_epochs >= 0 && epoch_count >= n_epochs) {
                break;
            }
    
            double cum_loss = 0.0;
            int epoch_iters = 0;
    
            for (auto& sample_batch : *dataloader) {
                /* If iteration limit is reached */
                if (n_iters >= 0 && iter_count >= n_iters) {
                    stop_loop = true;
                    break;
                }

                /* propagate */
                optimizer.zero_grad();
                
                /* Prepare input batch */
                auto collacted_sample = K::collate_fn(sample_batch);
                auto data = collacted_sample.features.to(device);
                auto mask = collacted_sample.mask.to(device);

                /* sanity checks */
                TORCH_CHECK(!data.requires_grad() && !data.grad_fn(), "(vicreg_4d.h)[VICReg_4D::fit] data still has grad history");
                TORCH_CHECK(!mask.requires_grad() && !mask.grad_fn(), "(vicreg_4d.h)[VICReg_4D::fit] mask still has grad history");
                
                /* validate data and mask are of the correct dimensions */
                TORCH_CHECK(data.dim()   == 4, "(vicreg_4d.h)[VICReg_4D::fit] data from the dataloader must be [B,C,T,D]");
                TORCH_CHECK(data.size(1) == C, "(vicreg_4d.h)[VICReg_4D::fit] data does not matched the expected number of dimensions in the C rank !");
                TORCH_CHECK(data.size(2) == T, "(vicreg_4d.h)[VICReg_4D::fit] data does not matched the expected number of dimensions in the T rank !");
                TORCH_CHECK(data.size(3) == D, "(vicreg_4d.h)[VICReg_4D::fit] data does not matched the expected number of dimensions in the D rank !");
                
                TORCH_CHECK(mask.dim()   == 3, "(vicreg_4d.h)[VICReg_4D::fit] mask from the dataloader must be [B,C,T]");
                TORCH_CHECK(mask.size(1) == C, "(vicreg_4d.h)[VICReg_4D::fit] Mask does not matched the expected number of dimensions in the C rank !");
                TORCH_CHECK(mask.size(2) == T, "(vicreg_4d.h)[VICReg_4D::fit] Mask does not matched the expected number of dimensions in the T rank !");
                
                /* augment the data (time_wrap and random drops) */
                auto [d1,m1] = aug.augment(data, mask);
                auto [d2,m2] = aug.augment(data, mask);    // [d2,m2] != [d1,m1]

                /* forward the encoder network */
                auto k1 = _encoder_net(d1, m1);
                auto k2 = _encoder_net(d2, m2);

                auto z1 = _projector_net(k1);
                auto z2 = _projector_net(k2);

                auto loss = vicreg_loss(
                    z1, z2, 
                    sim_coeff, std_coeff, cov_coeff
                );

                /* back propagate */
                loss.backward();
                optimizer.step();
    
                /* Update SWA parameters */
                _swa_encoder_net->update_parameters(_encoder_net);
    
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
     * @param data      
     * @param mask      Binary mask of shape [B, C, T, D] 
     *                      Values of 1 => valid, 0 => padding. This is multiplied with input to zero out
     *                      padded positions if present. If not provided, a NaN sentinel is stored.
     * 
     * @return torch::Tensor   The encoded outputs.
     */
    torch::Tensor encode(
        const torch::Tensor &data,
        torch::Tensor mask
    ) {
        // Validate input dimensions explicitly
        TORCH_CHECK(data.size(1) == C, "(vicreg_4d.h)[VICReg_4D::encode] data dimension mismatch (C)");
        TORCH_CHECK(data.size(2) == T, "(vicreg_4d.h)[VICReg_4D::encode] data dimension mismatch (T)");
        TORCH_CHECK(data.size(3) == D, "(vicreg_4d.h)[VICReg_4D::encode] data dimension mismatch (D)");

        TORCH_CHECK(mask.size(1) == C, "(vicreg_4d.h)[VICReg_4D::encode] mask dimension mismatch (C)");
        TORCH_CHECK(mask.size(2) == T, "(vicreg_4d.h)[VICReg_4D::encode] mask dimension mismatch (T)");
        TORCH_CHECK(mask.size(3) == D, "(vicreg_4d.h)[VICReg_4D::encode] mask dimension mismatch (D)");

        /* --------- Forward -------------------------------- */
        return _projector_net(
            _swa_encoder_net->forward(
                data, mask
            )
        );
    }

    /**
     * @brief Encodes a batch of time series data using the SWA-averaged model.
     * 
     * @tparam Q The dataset type. This dataset should be compatible with torch::data::Dataset and return samples of type K from its `get(...)` method.
     * @tparam K The sample type returned by the dataset (e.g., a struct containing features and mask tensors).
     * @tparam Td The underlying data type used by the dataset, if required.
     *
     * @param dataloader   A dataloader yielding 4D tensors of shape (B, C, T, D). 
     * 
     * @return torch::Tensor   Concatenated tensor of encoded outputs.
     */
    template<typename Q, typename K, typename Td>
    torch::Tensor encode(
        cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Q, K, Td, torch::data::samplers::SequentialSampler>& dataloader
    ) {
        // Store current training state and set encoder to eval
        bool was_training = _swa_encoder_net->encoder().is_training();
        _swa_encoder_net->encoder().eval();

        // Prepare a vector to collect outputs from each batch
        std::vector<torch::Tensor> outputs;

        {
            torch::NoGradGuard no_grad;

            // Loop over DataLoader
            for (auto& sample_batch : *dataloader) {
                // Collate batch and send to device
                auto collated_sample = K::collate_fn(sample_batch);
                auto data = collated_sample.features.to(device); // [B, C, T, D]
                auto mask = collated_sample.mask.to(device);     // [B, C, T, D]

                // Forward pass: encode expanded data
                outputs.push_back(encode(data, mask));
            } // end for dataloader
        }

        // Restore previous training mode
        if (was_training) {
            _swa_encoder_net->encoder().train();
        }

        // Concatenate and return
        return torch::cat(outputs, /*dim=*/0);
    }

    /**
     * save: saves the SWA model's state_dict
     */
    void save(const std::string& filepath);

    /**
     * load: loads the SWA model's state_dict
     */
    void load(const std::string& filepath);
};

} /* namespace vicreg_4d */
} /* namespace wikimyei */
} /* namespace cuwacunu */



    // /* --------------------------------------------- */
    // /* _eval_with_pooling */
    // /* --------------------------------------------- */

    // /**
    //  * @brief Evaluates the input time series using the SWA-averaged encoder network
    //  *        and optionally applies temporal pooling to produce a fixed-size representation.
    //  *
    //  * This function is used during inference (e.g., encoding time series) to extract
    //  * representations using the smoothed `_swa_encoder_net`, rather than the raw `_encoder_net`.
    //  *
    //  * It mirrors the original `_eval_with_pooling` precisely:
    //  *    1. Runs a forward pass through `_swa_encoder_net`.
    //  *    2. Depending on `encoding_window`:
    //  *       - "full_series": global max-pool over the entire time dimension.
    //  *       - "multiscale": repeatedly max-pool with increasing kernel sizes.
    //  *       - Integer: uses that integer as a kernel_size, with stride=1 and padding=kernel_size//2.
    //  *         If the kernel_size is even, the last timestep in dimension 1 is removed.
    //  *       - Else (no window given or unrecognized string): return the raw per-step representations.
    //  *    3. If `slicing` is provided, it slices the time dimension in `[start, end)`.
    //  *
    //  * @param x              A 3D input tensor of shape (N, T, C), where:
    //  *                         - N = batch size,
    //  *                         - T = number of timesteps,
    //  *                         - C = number of channels/features per timestep.
    //  * @param x_mask      Optional binary mask of shape [B, C, T, D]. 
    //  *                      Values of 1 => valid, 0 => padding. This is multiplied with input to zero out
    //  *                      padded positions if present. If not provided, a NaN sentinel is stored.
    //  * @param encoding_window Optional string controlling how the output is pooled:
    //  *                         - "full_series": global max pool over time (producing shape N,1,C).
    //  *                         - "multiscale": multiple max pools with exponentially growing kernel.
    //  *                         - stringified integer (e.g. "5"): max pool with the specified kernel size.
    //  *                         - no value (or unrecognized string): no pooling.
    //  * @param slicing        An optional pair of (start, end) indices to slice the time dimension.
    //  *
    //  * @return torch::Tensor A 3D tensor of shape (N, T', C') after pooling and/or slicing,
    //  *                       where T' depends on the pooling strategy and slicing.
    //  */
    //  torch::Tensor VICReg_4D::_eval_with_pooling(
    //     torch::Tensor x,
    //     c10::optional<torch::Tensor> x_mask,
    //     c10::optional<std::string> encoding_window,
    //     c10::optional<std::pair<int,int>> slicing
    // ) {
    //     /* 1) Forward pass through the SWA-based encoder. The output is expected to have shape [N, T, C]. */
    //     /*    Adjust the device usage or forward signature as needed for our environment. */
    //     auto out = _swa_encoder_net->forward(x.to(device), x_mask);

    //     /* 2) Check encoding_window to decide how (or if) we apply pooling. */
    //     if (encoding_window && *encoding_window == "full_series")
    //     {
    //         /* Global max-pool over the time dimension. */
    //         /* The output shape is assumed [N, T, C], so transpose to [N, C, T] for max_pool1d. */
    //         out = torch::nn::functional::max_pool1d(
    //             out.transpose(1, 2),
    //             /* kernel_size = out.size(1) => pool over the entire T dimension */
    //             torch::nn::functional::MaxPool1dFuncOptions(out.size(1))
    //         ).transpose(1, 2);
    //     }
    //     else if (encoding_window && *encoding_window == "multiscale")
    //     {
    //         /* Multiscale pooling: repeatedly apply max_pool1d with kernel sizes (2^(p+1) + 1), */
    //         /* each time with stride=1 and padding=2^p, until the kernel would exceed T. */
    //         int p = 0;
    //         std::vector<torch::Tensor> reprs;

    //         /* The time dimension is out.size(1) in [N, T, C]. */
    //         /* while ((1 << p) + 1 < out.size(1)) */
    //         while ((1 << p) + 1 < out.size(1)) {
    //             int kernel_size = (1 << (p + 1)) + 1;
    //             int pad         = (1 << p);

    //             /* max_pool1d requires [N, C, T], so we transpose first. */
    //             auto t_out = torch::nn::functional::max_pool1d(
    //                 out.transpose(1, 2),
    //                 torch::nn::functional::MaxPool1dFuncOptions(kernel_size)
    //                     .stride(1)
    //                     .padding(pad)
    //             ).transpose(1, 2);

    //             reprs.push_back(t_out);
    //             p++;
    //         }

    //         /* Concatenate the pooled representations along the last dimension. */
    //         /* In, dim=-1 => in C++ with [N, T, C], that is dim=2. */
    //         out = torch::cat(reprs, /*dim=*/-1);
    //     }
    //     else if (encoding_window)
    //     {
    //         /* If encoding_window is neither "full_series" nor "multiscale", */
    //         /* we interpret it as an integer string (same as the the Original 'isinstance(encoding_window, int)' case). */
    //         int w;
    //         try {
    //             w = std::stoi(*encoding_window);
    //         } catch (const std::invalid_argument&) {
    //             throw std::runtime_error(
    //                 "encoding_window must be 'full_series', 'multiscale', or a valid integer string"
    //             );
    //         }

    //         /* Apply max_pool1d with kernel_size = w, stride=1, and padding=w/2 (floor division). */
    //         out = torch::nn::functional::max_pool1d(
    //             out.transpose(1, 2),
    //             torch::nn::functional::MaxPool1dFuncOptions(w)
    //                 .stride(1)
    //                 .padding(w / 2)
    //         ).transpose(1, 2);

    //         /* If w is even, remove the last time step (out = out[:, :-1, :] in the Original). */
    //         if (w % 2 == 0) {
    //             auto dim_size = out.size(1);
    //             out = out.index({
    //                 torch::indexing::Slice(),
    //                 torch::indexing::Slice(0, dim_size - 1),
    //                 torch::indexing::Slice()
    //             });
    //         }
    //     }

    //     if (slicing.has_value()) {
    //         auto start = slicing->first;
    //         auto end   = slicing->second;
    //         out = out.index({
    //             torch::indexing::Slice(),
    //             torch::indexing::Slice(start, end),
    //             torch::indexing::Slice()
    //         });
    //     }

    //     return out;
    // }

    // /**
    //  * @brief Encodes a batch of time series data using the SWA-averaged model.
    //  * 
    //  * This method runs the input data through the encoder in evaluation mode,
    //  * optionally applying temporal pooling depending on the `encoding_window` setting.
    //  * Uses a DataLoader for efficient batching and returns the encoded outputs.
    //  * 
    //  * @tparam Q The dataset type. This dataset should be compatible with torch::data::Dataset and return samples of type K from its `get(...)` method.
    //  * @tparam K The sample type returned by the dataset (e.g., a struct containing features and mask tensors).
    //  * @tparam T The underlying data type used by the dataset, if required.
    //  *
    //  * @param dataloader   A dataloader yielding 4D tensors of shape (B, C, T, D). 
    //  * @param x_mask      Optional binary mask of shape [B, C, T, D]. 
    //  *                      Values of 1 => valid, 0 => padding. This is multiplied with input to zero out
    //  *                      padded positions if present. If not provided, a NaN sentinel is stored.
    //  * @param encoding_window  Optional setting for pooling over time (e.g., "full_series", "multiscale", "2", "6", "3"...).
    //  * @param causal           causal handling (currently unused).
    //  * @param sliding_padding  Padding to apply if using sliding windows.
    //  * @param n_samples        Number of samples in the dataset of the dataloader.
    //  * @param sliding_length   sliding window encoding (currently unused).
    //  * @param batch_size_      Optional override for batch size during encoding.
    //  * 
    //  * @return torch::Tensor   Concatenated tensor of encoded outputs.
    //  */
    // template<typename Q, typename K, typename T>
    // torch::Tensor encode(
    //     cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Q, K, T, torch::data::samplers::SequentialSampler>& dataloader,
    //     c10::optional<torch::Tensor> x_mask,
    //     c10::optional<std::string> encoding_window,
    //     bool causal,
    //     int sliding_padding,
    //     c10::optional<int> n_samples,
    //     c10::optional<int> sliding_length
    // ) {
    //     // Store current training state and set encoder to eval
    //     bool was_training = _swa_encoder_net->encoder().is_training();
    //     _swa_encoder_net->encoder().eval();

    //     // Prepare a vector to collect outputs from each batch
    //     std::vector<torch::Tensor> outputs;

    //     {
    //         // Disable grad during inference
    //         torch::NoGradGuard no_grad;

    //         // Loop over DataLoader
    //         for (auto& sample_batch : *dataloader)
    //         {
    //             // batch.data has shape (B, T, C)
    //             /* Prepare input batch */
    //             auto collacted_sample = K::collate_fn(sample_batch);
    //             auto data = collacted_sample.features.to(device);
    //             auto mask = collacted_sample.mask.to(device);
            
    //             /* validate data and mask are of the correct dimensions */
    //             TORCH_CHECK(data.size(1) != C, "(vicreg_4d.h)[VICReg_4D::fit] data does not matched the expected number of dimensions in the C rank !");
    //             TORCH_CHECK(data.size(2) != T, "(vicreg_4d.h)[VICReg_4D::fit] data does not matched the expected number of dimensions in the T rank !");
    //             TORCH_CHECK(data.size(3) != D, "(vicreg_4d.h)[VICReg_4D::fit] data does not matched the expected number of dimensions in the D rank !");

    //             TORCH_CHECK(mask.size(1) != C, "(vicreg_4d.h)[VICReg_4D::fit] Mask does not matched the expected number of dimensions in the C rank !");
    //             TORCH_CHECK(mask.size(2) != T, "(vicreg_4d.h)[VICReg_4D::fit] Mask does not matched the expected number of dimensions in the T rank !");
    //             TORCH_CHECK(mask.size(3) != D, "(vicreg_4d.h)[VICReg_4D::fit] Mask does not matched the expected number of dimensions in the D rank !");

    //             // If no sliding window is requested, do a single pass
    //             if (!sliding_length.has_value()) {
    //                 // => call our existing helper
    //                 auto out = _eval_with_pooling(data, x_mask, encoding_window);

    //                 // if encoding_window == 'full_series', out = out.squeeze(1).
    //                 // Because our `_eval_with_pooling` already returns shape [B, 1, C] after "full_series",
    //                 // we replicate the `.squeeze(1)` step here:
    //                 if (encoding_window.has_value() && encoding_window.value() == "full_series") {
    //                     // If `_eval_with_pooling` ends with shape (B, 1, C),
    //                     // then out.squeeze(1) => (B, C).
    //                     out = out.squeeze(1);
    //                 }
    //                 outputs.push_back(out);
    //             }
    //             else {
    //                 // --------------- Sliding window path ---------------
    //                 TORCH_CHECK(n_samples.has_value(), "When sliding_length is given n_samples argument must also have a value");

    //                 int s_len = sliding_length.value();
    //                 // We'll gather the partial outputs in 'reprs' for each sub-slice
    //                 std::vector<torch::Tensor> reprs;

    //                 // until we can form a full batch pass. Then split the result back.
    //                 // We'll replicate that logic:
    //                 bool do_accumulate = (n_samples.value() < eff_batch_size);

    //                 std::vector<torch::Tensor> calc_buffer;
    //                 int64_t calc_buffer_count = 0; // # sequences so far in buffer

    //                 // Slide over [0..T..s_len]
    //                 // for i in range(0, T, sliding_length):
    //                 for (int64_t i = 0; i < T; i += s_len) {
    //                     // left = i - sliding_padding
    //                     // right = i + sliding_length + (sliding_padding if not causal else 0)
    //                     int64_t left  = i - sliding_padding;
    //                     int64_t right = i + s_len + (causal ? 0 : sliding_padding);

    //                     // clamp [left, right] to [0, T]
    //                     int64_t clamped_left  = std::max<int64_t>(left, 0);
    //                     int64_t clamped_right = std::min<int64_t>(right, T);

    //                     // x[:, clamped_left:clamped_right, :]
    //                     auto x_slice = x.index({
    //                         torch::indexing::Slice(),
    //                         torch::indexing::Slice(clamped_left, clamped_right),
    //                         torch::indexing::Slice()
    //                     });

    //                     //   pad_left  = -left if left<0 else 0
    //                     //   pad_right = right - T if right>T else 0
    //                     int64_t pad_left  = (left < 0) ? -left : 0;
    //                     int64_t pad_right = (right > T) ? (right - T) : 0;

    //                     // total length after padding
    //                     int64_t total_len = x_slice.size(1) + pad_left + pad_right;

    //                     // We'll create a new (B, total_len, features) tensor filled with NaN,
    //                     // then copy x_slice into the center region:
    //                     auto x_sliding = torch::full(
    //                         {B, total_len, x_slice.size(2)},
    //                         std::numeric_limits<float>::quiet_NaN(),
    //                         x_slice.options()
    //                     );
    //                     // place the slice in [pad_left, pad_left + slice_len)
    //                     int64_t slice_len = x_slice.size(1);
    //                     x_sliding.index_put_({
    //                         torch::indexing::Slice(),
    //                         torch::indexing::Slice(pad_left, pad_left + slice_len),
    //                         torch::indexing::Slice()
    //                     }, x_slice);

    //                     // Now x_sliding is our padded slice
    //                     if (do_accumulate) {
    //                         // Accumulate in calc_buffer
    //                         calc_buffer.push_back(x_sliding);
    //                         calc_buffer_count += B;

    //                         // If we've reached or exceeded the desired batch size:
    //                         if (calc_buffer_count >= eff_batch_size) {
    //                             // combine into one big batch
    //                             auto big_input = torch::cat(calc_buffer, /*dim=*/0); // shape (calc_buffer_count, total_len, C)
    //                             calc_buffer.clear();
    //                             calc_buffer_count = 0;

    //                             // Now we call _eval_with_pooling, but we only want to keep
    //                             // the center portion [sliding_padding, sliding_padding+s_len)
    //                             // from the time dimension. 
    //                             // Our `_eval_with_pooling` takes an optional pair for slicing:
    //                             auto out = _eval_with_pooling(
    //                                 big_input,
    //                                 x_mask,
    //                                 encoding_window,
    //                                 c10::optional<std::pair<int, int>>{
    //                                     {sliding_padding, sliding_padding + s_len}
    //                                 }
    //                             );

    //                             // out is shape (calc_buffer_count, ?, D)
    //                             // but we just split it back into sub-chunks of size B
    //                             // so each chunk is (B, ?, D)
    //                             auto splitted = out.split(B, 0);
    //                             for (auto &s : splitted) {
    //                                 reprs.push_back(s);
    //                             }
    //                         }
    //                     }
    //                     else {
    //                         // If we don't need to accumulate, do a direct pass
    //                         auto out = _eval_with_pooling(
    //                             x_sliding,
    //                             x_mask,
    //                             encoding_window,
    //                             c10::optional<std::pair<int, int>>{
    //                                 {sliding_padding, sliding_padding + s_len}
    //                             }
    //                         );
    //                         reprs.push_back(out);
    //                     }
    //                 } // end sliding over time

    //                 // If there's anything left in the buffer, process it now
    //                 if (do_accumulate && calc_buffer_count > 0) {
    //                     auto big_input = torch::cat(calc_buffer, /*dim=*/0);
    //                     calc_buffer.clear();
    //                     calc_buffer_count = 0;

    //                     auto out = _eval_with_pooling(
    //                         big_input,
    //                         x_mask,
    //                         encoding_window,
    //                         c10::optional<std::pair<int, int>>{
    //                             {sliding_padding, sliding_padding + s_len}
    //                         }
    //                     );

    //                     // split out => sub-chunks of size B
    //                     auto splitted = out.split(B, 0);
    //                     for (auto &s : splitted) {
    //                         reprs.push_back(s);
    //                     }
    //                 }

    //                 // Now we have many slices in 'reprs' => each is (B, time_chunk, D).
    //                 // Original code does out = torch.cat(reprs, dim=1) => (B, total_time, D)
    //                 auto cat_reprs = torch::cat(reprs, /*dim=*/1);

    //                 // If encoding_window == 'full_series', the Original code does a final max-pool
    //                 // at the end. But note our _eval_with_pooling also does a pool if "full_series".
    //                 // The Original code: 
    //                 //    out = torch.cat(reprs, dim=1)
    //                 //    if encoding_window == 'full_series':
    //                 //        out = F.max_pool1d(out.transpose(1,2), kernel_size=out.size(1)).squeeze(1)
    //                 // => The user’s `_eval_with_pooling(...)` already does the global pool if "full_series",
    //                 //    but we want to replicate the Original's final step. 
    //                 // => The Original snippet says "If encoding_window == 'full_series', out = out.squeeze(1)."
    //                 //    This is because _eval_with_pooling returned shape (B, 1, D). 
    //                 //    We want (B, D).
    //                 //
    //                 // In practice, our _eval_with_pooling will do the pool, returning shape (B, 1, C).
    //                 // So we do a final .squeeze(1) to match Original's shape.
    //                 if (encoding_window.has_value() && encoding_window.value() == "full_series") {
    //                     cat_reprs = cat_reprs.squeeze(1);
    //                 }

    //                 outputs.push_back(cat_reprs);
    //             } // end if sliding
    //         } // end for dataloader
    //     }

    //     // Restore training mode
    //     if (was_training) {
    //         _swa_encoder_net->encoder().train();
    //     }

    //     // Concatenate all batch outputs (along dim=0)
    //     auto final_out = torch::cat(outputs, /*dim=*/0);

    //     return final_out;
    // }