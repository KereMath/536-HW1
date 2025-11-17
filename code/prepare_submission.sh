#!/bin/bash

# Prepare submission for CENG 536 HW1

echo "======================================"
echo "Preparing CENG 536 HW1 Submission"
echo "======================================"

# Clean first
echo "Cleaning previous builds..."
make clean

# Remove old submission
rm -rf hw1_submission

# Create hw1 directory
mkdir -p hw1_submission/hw1

# Copy necessary files
echo "Copying source files..."
cp hw1.c hw1_submission/hw1/
cp heap.c hw1_submission/hw1/

# Copy makefile (ensure lowercase!)
if [ -f "makefile" ]; then
    cp makefile hw1_submission/hw1/
elif [ -f "Makefile" ]; then
    cp Makefile hw1_submission/hw1/makefile  # Rename to lowercase!
else
    echo "❌ ERROR: No makefile found!"
    exit 1
fi

# Optional: Copy header if exists
if [ -f "heap.h" ]; then
    cp heap.h hw1_submission/hw1/
fi

# Go to submission directory
cd hw1_submission

# Create tar.gz
echo "Creating tar.gz archive..."
tar czf kerem_hw1.tar.gz hw1/

# Verify tar contents
echo ""
echo "Archive contents:"
tar tzf kerem_hw1.tar.gz

# Test extraction
echo ""
echo "======================================"
echo "Testing submission..."
echo "======================================"
mkdir test_extract
cd test_extract

# ✅ EXACTLY AS HOCA DOES IT!
echo "Extracting with EXACT hoca command..."
echo "Running: tar xzvf ../kerem_hw1.tar.gz"
tar xzvf ../kerem_hw1.tar.gz  # ✅ xzvf (with verbose!)

# ✅ EXACTLY AS HOCA DOES IT!
echo ""
echo "Running: pushd hw1"
pushd hw1

# ✅ EXACTLY AS HOCA DOES IT!
echo "Running: make hw1"
make hw1

# Check result
if [ -f "./hw1" ]; then
    echo ""
    echo "✅ SUCCESS! Binary created successfully"
    echo "✅ Binary location: $(pwd)/hw1"
    
    # Test execution
    echo ""
    echo "Testing execution..."
    ./hw1 2>&1 | head -5
    
else
    echo ""
    echo "❌ ERROR: hw1 binary not found!"
    echo "❌ Build failed!"
    popd
    exit 1
fi

# ✅ EXACTLY AS HOCA DOES IT!
echo ""
echo "Running: popd"
popd

# Return to original directory
cd ../..

# Final message
echo ""
echo "======================================"
echo "✅ SUBMISSION READY!"
echo "======================================"
echo "Submission file: hw1_submission/kerem_hw1.tar.gz"
echo "File size: $(du -h hw1_submission/kerem_hw1.tar.gz | cut -f1)"
echo ""
echo "Upload 'kerem_hw1.tar.gz' to odtuclass"
echo ""
echo "Archive structure:"
tar tzf hw1_submission/kerem_hw1.tar.gz
echo ""