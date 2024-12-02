#!/bin/bash

# This script downloads Binance klines with a structured folder hierarchy.

symbols=("BTCUSDT")  # Add more symbols if needed
intervals=("1m" "3m" "5m" "15m" "30m" "1h" "2h" "4h" "6h" "8h" "12h" "1d" "3d" "1w")
years=("2020" "2021" "2022" "2023" "2024")  # Start from 2020 since earlier data might not exist
months=(01 02 03 04 05 06 07 08 09 10 11 12)

baseurl="https://data.binance.vision/data/spot/monthly/klines"
download_base_dir="../data"  # Directory to store downloaded files

for symbol in "${symbols[@]}"; do
  for interval in "${intervals[@]}"; do
    for year in "${years[@]}"; do
      for month in "${months[@]}"; do
        # Create folder structure: ../../data/symbol/interval/year/month/
        download_dir="${download_base_dir}/${symbol}/${interval}/${year}/${month}"
        mkdir -p "$download_dir"

        # Construct URLs for both ZIP file and CHECKSUM file
        zip_url="${baseurl}/${symbol}/${interval}/${symbol}-${interval}-${year}-${month}.zip"
        checksum_url="${zip_url}.CHECKSUM"

        # Define file destinations
        zip_destination="${download_dir}/${symbol}-${interval}-${year}-${month}.zip"
        checksum_destination="${download_dir}/${symbol}-${interval}-${year}-${month}.zip.CHECKSUM"

        # Check if ZIP file already exists
        if [ -f "$zip_destination" ]; then
          echo -e "\033[0;33mZIP file already exists: $zip_destination, skipping download.\033[0m"
        else
          # Download ZIP file if it exists on the server
          zip_http_code=$(curl -s -o /dev/null -w "%{http_code}" -I "$zip_url")
          if [ "$zip_http_code" == "200" ]; then
            echo "Downloading: ${zip_url}"
            curl -s -o "$zip_destination" "$zip_url"
            echo -e "\033[0;32mSuccessfully downloaded: ${zip_url} to ${download_dir}\033[0m"
          else
            echo -e "\033[0;31mFile not found (HTTP $zip_http_code): ${zip_url}\033[0m"
          fi
        fi

        # Check if CHECKSUM file already exists
        if [ -f "$checksum_destination" ]; then
          echo -e "\033[0;33mCHECKSUM file already exists: $checksum_destination, skipping download.\033[0m"
        else
          # Download CHECKSUM file if it exists on the server
          checksum_http_code=$(curl -s -o /dev/null -w "%{http_code}" -I "$checksum_url")
          if [ "$checksum_http_code" == "200" ]; then
            echo "Downloading: ${checksum_url}"
            curl -s -o "$checksum_destination" "$checksum_url"
            echo -e "\033[0;32mSuccessfully downloaded: ${checksum_url} to ${download_dir}\033[0m"
          else
            echo -e "\033[0;31mCHECKSUM file not found (HTTP $checksum_http_code): ${checksum_url}\033[0m"
          fi
        fi
      done
    done
  done
done
