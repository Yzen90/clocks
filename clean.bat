@echo off
del /q src\config.hpp
rd /q /s locale
del /q src\i18n\locales.*
rd /q /s build
