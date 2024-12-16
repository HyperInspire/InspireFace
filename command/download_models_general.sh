#!/bin/bash

# Target download folder
DOWNLOAD_DIR="test_res/pack"

# File URLs
URL1="https://github.com/HyperInspire/InspireFace/releases/download/v1.x/Megatron"
URL2="https://github.com/HyperInspire/InspireFace/releases/download/v1.x/Pikachu"

# Color codes
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create download folder
mkdir -p "$DOWNLOAD_DIR"

# Function to download file
download_file() {
    local url=$1
    if command -v wget > /dev/null 2>&1; then
        echo "Using wget for download..."
        wget --no-check-certificate -L -P "$DOWNLOAD_DIR" "$url"
    else
        echo "wget not found, using curl instead..."
        cd "$DOWNLOAD_DIR"
        curl -L -O "$url"
        cd - > /dev/null
    fi
}

# Function to print file path
print_file_path() {
    local filename=$1
    echo -e "File downloaded to: ${YELLOW}$(cd "$DOWNLOAD_DIR" && pwd)/${filename}${NC}"
}

# Check if argument is provided
if [ $# -eq 0 ]; then
    echo "No argument provided, downloading all files..."
    download_file "$URL1"
    download_file "$URL2"
    # Check both files
    if [ -f "$DOWNLOAD_DIR/Megatron" ] && [ -f "$DOWNLOAD_DIR/Pikachu" ]; then
        echo "All downloads completed successfully!"
        print_file_path "Megatron"
        print_file_path "Pikachu"
    else
        echo "Download failed!"
        exit 1
    fi
else
    case "$1" in
        "Megatron")
            echo "Downloading Megatron..."
            download_file "$URL1"
            # Check Megatron file
            if [ -f "$DOWNLOAD_DIR/Megatron" ]; then
                echo "Megatron download completed successfully!"
                print_file_path "Megatron"
            else
                echo "Megatron download failed!"
                exit 1
            fi
            ;;
        "Pikachu")
            echo "Downloading Pikachu..."
            download_file "$URL2"
            # Check Pikachu file
            if [ -f "$DOWNLOAD_DIR/Pikachu" ]; then
                echo "Pikachu download completed successfully!"
                print_file_path "Pikachu"
            else
                echo "Pikachu download failed!"
                exit 1
            fi
            ;;
        *)
            echo "Invalid argument. Please use 'Megatron' or 'Pikachu'"
            exit 1
            ;;
    esac
fi