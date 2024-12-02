#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Root folder (modify this if needed)
ROOT_FOLDER="../data"

# Function to recursively traverse directories and process files
process_files() {
  local folder="$1"
  cd "$folder" || exit 1  # Change to the directory, exit if it fails

  for zip_file in *.zip; do
    # Check if the zip file exists (avoid errors when no files are found)
    [ -e "$zip_file" ] || continue

    # Define the CSV file name pattern based on the ZIP file
    csv_file="${zip_file%.zip}.csv"

    # Remove the CSV file if it exists
    if [ -e "$csv_file" ]; then
      echo -e "${RED}Removing existing CSV file: $csv_file${NC}"
      rm "$csv_file"
    fi

    # Check if the corresponding CHECKSUM file exists
    checksum_file="${zip_file}.CHECKSUM"
    if [ -e "$checksum_file" ]; then
      # Extract expected checksum from the .CHECKSUM file
      expected_checksum=$(awk '{print $1}' "$checksum_file")

      # Compute actual checksum of the ZIP file
      actual_checksum=$(sha256sum "$zip_file" | awk '{print $1}')

      # Verify the checksum
      if [ "$expected_checksum" == "$actual_checksum" ]; then
        echo -e "${GREEN}Checksum OK: $zip_file${NC}"
        unzip -o "$zip_file"  # Unzip with overwrite option
      else
        echo -e "${RED}Checksum MISMATCH: $zip_file${NC}"
      fi
    else
      echo -e "${RED}Missing CHECKSUM file: $checksum_file${NC}"
    fi
  done

  # Recursively process subdirectories
  for dir in */; do
    [ -d "$dir" ] && process_files "$dir"
  done

  cd .. || exit 1  # Return to the parent directory
}

# Start the process from the root folder
process_files "$ROOT_FOLDER"
