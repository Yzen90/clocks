@echo off
vcpkg install --triplet x86-windows --no-print-usage
xmake config --arch=x86
xmake build schemagen
xmake build makeheaders
