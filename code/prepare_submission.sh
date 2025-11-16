#!/bin/bash

# Prepare submission for CENG 536 HW1

echo "Preparing submission..."

# Clean first
make clean

# Create hw1 directory
rm -rf hw1_submission
mkdir -p hw1_submission/hw1

# Copy only necessary files
cp hw1.c hw1_submission/hw1/
cp heap.c hw1_submission/hw1/
cp Makefile hw1_submission/hw1/

# Go to submission directory
cd hw1_submission

# Create tar.gz
tar czf kerem_hw1.tar.gz hw1/

# Test extraction
echo "Testing extraction..."
mkdir test_extract
cd test_extract
tar xzf ../kerem_hw1.tar.gz
cd hw1
make hw1

if [ -f "./hw1" ]; then
    echo "✅ SUCCESS! hw1 binary created in current directory"
    echo "✅ Submission file ready: hw1_submission/kerem_hw1.tar.gz"
else
    echo "❌ ERROR: hw1 binary not found!"
fi

cd ../../..