#!/bin/bash

# This script merges monthly CSV files into yearly CSVs and then merges all yearly CSVs into interval CSVs.

symbols=("BTCUSDT")  # Add more symbols if needed
intervals=("1m" "3m" "5m" "15m" "30m" "1h" "2h" "4h" "6h" "8h" "12h" "1d" "3d" "1w")
years=("2020" "2021" "2022" "2023" "2024")  # Adjust as needed

download_base_dir="../data"  # Directory where downloaded and unzipped CSVs are stored

for symbol in "${symbols[@]}"; do
  for interval in "${intervals[@]}"; do
    # Define the output interval-level CSV
    interval_csv="${download_base_dir}/${symbol}/${interval}/${symbol}-${interval}-all-years.csv"
    
    # Remove the interval CSV if it already exists to avoid appending
    [ -f "$interval_csv" ] && rm "$interval_csv"

    for year in "${years[@]}"; do
      # Create output directory if it doesn't exist
      yearly_dir="${download_base_dir}/${symbol}/${interval}/${year}"
      mkdir -p "$yearly_dir"

      # Define the yearly CSV file
      yearly_csv="${yearly_dir}/${symbol}-${interval}-${year}.csv"

      echo -e "Processing year: $year for symbol: $symbol and interval: $interval"

      # Remove the yearly file if it already exists to avoid appending
      [ -f "$yearly_csv" ] && rm "$yearly_csv"

      # Loop over all months and merge them into the yearly file
      for month in {01..12}; do
        month_csv="${download_base_dir}/${symbol}/${interval}/${year}/${month}/${symbol}-${interval}-${year}-${month}.csv"

        if [ -f "$month_csv" ]; then
          echo -e "\tAdding ${month_csv} to ${yearly_csv}"
          cat "$month_csv" >> "$yearly_csv"
        else
          echo -e "\033[0;31mSkipping missing file: ${month_csv}\033[0m"
        fi
      done

      echo -e "\033[0;32mYearly CSV created: ${yearly_csv}\033[0m"

      # Add yearly CSV to the interval CSV
      if [ -f "$yearly_csv" ]; then
        echo -e "\tAdding ${yearly_csv} to ${interval_csv}"
        cat "$yearly_csv" >> "$interval_csv"
      else
        echo -e "\033[0;31mSkipping missing yearly file: ${yearly_csv}\033[0m"
      fi
    done

    echo -e "\033[0;32mInterval CSV created: ${interval_csv}\033[0m"
  done
done
