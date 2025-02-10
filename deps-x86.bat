@echo off
vcpkg install --triplet x86-windows-static --no-print-usage
xmake config --arch=x86
xmake build schemagen
