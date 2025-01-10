set_project('clocks')
set_version('0.1.0')

set_languages('cxx23')
set_config('toolchain', 'clang')

add_rules('mode.debug', 'mode.release')

target('clocks')
  set_kind('shared')
  add_files('src/ClocksPlugin.cpp')
  add_includedirs('extern/TrafficMonitor/include')
  add_includedirs('vcpkg_installed/x64-windows/include')

  set_configvar('NAME', 'Clocks')
  set_configvar('DESCRIPTION', 'World clocks plugin for TrafficMonitor.')
  set_configvar('AUTHOR', 'Edgar Montiel Cruz')
  set_configvar('COPYRIGHT', '©2025 Edgar Montiel Cruz')
  set_configvar('URL', 'https://github.com/Yzen90/clocks')
  add_configfiles('src/Config.hpp.in')
  set_configdir('src')
