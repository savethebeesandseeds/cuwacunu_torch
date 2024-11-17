/*

{
    
    resolve the dilema of the separated projects cuwacunu_utils
    create the binary files if: do this every time the dataloader is created
        a) no file exist,
        b) csv file date exceeds binary file date
    
    # operate only if the daterange is requested
    
    # deal with holes in the data

    # libtorch

    # this one is a sub module of the upper observation_pipeline


class binance_dataloader : Dataloader {

    stream<sample_t> Access_Data_Lazily:
    sample_t Random_sample
}

}

# check if the data needs to be completed by requesting binance 

*/

/* binance_dataliader.h */
#pragma once
#include <torch/torch.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <mutex>
#include "piaabo/dutils.h"
#include "piaabo/dlarge_files.h"
#include "camahjucunu/exchange/abstract/abstract_dataloader.h"

RUNTIME_WARNING("[binance_dataloader.h]() For large files, memory mapping techniques can be used like mmap \n");


namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace dataloader {
namespace binance {


/*
    make a note: template argument needs to have a method tensor_features that returns a std::vector<double>
*/

template <typename T>
class MemoryMappedDataset : public torch::data::datasets::Dataset<MemoryMappedDataset<T>, torch::Tensor> {
private:
    std::string bin_filename_;
    size_t num_records_;
    int fd_;
    void* data_ptr_;
    size_t file_size_;

public:
    MemoryMappedDataset(const std::string& bin_filename) : bin_filename_(bin_filename) {
        // Open the file
        fd_ = open(bin_filename_.c_str(), O_RDONLY);
        if (fd_ == -1) {
            throw std::runtime_error("[MemoryMappedDataset] Error: Could not open binary file " + bin_filename_);
        }

        // Get file size
        file_size_ = lseek(fd_, 0, SEEK_END);
        lseek(fd_, 0, SEEK_SET);

        // Calculate number of records
        num_records_ = file_size_ / sizeof(T);
        if (file_size_ % sizeof(T) != 0) {
            close(fd_);
            throw std::runtime_error("[MemoryMappedDataset] Error: Binary file size is not a multiple of struct size.");
        }

        // Memory-map the file
        data_ptr_ = mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
        if (data_ptr_ == MAP_FAILED) {
            close(fd_);
            throw std::runtime_error("[MemoryMappedDataset] Error: Failed to memory-map the file.");
        }
    }

    ~MemoryMappedDataset() {
        munmap(data_ptr_, file_size_);
        close(fd_);
    }

    torch::Tensor get(size_t index) override {
        if (index >= num_records_) {
            throw std::out_of_range("[MemoryMappedDataset] Index out of range.");
        }

        // Access the record directly
        const T* record_ptr = reinterpret_cast<const T*>(static_cast<const char*>(data_ptr_) + index * sizeof(T));
        T record = *record_ptr;

        // Convert the record to a tensor
        return torch::tensor(record.tensor_features());
    }

    torch::optional<size_t> size() const override {
        return num_records_;
    }
};


/*
    usage:

    int main() {
        // Create the dataset
        auto dataset = MemoryMappedDataset<kline_t>("data.bin");

        // Create the DataLoader
        auto data_loader = torch::data::make_data_loader<torch::Tensor>(
            std::move(dataset),
            torch::data::DataLoaderOptions()
                .batch_size(64)
                .workers(4)
                .collate_fn(torch::data::transforms::Stack<torch::Tensor>())
        );

        // Training loop
        for (auto& batch : *data_loader) {
            auto inputs = batch; // Batch of states

            // Use inputs in your RL algorithm
        }

        return 0;
    }

*/

} /* namespace binance */
} /* namespace dataloader */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */