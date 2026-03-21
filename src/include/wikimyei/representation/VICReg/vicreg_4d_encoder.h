/* vicreg_4d_encoder.h */
#pragma once

#include <torch/torch.h>
#include "wikimyei/representation/VICReg/vicreg_4d_dilated_conv.h"
#include "wikimyei/representation/VICReg/vicreg_4d_temporalTransformerModel.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {


/* -----------------------------------------------------------
 *           4D Encoder Module Declaration (with TTN)
 * ----------------------------------------------------------- */
 struct VICReg_4D_EncoderImpl : public torch::nn::Cloneable<VICReg_4D_EncoderImpl> {
    /* ---- Public hyper‑parameters ---------------------------------- */
    int C, T, D;
    int encoding_dim;
    int channel_expansion_dim;
    int fused_feature_dim;
    int hidden_dims;
    int depth;
    torch::Dtype dtype;
    torch::Device device;

    /* ---- Sub‑modules ---------------------------------------------- */
    torch::nn::Conv2d  conv_depthwise{nullptr};
    torch::nn::Conv2d  conv_proj{nullptr};
    torch::nn::Embedding feature_embed{nullptr};
    torch::Tensor      id_encoding;

    TemporalTransformer1D temporal_transform{nullptr};

    torch::nn::Conv2d conv_fuse_channels{nullptr};
    torch::nn::Linear fused_start{nullptr};
    DilatedConvEncoder feature_extractor{nullptr};
    torch::nn::Dropout repr_dropout{nullptr};

    /* ---- Constructor ---------------------------------------------- */
    /**
     * @brief Constructs a temporal sequence encoder with configurable architecture and optional padding mask.
     *
     * This encoder processes 4D time-series data: [B, C, T, D].
     * It supports optional masking to ignore padded or invalid regions in the input.
     *
     * @param C_                     Number of channels (C)
     * @param T_                     Number of timesteps (T)
     * @param D_                     Number of features per timestep (D)
     * @param encoding_dim_          Desired dimensionality of the final representation per timestep
     * @param channel_expansion_dim_ Width of the per-channel expansion
     * @param fused_feature_dim_     Hidden dimension after channel fusion
     * @param hidden_dims_           Number of hidden units per layer in the dilated conv
     * @param depth_                 Number of layers in the dilated conv stack
     * @param default_mask_mode_     Strategy used for dynamic time masking (e.g. Binomial, AllTrue, etc.)
     */
    VICReg_4D_EncoderImpl(
        int C_, int T_, int D_,
        int encoding_dim_ = 320,
        int channel_expansion_dim_ = 64,
        int fused_feature_dim_ = 128,
        int hidden_dims_ = 64,
        int depth_ = 10,
        torch::Dtype dtype_ = torch::kFloat32,
        torch::Device device_ = torch::kCPU
    ) : C(C_), T(T_), D(D_), 
        encoding_dim(encoding_dim_), 
        channel_expansion_dim(channel_expansion_dim_),
        fused_feature_dim(fused_feature_dim_), 
        hidden_dims(hidden_dims_), 
        depth(depth_), 
        dtype(dtype_), 
        device(device_)
    {
        reset();
    }

    /* ---- Build network -------------------------------------------- */
    void reset() override {
        /* 1) Local depth‑wise conv on raw signal */
        conv_depthwise = register_module("conv_depthwise", torch::nn::Conv2d(
            torch::nn::Conv2dOptions(C, C, {3,1}).groups(C).padding({1,0})));

        /* 2) Per‑channel projection 1×D */
        conv_proj = register_module("conv_proj", torch::nn::Conv2d(
            torch::nn::Conv2dOptions(C, C*channel_expansion_dim, {1, D}).groups(C)));

        /* 3) Identity embedding per channel */
        feature_embed = register_module("feature_embed", torch::nn::Embedding(C, channel_expansion_dim));
        feature_embed->to(device, dtype);
        id_encoding = register_buffer("id_encoding", feature_embed->weight.view({1, C, 1, channel_expansion_dim}).to(device, dtype));
        
        /* 4) Temporal transformer operates on flattened (C*E) channels */
        int flat_channels = C * channel_expansion_dim;
        temporal_transform = register_module("temporal_transform", TemporalTransformer1D(flat_channels, T));

        /* 5) Channel fuse (keep T)  [B, C, T, E] -> [B, fused_feature_dim, T, E] */
        conv_fuse_channels = register_module("conv_fuse_channels", torch::nn::Conv2d(
            torch::nn::Conv2dOptions(C, fused_feature_dim, {1,1})));

        /* 6) Linear proj on feature dim (E) */
        fused_start = register_module("fused_start", torch::nn::Linear(channel_expansion_dim, hidden_dims));

        /* 7) Dilated conv stack across time */
        int dilated_input_channels = fused_feature_dim * hidden_dims;
        std::vector<int> channels(depth, hidden_dims);
        channels.push_back(encoding_dim);
        feature_extractor = register_module(
            "feature_extractor",
            DilatedConvEncoder(dilated_input_channels, channels, 3)
        );
        
        /* 8) dropout */
        repr_dropout = register_module("repr_dropout", torch::nn::Dropout(0.1));

        /* 9) move to device and type */
        this->to(device, dtype);
    }

    /* ----------------------------------------------------------------
     * Forward: see comments inline for shapes.
     * @param x_input, 
     * @param x_mask      Optional binary mask of shape [B, C, T]
     *                      Values of 1 => valid, 0 => padding. Invalid slots are
     *                      explicitly zeroed before the first layer and at stage
     *                      boundaries when present.
     * ----------------------------------------------------------------*/
     torch::Tensor forward(const torch::Tensor &x_input,
        c10::optional<torch::Tensor> x_mask = c10::nullopt)
    {
        auto x = x_input;

        auto B = x_input.size(0);
        auto channel_mask_4d = torch::Tensor{};
        auto time_mask_b1t = torch::Tensor{};
        auto time_mask_b1t1 = torch::Tensor{};
        auto time_mask_bt1 = torch::Tensor{};

        auto remask_channel = [&](torch::Tensor t) {
            if (!channel_mask_4d.defined()) return t;
            return t.masked_fill(~channel_mask_4d.expand_as(t), 0.0);
        };
        auto remask_time_bct = [&](torch::Tensor t) {
            if (!time_mask_b1t.defined()) return t;
            return t.masked_fill(~time_mask_b1t.expand_as(t), 0.0);
        };
        auto remask_time_bcte = [&](torch::Tensor t) {
            if (!time_mask_b1t1.defined()) return t;
            return t.masked_fill(~time_mask_b1t1.expand_as(t), 0.0);
        };
        auto remask_time_bte = [&](torch::Tensor t) {
            if (!time_mask_bt1.defined()) return t;
            return t.masked_fill(~time_mask_bt1.expand_as(t), 0.0);
        };

        /* ---- Structural mask ------------------------------------------------ */
        if (x_mask.has_value()) {
            auto mask_bct = x_mask.value().to(torch::kBool);
            channel_mask_4d = mask_bct.unsqueeze(-1); // [B, C, T, 1]
            time_mask_b1t = mask_bct.any(/*dim=*/1, /*keepdim=*/true); // [B, 1, T]
            time_mask_b1t1 = time_mask_b1t.unsqueeze(-1); // [B, 1, T, 1]
            time_mask_bt1 = time_mask_b1t.transpose(1, 2); // [B, T, 1]
            x = x_input.masked_fill(~channel_mask_4d.expand_as(x_input), 0.0);
        }
        
        /* ---- Local convs : [B,C,T,D] -> [B,C,T,E] --------------------------- */
        x = torch::relu(conv_depthwise(x));
        x = remask_channel(x);
        x = conv_proj(x)                 // [B,C*E,T,1]
                .squeeze(-1)             // [B,C*E,T]
                .view({B, C, channel_expansion_dim, T})
                .permute({0,1,3,2});     // [B,C,T,E]
        x = x + id_encoding;             // inject identity
        x = remask_channel(x);
    
        /* ---- Temporal transformer (flatten (C,E)) --------------------------- */
        auto resh = x.permute({0,1,3,2}) // [B,C,E,T]
                     .reshape({B, C*channel_expansion_dim, T}); // [B,flatC,T]
                     
        resh = temporal_transform->forward(resh, time_mask_b1t); // warped

        x = resh.reshape({B, C, channel_expansion_dim, T})
                 .permute({0,1,3,2});    // [B,C,T,E]
        x = remask_channel(x);
    
        /* ---- Fuse channels but keep T --------------------------------------- */
        x = torch::relu(conv_fuse_channels(x));  // [B,F,T,E]
        x = remask_time_bcte(x);
    
        /* ---- Linear proj on feature dim  ------------------------------------ */
        x = x.view({B, fused_feature_dim, T, channel_expansion_dim});
        x = x.view({B * fused_feature_dim * T, channel_expansion_dim});
        x = fused_start(x);
        x = x.view({B, fused_feature_dim, T, hidden_dims});
        x = remask_time_bcte(x);
    
        /* ---- Dilated conv stack with per-block remasking -------------------- */
        x = x.permute({0,1,3,2})   // [B,F,hidden,T]
              .flatten(1,2);       // merge F and hidden as channels
        x = repr_dropout(feature_extractor->forward(x, time_mask_b1t)); // [B,encoding,T]
        x = remask_time_bct(x);

        /* ---- Output --------------------------------------------------------- */
        x = x.transpose(1,2);      // [B,T,encoding_dim]
        x = remask_time_bte(x);

        return x;
    }
};
TORCH_MODULE(VICReg_4D_Encoder);

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
