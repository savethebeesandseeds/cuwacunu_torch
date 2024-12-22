#include "piaabo/dfiles.h"
#include "camahjucunu/exchange/exchange_types_data.h"

int main() {
    using T = cuwacunu::camahjucunu::exchange::basic_t;
    size_t buffer_size = 1024;
    std::string bin_filename = "/cuwacunu/data/raw/UTILITIES/sine/sine_wave.bin";
    
    std::vector<T> vector = cuwacunu::piaabo::dfiles::binaryFile_to_vector<T>(bin_filename, buffer_size);

    std::cout << std::fixed << std::setprecision(4) << std::endl;

    size_t count = 0;

    for(T& item: vector) {
        std::cout << "[" << count << "]\t" << item.time << ", " <<  item.value << std::endl;
        count++;
    }
}
