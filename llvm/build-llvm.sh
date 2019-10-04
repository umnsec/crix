#!/bin/bash -e


# LLVM version: 10.0.0 (as of 09/12/2019)

ROOT=$(git rev-parse --show-toplevel)
cd $ROOT/llvm/llvm-project

if [ ! -d "build" ]; then
  mkdir build
fi

cd build
cmake -DLLVM_TARGET_ARCH="ARM;X86;AArch64" -DLLVM_TARGETS_TO_BUILD="ARM;X86;AArch64" -DCMAKE_BUILD_TYPE=Release \
			-DLLVM_ENABLE_PROJECTS="clang" -G "Unix Makefiles" ../llvm
make -j8

if [ ! -d "$ROOT/llvm/llvm-project/prefix" ]; then
  mkdir $ROOT/llvm/llvm-project/prefix
fi

cmake -DCMAKE_INSTALL_PREFIX=$ROOT/llvm/llvm-project/prefix -P cmake_install.cmake
