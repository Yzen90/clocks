set_project('clocks')
set_version('0.1.0')

add_configfiles('config.hpp.in')
set_configdir('src')
set_configvar('VERSION_MAJOR', 0)
set_configvar('VERSION_MINOR', 1)
set_configvar('VERSION_PATCH', 0)
set_configvar('NAME', 'Clocks')
set_configvar('DESCRIPTION', 'World clocks plugin for TrafficMonitor.')
set_configvar('AUTHOR', 'Yzen')
set_configvar('C_SYMBOL', '\\xa9')
set_configvar('COPYRIGHT', '2025 Edgar Montiel Cruz')
set_configvar('URL', 'https://github.com/Yzen90/clocks')

set_languages('cxx23')
set_config('toolchain', 'msvc')
add_cxxflags('/Zc:preprocessor')
add_cxxflags('/utf-8')

add_rules('mode.debug', 'mode.release')


add_includedirs('extern/TrafficMonitor/include')
add_includedirs('vcpkg_installed/x64-windows/include')
add_links('WindowsApp')


target('clocks')
  set_kind('shared')

  add_files('src/*.cpp')
  add_files('clocks.rc')

  add_defines('NOMINMAX')

target('tester')
  set_kind('binary')

  add_files('tester.cpp')
  add_deps('clocks')
