
/* memory_mapped_datafile.h */
#pragma once

#include <filesystem>
#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {

  /*
   * Template Function: normalize_binary_file
   * -----------------------------------
   * Reads a binary file and reconstructs it into a binary file of the same size but normalized. 
   *
   * Requirements:
   * - The template type T must be a POD type with no padding.
   *
   * Parameters:
   * - bin_filename: Path to the input binary file.
   * - buffer_size:  (Optional) Rolling window size (default to size of file).
   *
   * Returns:
   * - void
   */
template <typename T>
void normalize_binary_file(const std::string& bin_filename, std::size_t window_size = std::numeric_limits<std::size_t>::max()) {
  log_info("[normalize_binary_file] Normalizing binary file from csv data file: %s%s%s\n", 
    ANSI_COLOR_Dim_Black_Grey, bin_filename.c_str(), ANSI_COLOR_RESET);

  /* Open the file in read-write mode (both input and output), binary mode */
  std::fstream bin_file(bin_filename, std::ios::in | std::ios::out | std::ios::binary);
  if (!bin_file.is_open()) {
    log_fatal("[normalize_binary_file] Error: Could not open the binary file: %s%s%s for reading and writing. \n", 
      ANSI_COLOR_Dim_Black_Grey, bin_filename.c_str(), ANSI_COLOR_RESET);
  }

  /* Get the file size */
  bin_file.seekg(0, std::ios::end);
  std::streamsize file_size = bin_file.tellg();
  bin_file.seekg(0, std::ios::beg);

  /* Calculate the total number of records */
  size_t total_records = file_size / sizeof(T);
  if (file_size % sizeof(T) != 0) {
    log_fatal("[normalize_binary_file] Error: Binary file size is not a multiple of struct size: %s%s%s\n", 
      ANSI_COLOR_Dim_Black_Grey, bin_filename.c_str(), ANSI_COLOR_RESET);
  }

  /* assign the window_size */
  if(window_size == std::numeric_limits<std::size_t>::max() || window_size > total_records) {
    window_size = total_records;
  }

  /* Clear any error flags and reset the pointer to the beginning */
  bin_file.clear();
  bin_file.seekg(0, std::ios::beg);

  /* initialize statistics pack  */
  auto stats_pack = T::initialize_statistics_pack(window_size);

  /* here, mean and std for all the fields in T is zero, we iterate before transforming the data */
  while(!stats_pack.ready()) { /* iterate window_size times */
    /* Read the record */
    T record;
    bin_file.read(reinterpret_cast<char*>(&record), sizeof(T));
    if (!bin_file) {
      log_fatal("[normalize_binary_file] Error: Failed to read from binary file: %s%s%s\n", 
        ANSI_COLOR_Dim_Black_Grey, bin_filename.c_str(), ANSI_COLOR_RESET);
    }
    /* update the statistics pack for the initial size */
    stats_pack.update(record);
  }

  /* reset the file to the start again */
  bin_file.clear();
  bin_file.seekg(0, std::ios::beg);

  /* loading bar */
  START_LOADING_BAR(normalization_progress_bar_, 60, "Normalize binary file");

  /* iterate over all the records to replace */
  for (size_t i = 0; i < total_records; ++i) {
    /* Read the record */
    T record;
    bin_file.read(reinterpret_cast<char*>(&record), sizeof(T));
    if (!bin_file) {
      log_fatal("[normalize_binary_file] Error: Failed to read from binary file: %s%s%s\n", 
        ANSI_COLOR_Dim_Black_Grey, bin_filename.c_str(), ANSI_COLOR_RESET);
    }

    if(record.is_valid()) {
      /* Update the statistics space with the record */
      stats_pack.update(record);
      
      /* Normalize the record */
      T normalized_record = stats_pack.normalize(record);
      
      /* Move the put pointer back to the beginning of the record */
      bin_file.seekp(-static_cast<std::streamoff>(sizeof(T)), std::ios::cur);

      /* Write the modified record back */
      bin_file.write(reinterpret_cast<const char*>(&normalized_record), sizeof(T));
      if (!bin_file) {
        log_fatal("[normalize_binary_file] Error: Failed to write to binary file: %s%s%s\n", 
          ANSI_COLOR_Dim_Black_Grey, bin_filename.c_str(), ANSI_COLOR_RESET);
      }

      /* Move the get pointer to the next record */
      bin_file.seekg(bin_file.tellp(), std::ios::beg);

      /* flush the changes */
      bin_file.flush();

    }
    
    /* update loading graph */
    double current_percentage = static_cast<double>((static_cast<double>(i + 1) / total_records) * 100);
    current_percentage = std::round(current_percentage * 100.0) / 100.0; /* round to two decimal places */
    UPDATE_LOADING_BAR(normalization_progress_bar_, current_percentage);
  }

  /* finalize */
  FINISH_LOADING_BAR(normalization_progress_bar_);

  bin_file.close();
  log_info("(normalize_binary_file) %sNormalization completed successfully%s. File: %s%s%s\n", 
    ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, ANSI_COLOR_Dim_Black_Grey, bin_filename.c_str(), ANSI_COLOR_RESET);
}

