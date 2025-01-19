@echo off
git submodule init
git pull --recurse-submodules
vcpkg install --no-print-usage
