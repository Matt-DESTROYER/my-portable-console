#!/bin/bash

set -e

echo "--- INSTALLING PICO DEVELOPMENT TOOLS (LINUX) ---"

echo "Updating package lists..."
sudo apt update

echo "Installing dependencies..."
sudo apt install -y cmake \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    build-essential \
    ninja-build \
    python3 \
    git

echo "Installation complete!"
