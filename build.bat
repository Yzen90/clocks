@echo off

vcpkg install --triplet x86-windows --no-print-usage
xmake config --arch=x86 --mode=release
xmake build schemagen
xmake

vcpkg install --triplet x64-windows --no-print-usage
xmake config --arch=x64 --mode=release
xmake build schemagen
xmake
