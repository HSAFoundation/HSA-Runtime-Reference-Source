#!/bin/bash

clang -O0 -emit-llvm -include /usr/local/include/clc/clc.h  -Dcl_clang_storage_class_specifiers -target r600--amdhsa-amdgpu -mcpu=kaveri -c $1 -o $1.bc

llvm-link $1.bc /usr/local/lib/clc/kaveri-r600--.bc -o $1-linked.bc

opt -O2 $1-linked.bc -o $1-opt.bc

llc -mtriple=r600--amdhsa-amdgpu -mcpu=kaveri $1-opt.bc -filetype=obj -o $1.o
