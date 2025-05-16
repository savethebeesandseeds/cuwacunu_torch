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

RUNTIME_WARNING("(vicreg_4d.h)[] remember doing torch::jit::Module scripted = torch::jit::freeze(torch::jit::script::Module(my_encoder));\n");
RUNTIME_WARNING("(vicreg_4d.h)[] try along torch::kFloat16\n");

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
    {
        warm_up();
    }

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
        _projector_net->train();
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

                /* compute the loss */
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
     * @brief Warm up the model, the first foward run is usually very slow. 
     *  
     * @return void.
     */
    void warm_up() {
        /* cpu does not need to be warm_up */
        if(!device.is_cpu()) {
            int B = 1; // warm_up batch size
            TICK(warming_up_vicreg_4d_);
            {
                auto data = torch::ones({B,C,T,D}, torch::TensorOptions().dtype(dtype).device(device));
                auto mask = torch::full({B, C, T}, true, torch::TensorOptions().dtype(torch::kBool).device(device));
                encode(data, mask);
                torch::cuda::synchronize();
            }
            PRINT_TOCK_ms(warming_up_vicreg_4d_);
        }
    }

    /**
     * @brief Encodes a batch of time series data using the SWA-averaged model.
     * 
     * @param data      [B, C, T, D] 
     * @param mask      Binary mask of shape [B, C, T] 
     *                      Values of 1 => valid, 0 => padding. This is multiplied with input to zero out
     *                      padded positions if present. If not provided, a NaN sentinel is stored.
     * 
     * @return torch::Tensor   The encoded outputs.
     */
    torch::Tensor encode(
        const torch::Tensor &data,
        torch::Tensor mask
    ) {
        // _encoder_net->eval();
        _projector_net->eval();
        _swa_encoder_net->encoder().eval();

        /* Validate input dimensions explicitly */
        TORCH_CHECK(data.dim()   == 4, "(vicreg_4d.h)[VICReg_4D::encode] data from the dataloader must be [B,C,T,D]");
        TORCH_CHECK(data.size(1) == C, "(vicreg_4d.h)[VICReg_4D::encode] data does not matched the expected number of dimensions in the C rank !");
        TORCH_CHECK(data.size(2) == T, "(vicreg_4d.h)[VICReg_4D::encode] data does not matched the expected number of dimensions in the T rank !");
        TORCH_CHECK(data.size(3) == D, "(vicreg_4d.h)[VICReg_4D::encode] data does not matched the expected number of dimensions in the D rank !");
        
        TORCH_CHECK(mask.dim()   == 3, "(vicreg_4d.h)[VICReg_4D::encode] mask from the dataloader must be [B,C,T]");
        TORCH_CHECK(mask.size(1) == C, "(vicreg_4d.h)[VICReg_4D::encode] Mask does not matched the expected number of dimensions in the C rank !");
        TORCH_CHECK(mask.size(2) == T, "(vicreg_4d.h)[VICReg_4D::encode] Mask does not matched the expected number of dimensions in the T rank !");

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
