/* vicreg_rank4_dilated_conv.h */
#pragma once
#include <torch/torch.h>

#include <utility>
#include <vector>

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_rank4 {
    
// Utility function: Same padding convolution (SamePadConv)
struct SamePadConvImpl : public torch::nn::Cloneable<SamePadConvImpl> {
    int in_channels;
    int out_channels;
    int kernel_size;
    int dilation;
    int groups;
    int receptive_field;
    int remove;
    torch::nn::Conv1d conv{nullptr};

    SamePadConvImpl(int in_channels_, int out_channels_, int kernel_size_, int dilation_ = 1, int groups_ = 1) 
    :   in_channels(in_channels_), 
        out_channels(out_channels_), 
        kernel_size(kernel_size_), 
        dilation(dilation_), 
        groups(groups_), 
        receptive_field((kernel_size - 1) * dilation + 1), 
        remove((receptive_field % 2 == 0) ? 1 : 0) 
    {
        // According to Cloneable pattern, initialization should happen in reset()
        reset();
    }
    
    void reset() override {
        int padding = receptive_field / 2;
        conv = torch::nn::Conv1d(torch::nn::Conv1dOptions(in_channels, out_channels, kernel_size)
                                 .padding(padding)
                                 .dilation(dilation)
                                 .groups(groups));
        register_module("conv", conv);
    }

    torch::Tensor forward(torch::Tensor x) {
        auto out = conv->forward(x);
        if (remove > 0) {
            out = out.index({"...", torch::indexing::Slice(c10::nullopt, -remove)});
        }
        return out;
    }
};
TORCH_MODULE(SamePadConv);

// ConvBlock with residual connection
struct ConvBlockImpl : public torch::nn::Cloneable<ConvBlockImpl> {
    int in_channels;
    int out_channels;
    int kernel_size;
    int dilation;
    bool final;

    SamePadConv conv1{nullptr}, conv2{nullptr};
    torch::nn::Conv1d projector{nullptr};
    bool has_projector;

    ConvBlockImpl(int in_channels_, int out_channels_, int kernel_size_, int dilation_, bool final_ = false)
    :   in_channels(in_channels_), 
        out_channels(out_channels_), 
        kernel_size(kernel_size_), 
        dilation(dilation_), 
        final(final_) 
    {
        // According to Cloneable pattern, initialization should happen in reset()
        reset();
    }

    void reset() override {
        conv1 = SamePadConv(in_channels, out_channels, kernel_size, dilation);
        conv2 = SamePadConv(out_channels, out_channels, kernel_size, dilation);
        register_module("conv1", conv1);
        register_module("conv2", conv2);

        has_projector = (in_channels != out_channels) || final;
        if (has_projector) {
            projector = torch::nn::Conv1d(torch::nn::Conv1dOptions(in_channels, out_channels, 1));
            register_module("projector", projector);
        }
    }

    torch::Tensor forward(torch::Tensor x,
                          c10::optional<torch::Tensor> time_mask = c10::nullopt) {
        const auto mask = time_mask.has_value()
            ? time_mask.value().to(torch::kBool)
            : torch::Tensor{};
        auto remask = [&](torch::Tensor t) {
            if (!mask.defined()) return t;
            return t.masked_fill(~mask.expand_as(t), 0.0);
        };

        auto residual = has_projector ? projector->forward(x) : x;
        x = torch::gelu(x);
        x = conv1->forward(x);
        x = remask(std::move(x));
        x = torch::gelu(x);
        x = conv2->forward(x);
        x = remask(std::move(x));
        x = x + residual;
        return remask(std::move(x));
    }
};
TORCH_MODULE(ConvBlock);

// Dilated convolution encoder
struct DilatedConvEncoderImpl : public torch::nn::Cloneable<DilatedConvEncoderImpl> {
    int in_channels;
    std::vector<int> channels;
    int kernel_size;
    torch::nn::Sequential net{nullptr};

    DilatedConvEncoderImpl(int in_channels_, const std::vector<int>& channels_, int kernel_size_)
    :   in_channels(in_channels_),
        channels(channels_),
        kernel_size(kernel_size_)
    {
        reset();
    }

    void reset() override {
        torch::nn::Sequential seq;
        int current_channels = in_channels;
        for (size_t i = 0; i < channels.size(); ++i) {
            int in_ch = current_channels;
            int out_ch = channels[i];
            int dilation = 1 << i; // 2^i dilation
            bool final = (i == channels.size() - 1);

            seq->push_back(ConvBlock(in_ch, out_ch, kernel_size, dilation, final));
            current_channels = out_ch;
        }
        net = register_module("net", seq);
    }

    torch::Tensor forward(torch::Tensor x,
                          c10::optional<torch::Tensor> time_mask = c10::nullopt) {
         auto remask = [&](torch::Tensor t) {
             if (!time_mask.has_value()) return t;
             auto mask = time_mask.value().to(torch::kBool);
             return t.masked_fill(~mask.expand_as(t), 0.0);
         };

         TORCH_CHECK(net, "[DilatedConvEncoder] 'net' submodule not initialized");
         x = remask(x);
         for (size_t i = 0; i < net->size(); ++i) {
             x = net->ptr<ConvBlockImpl>(i)->forward(x, time_mask);
             x = remask(x);
         }
         return remask(x);
    }
};
TORCH_MODULE(DilatedConvEncoder);

} // namespace vicreg_rank4
} // namespace wikimyei
} // namespace cuwacunu
