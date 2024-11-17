/* dlarge_files.h */
#pragma once
#include <fstream>
#include "piaabo/dutils.h"

RUNTIME_WARNING("(dlarge_files.h)[] binary_to_vector has a note on imporving performance \n");

/*  
  Note: The binary representation is dependent on the system's endianness.
    This code assumes that the same system architecture and endianness will be used
    for both writing and reading the binary data. If the binary file is to be shared
    across different architectures, consider implementing endianness conversion using
    functions like htonl, htons, ntohl, ntohs, or custom serialization methods.
*/
namespace cuwacunu {
namespace piaabo {
namespace dlarge_files {

  /*
   * Template Function: csv_to_binary
   * -------------------------------
   * Converts a CSV file to a binary file by parsing each line of the CSV
   * and serializing it into binary format.
   *
   * Requirements:
   * - The template type T must provide a static method:
   *     static T from_csv(const std::string& line, char delimiter, size_t line_number = 0);
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
   * - delimiter:    (Optional) Delimiter character used in the CSV file (default: ',').
   */
  template <typename T>
  void csv_to_binary(const std::string& csv_filename, const std::string& bin_filename, size_t buffer_size = 1024, char delimiter = ',');

  /*
   * Template Function: binary_to_vector
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
  std::vector<T> binary_to_vector(const std::string& bin_filename, size_t buffer_size = 1024);


} /* namespace dlarge_files */
} /* namespace piaabo */
} /* namespace cuwacunu */