template<typename T>
std::string sanitize_csv_into_binary_file(const std::string& csv_filename, std::size_t normalization_window = 0, bool force_binarization = false, size_t buffer_size = 1024, char delimiter = ',') {
  log_info("[sanitize_csv_into_binary_file]\t %sPreparing binary:%s file from csv data file: %s\n", 
    ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, csv_filename.c_str());
  
  /* validate parameters */
  if (buffer_size < 1) {
    log_fatal("[sanitize_csv_into_binary_file] Error: buffer_size cannot be zero or negative, requested for file%s\n:", csv_filename.c_str());
  }
  
  /* Get the file size (needed just to displya the loading bar) */
  size_t total_records = cuwacunu::piaabo::dfiles::countLinesInFile(csv_filename);

  /* open the csv file */
  std::ifstream csv_file = cuwacunu::piaabo::dfiles::readFileToStream(csv_filename);
  if (!csv_file.is_open()) {
    log_fatal("[sanitize_csv_into_binary_file] Error: Could not open the CSV file for reading: %s\n", csv_filename.c_str());
  }

  /* get the output name */
  std::string bin_filename = csv_filename;
  cuwacunu::piaabo::string_replace(bin_filename, ".csv", ".bin");

  /* check if operation is needed*/
  if (!force_binarization && std::filesystem::exists(bin_filename)) {
    auto csv_last_write_time = std::filesystem::last_write_time(csv_filename);
    auto bin_last_write_time = std::filesystem::last_write_time(bin_filename);

    if (bin_last_write_time > csv_last_write_time) {
      log_info("[sanitize_csv_into_binary_file]\t %sOperation skiped:%s binary file: %s is up to date. \n", 
        ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, bin_filename.c_str());
      return bin_filename;
    }
  }

  /* open the file and operate*/
  std::ofstream bin_file(bin_filename, std::ios::binary | std::ios::out | std::ios::trunc);
  if (!bin_file.is_open()) {
    csv_file.close();
    log_fatal("[sanitize_csv_into_binary_file] Error: Could not open the binary file: %s\n", csv_filename.c_str());
  }
  chmod(bin_filename.c_str(), S_IRUSR | S_IWUSR);

  /* reserve buffer */
  std::vector<T> buffer;
  buffer.reserve(buffer_size);

  std::string line_p0; /* line at position i */
  std::string line_p1; /* line at position i+1 */
  typename T::key_type_t regular_delta = 0; /*step increase in the key_value */
  typename T::key_type_t current_delta = 0; /*step increase in the key_value */
  
  /* Optionally skip the header line if present */
  /* std::getline(csv_file, line); */

  size_t line_number = 0;
  int64_t delta_steps = 0;

  /* retrive first line and first line object */
  if(!std::getline(csv_file, line_p0)) {
    log_fatal("[sanitize_csv_into_binary_file] Error: file: %s is too short\n", csv_filename.c_str());
  }
  T obj_p0 = T::from_csv(line_p0, delimiter, line_number);

  /* loading bar */
  START_LOADING_BAR(csv_file_preparation_progress_bar_, 60, "Preparing Binary data file");
  std::size_t counter = 0;

  while (std::getline(csv_file, line_p1)) { /* the cycles operates on p0; p1 is just for reference */
    /* update loading graph */
    ++counter;
    double current_percentage = static_cast<double>((static_cast<double>(counter) / total_records) * 100);
    current_percentage = std::round(current_percentage * 100.0) / 100.0; /* round to two decimal places */
    UPDATE_LOADING_BAR(csv_file_preparation_progress_bar_, current_percentage);
    
    /* retrive object */
    T obj_p1 = T::from_csv(line_p1, delimiter, ++line_number);

    /* skip invalid records */
    if(obj_p1.is_valid() == false) {
      continue;
    }

    /* validate sequentiality on the key_value */
    current_delta = obj_p1.key_value() - obj_p0.key_value();
    
    /* validate increment */
    if(current_delta == 0.0) {
      log_warn("%s\t %s•%s [sanitize_csv_into_binary_file]%s %szero increment%s, \n\tat line:\t %s%ld%s, \n\t in file:\t %s%s%s\n", 
        ANSI_CLEAR_LINE, ANSI_COLOR_Yellow, ANSI_COLOR_Dim_Black_Grey, ANSI_COLOR_RESET, 
        ANSI_COLOR_Red, ANSI_COLOR_RESET, 
        ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET, 
        ANSI_COLOR_Dim_Black_Grey, csv_filename.c_str(), ANSI_COLOR_RESET);
      continue; /* skip the binarization of the item as there has been no increment */
    }

    if(current_delta < 0) {
      log_fatal("[sanitize_csv_into_binary_file] Error: key_value increments are expected to be positive, \n\t at line:\t %s%ld%s, \n\t in file:\t %s%s%s, \n\t line:\t %s%s%s\n", 
        ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET, 
        ANSI_COLOR_Dim_Black_Grey, csv_filename.c_str(), ANSI_COLOR_RESET, 
        ANSI_COLOR_Dim_Red, line_p0.c_str(), ANSI_COLOR_RESET);
    }

    /* assing the regular delta, only once */
    if(regular_delta == 0) { regular_delta = current_delta;  } /* just for the first iteration */

    /* not divisible increments are unexpected */
    // if(current_delta % regular_delta != 0) {
    if (std::abs(std::fmod(current_delta, regular_delta)) > 1e-5 && 
      std::abs(std::fmod(current_delta, regular_delta) - regular_delta) > 1e-5) {
      log_err("%s\t %s•%s [sanitize_csv_into_binary_file]%s Error: there is an irregular increment [regular_delta != current_delta](%s%.15f%s != %s%.15f%s), on key_value, \n\t at line:\t %s%ld%s, \n\t in file:\t %s%s%s, \n\t line:\t %s%s%s\n", 
        ANSI_CLEAR_LINE, ANSI_COLOR_Red, ANSI_COLOR_Dim_Black_Grey, ANSI_COLOR_RESET, 
        ANSI_COLOR_Dim_Green, static_cast<double>(regular_delta), ANSI_COLOR_RESET, 
        ANSI_COLOR_Dim_Red, static_cast<double>(current_delta), ANSI_COLOR_RESET, 
        ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET, 
        ANSI_COLOR_Dim_Black_Grey, csv_filename.c_str(), ANSI_COLOR_RESET, 
        ANSI_COLOR_Dim_Red, line_p0.c_str(), ANSI_COLOR_RESET);
      continue;
    }

    /* calculate how many steps there are in between p0 and p1 */
    delta_steps = static_cast<int64_t>(std::round(current_delta / regular_delta));

    /* warn about misisng elements */
    if(delta_steps != 1) {
      log_warn("%s\t %s•%s [sanitize_csv_into_binary_file]%s %sextra large step increment%s (d=%s%ld%s) on key_value, \n\t at line:\t %s%ld%s, \n\t in file:\t %s%s%s\n", 
      ANSI_CLEAR_LINE, ANSI_COLOR_Yellow, ANSI_COLOR_Dim_Black_Grey, ANSI_COLOR_RESET, 
      ANSI_COLOR_Yellow, ANSI_COLOR_RESET, 
      ANSI_COLOR_Yellow, delta_steps, ANSI_COLOR_RESET, 
      ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET, 
      ANSI_COLOR_Dim_Black_Grey, csv_filename.c_str(), ANSI_COLOR_RESET);
    }

    /* record into the binary file n=delta_steps null records to fill the missing steps */
    for(int64_t i = 0; i < delta_steps; i++) {
      /* first (or if only one) use the expected record | anyother would be a null record (with the correct key_value) */
      T obj_px = (i == 0) ? obj_p0 : T::null_instance( obj_p0.key_value() + i * regular_delta );

      try {
        /* Add the object to the buffer */
        buffer.push_back(obj_px);

        /* Write buffer to binary file when full */
        if (buffer.size() == buffer_size) {
          bin_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(T));
          buffer.clear();
        }
      } catch (const std::exception& e) {
        log_fatal("[sanitize_csv_into_binary_file] Error processing line: %s%ld%s: %s%s%s. Exception: %s\n", 
          ANSI_COLOR_Blue, line_number, ANSI_COLOR_RESET, 
          ANSI_COLOR_Dim_Black_Grey, csv_filename.c_str(), ANSI_COLOR_RESET, 
          e.what());
      }
    }

    /* advance the line */
    line_p0 = line_p1;
    obj_p0 = obj_p1;
  }

  /* push the last record to the buffer */
  if(line_number != 0) {
    buffer.push_back(obj_p0);
  }

  /* Write any remaining data in the buffer */
  if (!buffer.empty()) {
    bin_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(T));
    buffer.clear();
  }

  /* finalize handling the file */
  FINISH_LOADING_BAR(csv_file_preparation_progress_bar_);

  csv_file.close();
  bin_file.close();

  /* --- --- --- Normalize --- --- --- */
  if(normalization_window > 0) {
    normalize_binary_file<T>(bin_filename, normalization_window);
  } else {
    log_info("(sanitize_csv_into_binary_file) No normalization configured. %s%s%s -> %s%s%s\n", 
      ANSI_COLOR_Dim_Black_Grey, csv_filename.c_str(), ANSI_COLOR_RESET, 
      ANSI_COLOR_Dim_Black_Grey, bin_filename.c_str(), ANSI_COLOR_RESET);
  }
  
  /* finish sanitation */
  log_info("(sanitize_csv_into_binary_file) %sConversion and sanitation completed successfully%s. %s%s%s -> %s%s%s\n", 
    ANSI_COLOR_Bright_Green, ANSI_COLOR_RESET, 
    ANSI_COLOR_Dim_Black_Grey, csv_filename.c_str(), ANSI_COLOR_RESET, 
    ANSI_COLOR_Dim_Black_Grey, bin_filename.c_str(), ANSI_COLOR_RESET);

  return bin_filename;
}

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
