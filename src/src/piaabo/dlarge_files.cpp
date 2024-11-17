/* dlarge_files.cpp */
#include "piaabo/dlarge_files.h"

namespace cuwacunu {
namespace piaabo {
namespace dlarge_files {

template <typename T>
void csv_to_binary(const std::string& csv_filename, const std::string& bin_filename, size_t buffer_size, char delimiter) {
    if (buffer_size < 1) {
        log_fatal("[csv_to_binary] Error: buffer_size cannot be zero or negative\n");
        return;
    }

    std::ifstream csv_file(csv_filename);
    if (!csv_file.is_open()) {
        log_fatal("[csv_to_binary] Error: Could not open the CSV file %s for reading\n", csv_filename.c_str());
        return;
    }

    std::ofstream bin_file(bin_filename, std::ios::binary);
    if (!bin_file.is_open()) {
        csv_file.close();
        log_fatal("[csv_to_binary] Error: Could not open the binary file %s for writing\n", bin_filename.c_str());
        return;
    }

    std::vector<T> buffer;
    buffer.reserve(buffer_size);

    std::string line;
    // Optionally skip the header line if present
    // std::getline(csv_file, line);

    size_t line_number = 0;
    while (std::getline(csv_file, line)) {
        ++line_number;

        try {
            // Create an instance of T from the CSV line
            T obj = T::from_csv(line, delimiter, line_number);

            // Add the object to the buffer
            buffer.push_back(obj);

            // Write buffer to binary file when full
            if (buffer.size() == buffer_size) {
                bin_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(T));
                buffer.clear();
            }
        } catch (const std::exception& e) {
            std::cerr << "[csv_to_binary] Error processing line " << line_number << ": " << line
                      << "\nException: " << e.what() << std::endl;
            // Skip this line and continue
            continue;
        }
    }

    // Write any remaining data in the buffer
    if (!buffer.empty()) {
        bin_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(T));
        buffer.clear();
    }

    csv_file.close();
    bin_file.close();
    log_info("(CSV->Binary) Conversion completed successfully. %s -> %s\n", csv_filename.c_str(), bin_filename.c_str());
}


template <typename T>
std::vector<T> binary_to_vector(const std::string& bin_filename, size_t buffer_size) {
    std::ifstream bin_file(bin_filename, std::ios::binary);
    if (!bin_file.is_open()) {
        throw std::runtime_error("[binary_to_vector] Error: Could not open the binary file " + bin_filename + " for reading.");
    }

    // Get the file size
    bin_file.seekg(0, std::ios::end);
    std::streamsize file_size = bin_file.tellg();
    bin_file.seekg(0, std::ios::beg);

    // Calculate the total number of records
    size_t total_records = file_size / sizeof(T);
    if (file_size % sizeof(T) != 0) {
        throw std::runtime_error("[binary_to_vector] Error: Binary file size is not a multiple of struct size.");
    }

    std::vector<T> records;
    records.reserve(total_records);

    size_t records_read = 0;
    while (records_read < total_records) {
        size_t records_to_read = std::min(buffer_size, total_records - records_read);
        std::vector<char> buffer(records_to_read * sizeof(T));

        bin_file.read(buffer.data(), buffer.size());
        if (!bin_file) {
            throw std::runtime_error("[binary_to_vector] Error: Failed to read from binary file " + bin_filename);
        }

        
        /* .... 
            Process each record in the buffer
                changing the process loop stage to just this line: 
                    records.insert(records.end(), buffer.begin(), buffer.end()); 
                improves the perfomance at the cost of readability of code 
        */
        for (size_t i = 0; i < records_to_read; ++i) {
            const char* data_ptr = buffer.data() + i * sizeof(T);
            records.emplace_back(T::from_binary(data_ptr));
        }
        /* .... */
        records_read += records_to_read;
    }

    bin_file.close();
    return records;
}


} /* namespace dlarge_files */
} /* namespace piaabo */
} /* namespace cuwacunu */