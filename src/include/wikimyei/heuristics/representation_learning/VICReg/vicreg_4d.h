/* vicreg_4d.h */
#pragma once

#include <torch/torch.h>
#include <torch/nn/utils/clip_grad.h>
#include <optional>
#include <vector>

#include "piaabo/dconfig.h"

#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d_encoder.h"
#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d_losses.h"
#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d_Projector.h"
#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d_AveragedModel.h"
#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d_Augmentations.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

#include "piaabo/torch_compat/optim/optimizers.h"
#include "piaabo/torch_compat/optim/schedulers/lambda_lr_scheduler.h"

RUNTIME_WARNING("(vicreg_4d.h)[] for imporving performance remember doing torch::jit::Module scripted = torch::jit::freeze(torch::jit::script::Module(my_encoder));\n");

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
    int C;
    int T;
    int D;
    int encoding_dims;
    int channel_expansion_dim;
    int fused_feature_dim;
    int encoder_hidden_dims;
    int encoder_depth;
    std::string projector_mlp_spec;
    double sim_coeff;
    double std_coeff;
    double cov_coeff;
    double optimizer_base_lr;
    double optimizer_weight_decay;
    int optimizer_lr_cycle;
    int optimizer_lr_warmup_epochs;
    double optimizer_lr_min;
    bool optimizer_clamp_weights;
    int optimizer_threshold_reset;
    torch::Dtype dtype;
    torch::Device device;
    bool enable_buffer_averaging;

    /* The base VICReg_4D_Encoder (trainable model) */
    VICReg_4D_Encoder _encoder_net{nullptr};

    /* The SWA/EMA version of the VICReg_4D_Encoder */
    StochasticWeightAverage_Encoder _swa_encoder_net{nullptr};

    /* The Projector from representation to optimizacion lattice */
    VICReg_4D_Projector _projector_net{nullptr};

    /* The aug module, for sample viriances in selfsupervised training */
    VICReg_4D_Augmentation aug;

    /* optimizer for the base networks */
    torch::optim::AdamW optimizer;

    /* learning rate scheduler */
    cuwacunu::piaabo::torch_compat::optim::schedulers::LambdaLR lr_sched;

    /**
     * @brief Constructor for VICReg_4D model
     *
     * @param C_ Number of input channels.
     * @param T_ Number of timesteps in the input.
     * @param D_ Number of input features (dimensionality).
     * @param encoding_dims_ Dimension of the learned representation.
     * @param channel_expansion_dim_ Intermediate expansion size for the encoder (channel-wise).
     * @param fused_feature_dim_ Dimension of the fused feature vector after spatial-temporal encoding.
     * @param encoder_hidden_dims_ Hidden layer size in the VICReg_4D_Encoder.
     * @param encoder_depth_ Number of residual blocks in the VICReg_4D_Encoder.
     * @param projector_mlp_spec_ Specification string for projector MLP architecture (e.g., "8192-8192-8192").
     * @param sim_coeff_ Weight for the invariance loss (MSE between views).
     * @param std_coeff_ Weight for the variance regularization term.
     * @param cov_coeff_ Weight for the covariance regularization term.
     * @param optimizer_base_lr_ Base learning rate for the optimizer.
     * @param optimizer_weight_decay_ Weight decay for the optimizer.
     * @param optimizer_lr_cycle_ Total number of epochs in one learning rate cycle.
     * @param optimizer_lr_warmup_epochs_ Number of warm-up epochs at the start of training.
     * @param optimizer_lr_min_ Minimum learning rate as a fraction of base_lr.
     * @param optimizer_clamp_weights Whether to clamp model weights during training.
     * @param optimizer_threshold_reset Step where to reset AdamW pow exponent. -1 for non reset
     * @param dtype_ Desired floating point precision (e.g., torch::kFloat32).
     * @param device_ Device for model training/inference (e.g., CPU or CUDA).
     * @param enable_buffer_averaging_ If true, SWA model buffers are averaged; if false, copied directly.
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
        double optimizer_base_lr_ = 0.1,
        double optimizer_weight_decay_ = 0.001,
        int optimizer_lr_cycle_ = 50,         // total_epochs (cycle length)
        int optimizer_lr_warmup_epochs_ = 0,  // warm-up epochs
        double optimizer_lr_min_ = 0.001,     // min_lr / base_lr, e.g. 0.001/0.10 = 0.01
        bool optimizer_clamp_weights_ = false, // clamp the weights of the models
        int optimizer_threshold_reset_ = -1, 
        torch::Dtype dtype_ = torch::kFloat32,
        torch::Device device_ = torch::kCPU,
        bool enable_buffer_averaging_ = false
    ) : 
        C(C_),
        T(T_),
        D(D_),
        encoding_dims(encoding_dims_),
        channel_expansion_dim(channel_expansion_dim_),
        fused_feature_dim(fused_feature_dim_),
        encoder_hidden_dims(encoder_hidden_dims_),
        encoder_depth(encoder_depth_),
        projector_mlp_spec(projector_mlp_spec_),
        sim_coeff(sim_coeff_),
        std_coeff(std_coeff_),
        cov_coeff(cov_coeff_),
        optimizer_base_lr(optimizer_base_lr_),
        optimizer_weight_decay(optimizer_weight_decay_),
        optimizer_lr_cycle(optimizer_lr_cycle_),
        optimizer_lr_warmup_epochs(optimizer_lr_warmup_epochs_),
        optimizer_lr_min(optimizer_lr_min_),
        optimizer_clamp_weights(optimizer_clamp_weights_),
        optimizer_threshold_reset(optimizer_threshold_reset_),
        dtype(dtype_),
        device(device_),
        enable_buffer_averaging(enable_buffer_averaging_),
        
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
        
        /* set up the optimizer */
        optimizer({
            _encoder_net->parameters(), 
            _projector_net->parameters()
        }, torch::optim::AdamWOptions(optimizer_base_lr).weight_decay(optimizer_weight_decay)),
        
        /* set up the lr scheduler */
        lr_sched(
            optimizer,
            cuwacunu::piaabo::torch_compat::optim::schedulers::
                warmup_cosine_lambda(
                    static_cast<unsigned>(optimizer_lr_warmup_epochs),  // warm-up epochs
                    optimizer_base_lr,                                  // base_lr 
                    optimizer_lr_min,                                   // min_lr / base_lr, e.g. 0.001/0.10 = 0.01
                    static_cast<unsigned>(optimizer_lr_cycle)           // total_epochs (cycle length)
                ))
    {
        display_model();
        warm_up();
    }

    /**
     * @brief Defacto constructor for VICReg model from configuration file
     *
     * @param C_ Number of input channels.
     * @param T_ Number of timesteps in the input.
     * @param D_ Number of input features (dimensionality).
     */
    VICReg_4D(
        int C_, // n channels
        int T_, // n timesteps
        int D_  // n features
    ) : VICReg_4D(
            C_,                                                                                                 /* C */ 
            T_,                                                                                                 /* T */ 
            D_,                                                                                                 /* D */ 
            cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("VICReg", "encoding_dims"),                /* encoding_dims */
            cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("VICReg", "channel_expansion_dim"),        /* channel_expansion_dim */
            cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("VICReg", "fused_feature_dim"),            /* fused_feature_dim */
            cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("VICReg", "encoder_hidden_dims"),          /* encoder_hidden_dims */
            cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("VICReg", "encoder_depth"),                /* encoder_depth */
            cuwacunu::piaabo::dconfig::config_space_t::get          ("VICReg", "projector_mlp_spec"),           /* projector_mlp_spec */
            cuwacunu::piaabo::dconfig::config_space_t::get<double>  ("VICReg", "sim_coeff"),                    /* sim_coeff */
            cuwacunu::piaabo::dconfig::config_space_t::get<double>  ("VICReg", "std_coeff"),                    /* std_coeff */
            cuwacunu::piaabo::dconfig::config_space_t::get<double>  ("VICReg", "cov_coeff"),                    /* cov_coeff */
            cuwacunu::piaabo::dconfig::config_space_t::get<double>  ("VICReg", "optimizer_base_lr"),            /* optimizer_base_lr */
            cuwacunu::piaabo::dconfig::config_space_t::get<double>  ("VICReg", "optimizer_weight_decay"),       /* optimizer_weight_decay */
            cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("VICReg", "optimizer_lr_cycle"),           /* optimizer_lr_cycle */
            cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("VICReg", "optimizer_lr_warmup_epochs"),   /* optimizer_lr_warmup_epochs */
            cuwacunu::piaabo::dconfig::config_space_t::get<double>  ("VICReg", "optimizer_lr_min"),             /* optimizer_lr_min */
            cuwacunu::piaabo::dconfig::config_space_t::get<bool>    ("VICReg", "optimizer_clamp_weights"),      /* optimizer_clamp_weights */
            cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("VICReg", "optimizer_threshold_reset"),    /* optimizer_threshold_reset */
            cuwacunu::piaabo::dconfig::config_dtype                 ("VICReg"),                                 /* dtype */
            cuwacunu::piaabo::dconfig::config_device                ("VICReg"),                                 /* device */
            cuwacunu::piaabo::dconfig::config_space_t::get<bool>    ("VICReg", "enable_buffer_averaging")       /* enable_buffer_averaging */
        )
    {
        log_info("Initialized VICReg encoder from Configuration file...\n");
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
     * @return std::vector<std::pair<int, double>>  A log of average training losses, one per epoch.
     */
    template<typename Q, typename K, typename Td>
    std::vector<std::pair<int, double>> fit(
        cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Q, K, Td, torch::data::samplers::SequentialSampler>& dataloader,
        int n_epochs = -1,
        int n_iters = -1,
        int swa_start_iter = 1000, 
        bool verbose = false
    ) {
        /* Training loop */
        int epoch_count = 0;
        int iter_count = 0;
        bool stop_loop = false;
        std::vector<std::pair<int, double>> loss_log;
        
        /* Set base net & SWA net to training mode */
        _encoder_net->train();
        _projector_net->train();
        _swa_encoder_net->encoder().train();

        /* used to clamp the parameters */
        std::vector<torch::Tensor> all_p;
        for (auto& p : _encoder_net->parameters())   all_p.push_back(p);
        for (auto& p : _projector_net->parameters()) all_p.push_back(p);
        
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
                
                /* clamp the parameters */
                if(optimizer_clamp_weights) {
                    torch::nn::utils::clip_grad_norm_(all_p, 1.0);   // L2-norm ≤ 1
                }

                /* step the optimizer */
                optimizer.step();
    
                /* Update SWA parameters */
                _swa_encoder_net->update_parameters(_encoder_net);

                /* Accumulate loss */
                cum_loss += loss.template item<double>();
                epoch_iters++;
                iter_count++;

            } /* end for dataloader */

            epoch_count++;
            
            if (!stop_loop && epoch_iters > 0) {
                /* fix AdamW exponent overflow */
                cuwacunu::piaabo::torch_compat::optim::clamp_adam_step(optimizer, static_cast<int64_t>(optimizer_threshold_reset));

                /* debug */
                if(epoch_count == 1 || epoch_count%50 == 0 || epoch_count == n_epochs) {
                    double avg_loss = cum_loss / static_cast<double>(epoch_iters);
                    loss_log.push_back({epoch_count, avg_loss});
                    if (verbose) {
                        log_info("%s Representation Learning %s [ %sEpoch # %s%5d%s ] \t%slr = %s%.6f%s, \t%sloss = %s%.5f%s \n", 
                            ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, 
                            ANSI_COLOR_Dim_Gray, ANSI_COLOR_Dim_Blue, epoch_count, ANSI_COLOR_RESET,
                            ANSI_COLOR_Dim_Gray, ANSI_COLOR_Dim_Magenta, optimizer.param_groups()[0].options().get_lr(), ANSI_COLOR_RESET,
                            ANSI_COLOR_Dim_Gray, ANSI_COLOR_Dim_Red, avg_loss, ANSI_COLOR_RESET
                        );
                    }
                }
            }

            /* call the scheduler ONCE per epoch */
            lr_sched.step();
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

    /**
     * print the configuration values
     */
    void display_model() const {
        // 1) prep any runtime‐only strings
        const char* dtype_str =
        (
            dtype == torch::kInt8    ? "kInt8"   :
            dtype == torch::kInt16   ? "kInt16"  :
            dtype == torch::kInt32   ? "kInt32"  :
            dtype == torch::kInt64   ? "kInt64"  :
            dtype == torch::kFloat32 ? "Float32" :
            dtype == torch::kFloat16 ? "Float16" :
            dtype == torch::kFloat64 ? "Float64" : "Unknown"
        );
        std::string dev = device.str();  // e.g. "cuda:0" or "cpu"
        const char* device_str = dev.c_str();
        const char* swa_str = enable_buffer_averaging ? "true" : "false";
        const char* optimizer_clamp_weights_str = optimizer_clamp_weights ? "true" : "false";
        const char* mlp_spec_str = projector_mlp_spec.c_str();

        // 2) one giant format string
        const char* fmt =
        "\n%s \t[Representation Learning] VICReg_4D:  %s\n"
        "\t\t%s%-25s%s %s%-8d%s\n"        // C
        "\t\t%s%-25s%s %s%-8d%s\n"        // T
        "\t\t%s%-25s%s %s%-8d%s\n"        // D
        "\t\t%s%-25s%s %s%-8d%s\n"        // encoding_dims
        "\t\t%s%-25s%s %s%-8d%s\n"        // channel_expansion_dim
        "\t\t%s%-25s%s %s%-8d%s\n"        // fused_feature_dim
        "\t\t%s%-25s%s %s%-8d%s\n"        // encoder_hidden_dims
        "\t\t%s%-25s%s %s%-8d%s\n"        // encoder_depth
        "\t\t%s%-25s%s %s%-8s%s\n"        // projector_mlp_spec
        "\t\t%s%-25s%s    %s%-8.4f%s\n"   // sim_coeff
        "\t\t%s%-25s%s    %s%-8.4f%s\n"   // std_coeff
        "\t\t%s%-25s%s    %s%-8.4f%s\n"   // cov_coeff
        "\t\t%s%-25s%s %s%-8.6f%s\n"      // optimizer_base_lr
        "\t\t%s%-25s%s %s%-8.6f%s\n"      // optimizer_weight_decay
        "\t\t%s%-25s%s %s%-8d%s\n"        // optimizer_lr_cycle
        "\t\t%s%-25s%s %s%-8d%s\n"        // optimizer_lr_warmup_epochs
        "\t\t%s%-25s%s %s%-8.6f%s\n"      // optimizer_lr_min
        "\t\t%s%-25s%s %s%-8s%s\n"        // optimizer_clamp_weights
        "\t\t%s%-25s%s %s%-8d%s\n"        // optimizer_threshold_reset
        "\t\t%s%-25s%s %s%-8s%s\n";       // SWA buffer avg

        // 3) single fprintf
        log_info(fmt,
        /* header */                    ANSI_COLOR_Dim_Green,      ANSI_COLOR_RESET,
        /* C */                         ANSI_COLOR_Bright_Grey,    "Channels  (C):",            ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  C,                          ANSI_COLOR_RESET,
        /* T */                         ANSI_COLOR_Bright_Grey,    "Timesteps (T):",            ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  T,                          ANSI_COLOR_RESET,
        /* D */                         ANSI_COLOR_Bright_Grey,    "Features  (D):",            ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  D,                          ANSI_COLOR_RESET,
        /* encoding_dims */             ANSI_COLOR_Bright_Grey,    "Encoding dims:",            ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  encoding_dims,              ANSI_COLOR_RESET,
        /* channel_expansion_dim */     ANSI_COLOR_Bright_Grey,    "Channel expansion:",        ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  channel_expansion_dim,      ANSI_COLOR_RESET,
        /* fused_feature_dim */         ANSI_COLOR_Bright_Grey,    "Fused feature dim:",        ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  fused_feature_dim,          ANSI_COLOR_RESET,
        /* encoder_hidden_dims */       ANSI_COLOR_Bright_Grey,    "Encoder hidden dims:",      ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  encoder_hidden_dims,        ANSI_COLOR_RESET,
        /* encoder_depth */             ANSI_COLOR_Bright_Grey,    "Encoder depth:",            ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  encoder_depth,              ANSI_COLOR_RESET,
        /* projector_mlp_spec */        ANSI_COLOR_Bright_Grey,    "Proj MLP spec:",            ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  mlp_spec_str,               ANSI_COLOR_RESET,
        /* sim_coeff */                 ANSI_COLOR_Bright_Grey,    "Sim coeff (λ₁):",           ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  sim_coeff,                  ANSI_COLOR_RESET,
        /* std_coeff */                 ANSI_COLOR_Bright_Grey,    "Std coeff (λ₂):",           ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  std_coeff,                  ANSI_COLOR_RESET,
        /* cov_coeff */                 ANSI_COLOR_Bright_Grey,    "Cov coeff (λ₃):",           ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  cov_coeff,                  ANSI_COLOR_RESET,
        /* optimizer_base_lr */         ANSI_COLOR_Bright_Grey,    "Learning rate (base):",     ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  optimizer_base_lr,          ANSI_COLOR_RESET,
        /* optimizer_weight_decay */    ANSI_COLOR_Bright_Grey,    "Learning weight decay:",    ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  optimizer_weight_decay,     ANSI_COLOR_RESET,
        /* optimizer_lr_cycle */        ANSI_COLOR_Bright_Grey,    "Learning rate cycle:",      ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  optimizer_lr_cycle,         ANSI_COLOR_RESET,
        /* optimizer_lr_warmup_epochs */ANSI_COLOR_Bright_Grey,    "Learning warmup epochs:",   ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  optimizer_lr_warmup_epochs, ANSI_COLOR_RESET,
        /* optimizer_lr_min */          ANSI_COLOR_Bright_Grey,    "Learning rate (min):",      ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  optimizer_lr_min,           ANSI_COLOR_RESET,
        /* optimizer_clamp_weights */   ANSI_COLOR_Bright_Grey,    "Learning clamp weights:",   ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  optimizer_clamp_weights_str,ANSI_COLOR_RESET,
        /* optimizer_threshold_reset */ ANSI_COLOR_Bright_Grey,    "Learning threshold reset:", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue,  optimizer_threshold_reset,   ANSI_COLOR_RESET,
        /* dtype */                     ANSI_COLOR_Bright_Grey,    "Data type:",                ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  dtype_str,                  ANSI_COLOR_RESET,
        /* device */                    ANSI_COLOR_Bright_Grey,    "Device:",                   ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  device_str,                 ANSI_COLOR_RESET,
        /* swa buffer */                ANSI_COLOR_Bright_Grey,    "SWA buffer avg:",           ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  swa_str,                    ANSI_COLOR_RESET
        );
    }
};

} /* namespace vicreg_4d */
} /* namespace wikimyei */
} /* namespace cuwacunu */
