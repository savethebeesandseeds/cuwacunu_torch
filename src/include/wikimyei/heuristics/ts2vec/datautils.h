/* datautils.h */
#pragma once
#include <torch/torch.h>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <set>
#include <iostream>

namespace cuwacunu {
namespace wikimyei {
namespace ts2vec {

std::vector<std::vector<double>> read_tsv(const std::string& filepath) {
    std::vector<std::vector<double>> data;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filepath << std::endl;
        return {};
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue; // Skip empty lines

        std::vector<double> row;
        std::stringstream ss(line);
        std::string cell;

        while (std::getline(ss, cell, '\t')) {
            try {
                row.push_back(std::stod(cell));
            } catch (const std::invalid_argument& e) {
                std::cerr << "Conversion error in line: " << line << "\n";
                row.push_back(std::nan("")); // handle conversion error gracefully
            }
        }

        data.push_back(row);
    }

    file.close();
    return data;
}
    

// Placeholder: Dataset-specific exceptions (datasets not to normalize)
const std::set<std::string> normalization_exceptions = {
    "AllGestureWiimoteX", "AllGestureWiimoteY", "AllGestureWiimoteZ", "BME",
    "Chinatown", "Crop", "EOGHorizontalSignal", "EOGVerticalSignal", "Fungi",
    "GestureMidAirD1", "GestureMidAirD2", "GestureMidAirD3",
    "GesturePebbleZ1", "GesturePebbleZ2", "GunPointAgeSpan",
    "GunPointMaleVersusFemale", "GunPointOldVersusYoung", "HouseTwenty",
    "InsectEPGRegularTrain", "InsectEPGSmallTrain", "MelbournePedestrian",
    "PickupGestureWiimoteZ", "PigAirwayPressure", "PigArtPressure", "PigCVP",
    "PLAID", "PowerCons", "Rock", "SemgHandGenderCh2", "SemgHandMovementCh2",
    "SemgHandSubjectCh2", "ShakeGestureWiimoteZ", "SmoothSubspace", "UMD"
};

// Main function to load UCR dataset
struct UCRDataset {
    torch::Tensor train_data;
    torch::Tensor train_labels;
    torch::Tensor test_data;
    torch::Tensor test_labels;

    UCRDataset(torch::Tensor td, torch::Tensor tl, torch::Tensor ted, torch::Tensor tel)
        : train_data(std::move(td)),
          train_labels(std::move(tl)),
          test_data(std::move(ted)),
          test_labels(std::move(tel)) {}
};


torch::Tensor vec2tensor(const std::vector<std::vector<double>>& data) {
    const int64_t rows = data.size();
    const int64_t cols = data[0].size();
    
    auto tensor = torch::empty({rows, cols}, torch::TensorOptions().dtype(torch::kFloat));

    auto tensor_accessor = tensor.accessor<float, 2>();
    for (int64_t i = 0; i < rows; ++i) {
        for (int64_t j = 0; j < cols; ++j) {
            tensor_accessor[i][j] = static_cast<float>(data[i][j]);
        }
    }

    return tensor;
}


UCRDataset load_UCR(const std::string& dataset_name) {
    std::string base_path = "/cuwacunu/data/tests/UCR/" + dataset_name + "/";
    std::string train_file = base_path + dataset_name + "_TRAIN.tsv";
    std::string test_file = base_path + dataset_name + "_TEST.tsv";

    // Placeholder: Implement actual TSV file reading
    auto train_array = read_tsv(train_file);
    auto test_array = read_tsv(test_file);

    // Placeholder: verify data reading
    if (train_array.empty() || test_array.empty()) {
        std::cerr << "Dataset loading failed." << std::endl;
        // Proper exception handling should be implemented here
    }

    // Extract labels and data
    std::vector<double> train_labels_vec, test_labels_vec;
    std::vector<std::vector<double>> train_data_vec, test_data_vec;

    for (const auto& row : train_array) {
        train_labels_vec.push_back(row[0]);
        train_data_vec.emplace_back(row.begin() + 1, row.end());
    }

    for (const auto& row : test_array) {
        test_labels_vec.push_back(row[0]);
        test_data_vec.emplace_back(row.begin() + 1, row.end());
    }

    // Map labels to {0,...,L-1}
    std::set<double> unique_labels(train_labels_vec.begin(), train_labels_vec.end());
    std::unordered_map<double, int64_t> label_transform;
    int64_t idx = 0;
    for (double label : unique_labels) {
        label_transform[label] = idx++;
    }

    std::vector<int64_t> train_labels_mapped, test_labels_mapped;
    for (double label : train_labels_vec) {
        train_labels_mapped.push_back(label_transform[label]);
    }
    for (double label : test_labels_vec) {
        test_labels_mapped.push_back(label_transform[label]);
    }

    // Convert to torch::Tensor
    auto train_tensor = vec2tensor(train_data_vec);
    auto test_tensor = vec2tensor(test_data_vec);
    auto train_labels_tensor = torch::tensor(train_labels_mapped, torch::kInt64);
    auto test_labels_tensor = torch::tensor(test_labels_mapped, torch::kInt64);

    // Dataset-specific normalization
    if (normalization_exceptions.find(dataset_name) == normalization_exceptions.end()) {
        // Compute global mean and std
        auto mean = train_tensor.mean();
        auto std = train_tensor.std();

        // Normalize
        train_tensor = (train_tensor - mean) / std;
        test_tensor = (test_tensor - mean) / std;
    }

    // Add an extra dimension at the end (equivalent to np.newaxis)
    train_tensor = train_tensor.unsqueeze(-1);
    test_tensor = test_tensor.unsqueeze(-1);

    return UCRDataset(train_tensor, train_labels_tensor, test_tensor, test_labels_tensor);
}

} // namespace ts2vec
} // namespace wikimyei
} // namespace cuwacunu
