
/* memory_mapped_datafile.h */
#pragma once

#include <filesystem>
#include "piaabo/dutils.h"
#include "piaabo/dlarge_files.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {

template<typename T>
std::string prepare_binary_file_from_csv(const std::string& csv_filename, bool force_operation = false, size_t buffer_size = 1024, char delimiter = ',') {
  /* validate parameters */
  if (buffer_size < 1) {
    log_fatal("[prepare_binary_file_from_csv] Error: buffer_size cannot be zero or negative, requested for file%s\n:", csv_filename.c_str());
  }

  /* open the csv file */
  std::ifstream csv_file(csv_filename);
  if (!csv_file.is_open()) {
    log_fatal("[prepare_binary_file_from_csv] Error: Could not open the CSV file for reading: %s\n", csv_filename.c_str());
  }

  /* get the output name */
  std::string bin_filename = csv_filename;
  cuwacunu::piaabo::string_replace(bin_filename, ".csv", ".bin");

  /* check if operation is needed*/
  if (!force_operation && std::filesystem::exists(bin_filename)) {
    auto csv_last_write_time = std::filesystem::last_write_time(csv_filename);
    auto bin_last_write_time = std::filesystem::last_write_time(bin_filename);

    if (bin_last_write_time > csv_last_write_time) {
      log_info("[prepare_binary_file_from_csv] operation skiped, binary file is up to date. \n");
      return bin_filename;
    }
  }


  /* open the file and operate*/
  std::ofstream bin_file(bin_filename, std::ios::binary | std::ios::out | std::ios::trunc);
  if (!bin_file.is_open()) {
    csv_file.close();
    log_fatal("[prepare_binary_file_from_csv] Error: Could not open the binary file: %s\n", csv_filename.c_str());
  }
  chmod(bin_filename.c_str(), S_IRUSR | S_IWUSR);

  /* reserve buffer */
  std::vector<T> buffer;
  buffer.reserve(buffer_size);

  std::string line_p0; /* line at position i */
  std::string line_p1; /* line at position i+1 */
  typename T::key_type_t regular_delta = 0; /*step increase in the key_value */
  typename T::key_type_t current_delta = 0; /*step increase in the key_value */
  
  // Optionally skip the header line if present
  // std::getline(csv_file, line);

  size_t line_number = 0;
  int64_t delta_steps = 0;

  /* retrive first line and first line object */
  if(!std::getline(csv_file, line_p0)) {
    log_fatal("[prepare_binary_file_from_csv] Error: file: %s is too short\n", csv_filename.c_str());
  }
  T obj_p0 = T::from_csv(line_p0, delimiter, line_number);

  while (std::getline(csv_file, line_p1)) {
    /* this cycle operates on just p0; p1 is just for reference */
    
    /* retrive object */
    T obj_p1 = T::from_csv(line_p1, delimiter, ++line_number);

    /* validate sequentiality on the key_value */
    current_delta = obj_p1.key_value() - obj_p0.key_value();
    
    if(line_number  <= 1) { regular_delta = current_delta;  } /* just for the first iteration */

    if(current_delta <= 0) {
      log_fatal("[prepare_binary_file_from_csv] Error: key_value increments are expected to be positive, at line %ld, in file: %s, \n\t line: %s\n", line_number, csv_filename.c_str(), line_p0.c_str());
    }

    /* validate zero increment */
    if(current_delta == 0.0) {
      log_warn("[prepare_binary_file_from_csv] zero increment at line number %ld on file %s\n", line_number, csv_filename.c_str());
      continue; /* skip the binarization of the item as there has been no increment */
    }

    /* not divisible increments are unexpected */
    if(current_delta % regular_delta != 0) {
      log_err("[prepare_binary_file_from_csv] Error: there is an irregular increment (regular_delta=%ld, current_delta=%ld), lin on key_value at line number %ld, in file: %s, \n\t line: %s\n", regular_delta, current_delta, line_number, csv_filename.c_str(), line_p0.c_str());
      continue;
    }

    /* calculate how many steps there are in between p0 and p1 */
    delta_steps = static_cast<int64_t>(current_delta / regular_delta);

    /* warn about misisng elements */
    if(delta_steps != 1) {
      log_warn("[prepare_binary_file_from_csv] extra large step increment (d=%ld) on key_value at line number %ld, in file: %s\n", delta_steps, line_number, csv_filename.c_str());
    }

    /* record into the binary file n=delta_steps null records to fill the missing steps */
    for(int64_t i = 0; i < delta_steps; i++) {
      try {
        /* Add the object to the buffer */
        buffer.push_back(obj_p0);

        /* Write buffer to binary file when full */
        if (buffer.size() == buffer_size) {
          bin_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(T));
          buffer.clear();
        }
      } catch (const std::exception& e) {
        log_err("[prepare_binary_file_from_csv] Error processing line: %ld: %s. Exception: %s\n", line_number, line_p0.c_str(), e.what());
      }
    }

    /* advance the line */
    line_p0 = line_p1;
    obj_p0 = obj_p1;
  }

  /* Write any remaining data in the buffer */
  if (!buffer.empty()) {
    bin_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(T));
    buffer.clear();
  }

  /* finalize */
  csv_file.close();
  bin_file.close();
  log_info("(prepare_binary_file_from_csv) Conversion completed successfully. %s -> %s\n", csv_filename.c_str(), bin_filename.c_str());

  return bin_filename;
}

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
