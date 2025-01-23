@echo off
git submodule init
git pull --recurse-submodules
vcpkg install --no-print-usage
npm install
yq --version > nul 2>&1 || winget install MikeFarah.yq
clang --version > nul 2>&1 || winget install LLVM.LLVM
