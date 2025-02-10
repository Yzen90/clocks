@echo off
vcpkg install --triplet x64-windows-static --no-print-usage
yq --version > nul 2>&1 || winget install MikeFarah.yq
clang --version > nul 2>&1 || winget install LLVM.LLVM
xmake config --arch=x64
xmake build schemagen
git submodule init
git pull --recurse-submodules
npm install
pip install fonttools
