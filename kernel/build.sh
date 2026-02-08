#!/bin/bash

# --- CLEAN COMMAND ---
if [ "$1" == "clean" ]; then
	echo "Cleaning build directory..."
	rm -rf build/
	echo "Done."
	exit 0
fi

# --- BUILD ---
# configure
echo "Configuring project..."
cmake --preset default

if [ $? -ne 0 ]; then
	echo "CMake configuration failed..."
	exit 1
fi

# build
echo "Building project..."
cmake --build --preset release

if [ $? -eq 0 ]; then
	echo "Build success!"
else
	echo "Build failed..."
	exit 1
fi
