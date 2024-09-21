#!/bin/bash
set -e 

export PICO_PATH=${HOME}/stack/pico
export PICO_SDK_PATH=${PICO_PATH}/pico-sdk
export PICO_EXTRAS_PATH=${PICO_PATH}/pico-extras
export GVGA_HOME=${HOME}/git/GVga

clear
env | grep PICO
rm -rf build
mkdir -p build
cd build

# It's important to include PICO_COPY_TO_RAM=1 
# since it increases performance for some of the applications 
# because they use SSI in fast DMA streaming mode.

# cmake ..
cmake -DPICO_COPY_TO_RAM=1  ..
# cmake.sh .. ### DEBUGER OUTPUT ENABLED

#make VERBOSE=1
make -j4
