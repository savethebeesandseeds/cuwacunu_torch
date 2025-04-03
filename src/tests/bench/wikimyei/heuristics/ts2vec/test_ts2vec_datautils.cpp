#include <torch/torch.h>
#include <iostream>
#include "wikimyei/heuristics/ts2vec/datautils.h"

int main() {
    const std::string dataset_name = "ECG200";

    try {
        cuwacunu::wikimyei::ts2vec::UCRDataset ucr = cuwacunu::wikimyei::ts2vec::load_UCR(dataset_name);

        std::cout << "Train data shape: " << ucr.train_data.sizes() << std::endl;
        std::cout << "Test data shape: " << ucr.test_data.sizes() << std::endl;
        std::cout << "Train labels shape: " << ucr.train_labels.sizes() << std::endl;
        std::cout << "Test labels shape: " << ucr.test_labels.sizes() << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
