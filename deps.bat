@echo off
vcpkg install --no-print-usage
yq --version > nul 2>&1 || winget install MikeFarah.yq
clang --version > nul 2>&1 || winget install LLVM.LLVM
xmake build schemagen
xmake build makeheaders
git submodule init
git pull --recurse-submodules
npm install
