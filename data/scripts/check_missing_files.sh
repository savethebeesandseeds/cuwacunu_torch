#!/bin/bash

# This script logs missing monthly CSV files for each symbol, interval, and year.

symbols=("BTCUSDT")  # Add more symbols if needed
intervals=("1m" "3m" "5m" "15m" "30m" "1h" "2h" "4h" "6h" "8h" "12h" "1d" "3d" "1w")
years=("2020" "2021" "2022" "2023" "2024")  # Adjust as needed

download_base_dir="../data"  # Directory where downloaded and unzipped CSVs are stored
log_file="missing_files.log"  # Log file to store missing files

# Clear the log file if it exists
[ -f "$log_file" ] && rm "$log_file"

for symbol in "${symbols[@]}"; do
  echo -e "\033[0;32m Symbol: ${symbol}\033[0m"
  for interval in "${intervals[@]}"; do
    echo -e "\tSymbol: ${symbol} ${interval}"
    for year in "${years[@]}"; do
      echo -e "\t\tSymbol: ${symbol} ${interval} ${year}"
      for month in {01..12}; do
        # Construct the expected path for the monthly CSV
        month_csv="${download_base_dir}/${symbol}/${interval}/${year}/${month}/${symbol}-${interval}-${year}-${month}.csv"

        if [ ! -f "$month_csv" ]; then
          # Log the missing file
          echo -e "\t\t\t\t \033[0;31mMissing file: \033[0m ${month_csv}" | tee -a "$log_file"
        fi
      done
    done
  done
done

echo -e "\033[0;32mMissing files have been logged to ${log_file}\033[0m"
