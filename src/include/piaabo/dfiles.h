/* dfiles.h */
#pragma once
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#if defined(__unix__) || defined(__APPLE__)
#include <sys/stat.h>
#endif
#include "piaabo/dutils.h"

/*  
  Note: The binary representation is dependent on the system's endianness.
  This code assumes that the same system architecture and endianness will be used
  for both writing and reading the binary data. If the binary file is to be shared
  across different architectures, consider implementing endianness conversion using
  functions like htonl, htons, ntohl, ntohs, or custom serialization methods.
*/
namespace cuwacunu {
namespace piaabo {
namespace dfiles {

std::string readFileToString(const std::string& filePath);
std::ifstream readFileToStream(const std::string& filePath);

  /*
   * Template Function: csvFile_to_binary
   * -------------------------------
   * Converts a CSV file to a binary file by parsing each line of the CSV
   * and serializing it into binary format.
   *
   * Requirements:
   * - The template type T must provide a static method:
   *  static T from_csv(const std::string& line, char delimiter, size_t line_number = 0);
   *   This method should parse a CSV line into an instance of T.
   *
   * Examples:
   * - Structs like `kline_t` and `trade_t` implement the `from_csv` method
   *   and can be used with this template function.
   *
   * Parameters:
   * - csv_filename: Path to the input CSV file.
   * - bin_filename: Path to the output binary file.
   * - buffer_size:  (Optional) Number of records to buffer before writing to disk (default: 1024).
   * - delimiter:  (Optional) Delimiter character used in the CSV file (default: ',').
   */
template <typename T>
void csvFile_to_binary(const std::string& csv_filename, const std::string& bin_filename, size_t buffer_size = 1024, char delimiter = ',') {
  static_assert(std::is_trivially_copyable<T>::value,
                "[csvFile_to_binary] T must be trivially copyable for raw binary writes");
  
  if (buffer_size == 0) {
    log_fatal("[csvFile_to_binary] Error: buffer_size cannot be zero\n");
    return;
  }

  std::ifstream csv_file = cuwacunu::piaabo::dfiles::readFileToStream(csv_filename);

  std::ofstream bin_file(bin_filename, std::ios::binary | std::ios::out | std::ios::trunc);
  if (!bin_file.is_open()) {
    csv_file.close();
    log_fatal("[csvFile_to_binary] Error: Could not open the binary file %s for writing\n", bin_filename.c_str());
    return;
  }
#if defined(__unix__) || defined(__APPLE__)
  if (chmod(bin_filename.c_str(), S_IRUSR | S_IWUSR) != 0) {
    log_warn("[csvFile_to_binary] Warning: chmod failed on %s (%d: %s)\n",
             bin_filename.c_str(),
             errno,
             std::strerror(errno));
  }
#endif

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
        bin_file.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size() * sizeof(T)));
        if (!bin_file) {
          csv_file.close();
          bin_file.close();
          log_fatal("[csvFile_to_binary] Error: Failed writing buffered records to %s\n",
                    bin_filename.c_str());
          return;
        }
        buffer.clear();
      }
    } catch (const std::exception& e) {
      log_warn("[csvFile_to_binary] Error processing line %zu: %s | Exception: %s\n",
               line_number,
               line.c_str(),
               e.what());
      // Skip this line and continue
      continue;
    }
  }

  // Write any remaining data in the buffer
  if (!buffer.empty()) {
    bin_file.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size() * sizeof(T)));
    if (!bin_file) {
      csv_file.close();
      bin_file.close();
      log_fatal("[csvFile_to_binary] Error: Failed writing tail records to %s\n",
                bin_filename.c_str());
      return;
    }
    buffer.clear();
  }

  bin_file.flush();
  if (!bin_file) {
    csv_file.close();
    bin_file.close();
    log_fatal("[csvFile_to_binary] Error: Failed flushing output file %s\n", bin_filename.c_str());
    return;
  }

  csv_file.close();
  bin_file.close();
  log_info("(CSV->Binary) Conversion completed successfully. %s -> %s\n", csv_filename.c_str(), bin_filename.c_str());
}

  /*
   * Template Function: binaryFile_to_vector
   * -----------------------------------
   * Reads a binary file and reconstructs a vector of structs of type T.
   *
   * Requirements:
   * - The template type T must be a POD type with no padding.
   *
   * Parameters:
   * - bin_filename: Path to the input binary file.
   * - buffer_size:  (Optional) Number of records to read at a time (default: 1024).
   *
   * Returns:
   * - A std::vector<T> containing the records read from the binary file.
   */
template <typename T>
std::vector<T> binaryFile_to_vector(const std::string& bin_filename, size_t buffer_size = 1024) {
  if (buffer_size == 0) {
    throw std::runtime_error("[binaryFile_to_vector] Error: buffer_size must be greater than zero.");
  }

  std::ifstream bin_file(bin_filename, std::ios::binary);
  if (!bin_file.is_open()) {
    throw std::runtime_error("[binaryFile_to_vector] Error: Could not open the binary file " + bin_filename + " for reading.");
  }

  // Get the file size
  bin_file.seekg(0, std::ios::end);
  if (!bin_file) {
    throw std::runtime_error("[binaryFile_to_vector] Error: Failed to seek to end of file " + bin_filename);
  }
  std::streamsize file_size = bin_file.tellg();
  if (file_size < 0) {
    throw std::runtime_error("[binaryFile_to_vector] Error: Failed to determine file size for " + bin_filename);
  }
  bin_file.seekg(0, std::ios::beg);
  if (!bin_file) {
    throw std::runtime_error("[binaryFile_to_vector] Error: Failed to seek to beginning of file " + bin_filename);
  }

  // Calculate the total number of records
  if (file_size % sizeof(T) != 0) {
    throw std::runtime_error("[binaryFile_to_vector] Error: Binary file size is not a multiple of struct size.");
  }
  size_t total_records = static_cast<size_t>(file_size / static_cast<std::streamsize>(sizeof(T)));

  std::vector<T> records;
  records.reserve(total_records);

  size_t records_read = 0;
  while (records_read < total_records) {
    size_t records_to_read = std::min(buffer_size, total_records - records_read);
    std::vector<char> buffer(records_to_read * sizeof(T));

    const std::streamsize bytes_to_read = static_cast<std::streamsize>(buffer.size());
    bin_file.read(buffer.data(), bytes_to_read);
    if (bin_file.gcount() != bytes_to_read) {
      throw std::runtime_error("[binaryFile_to_vector] Error: Failed to read from binary file " + bin_filename);
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

size_t countLinesInFile(const std::string& file_path);

} /* namespace dfiles */
} /* namespace piaabo */
} /* namespace cuwacunu */
