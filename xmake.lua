set_project('clocks')

clocks_version_major = 0
clocks_version_minor = 3
clocks_version_patch = 0
set_version(clocks_version_major .. '.' .. clocks_version_minor .. '.' .. clocks_version_patch)

add_configfiles('config.hpp.in')
set_configdir('src')
set_configvar('VERSION_MAJOR', clocks_version_major)
set_configvar('VERSION_MINOR', clocks_version_minor)
set_configvar('VERSION_PATCH', clocks_version_patch)
set_configvar('NAME', 'Clocks')
set_configvar('DESCRIPTION', 'World clocks plugin for TrafficMonitor.')
set_configvar('AUTHOR', 'Yzen')
set_configvar('C_SYMBOL', '\\xa9')
set_configvar('COPYRIGHT', '2025 Edgar Montiel Cruz')
set_configvar('URL', 'https://github.com/Yzen90/clocks')

set_languages('cxx23')
set_config('toolchain', 'clang')

add_rules('mode.debug', 'mode.release')
add_rules('plugin.compile_commands.autoupdate', {outputdir = 'build'})


add_defines('NOMINMAX')
add_includedirs('extern/TrafficMonitor/include')
add_includedirs('vcpkg_installed/$(arch)-$(plat)/include')
add_links('WindowsApp')

includes('src/i18n')


target('clocks')
  set_kind('shared')

  add_files('src/*.cpp')
  add_files('src/i18n/l10n.cpp')
  add_files('clocks.rc')

  add_defines('AUTO_INITIALIZE_EASYLOGGINGPP')
  add_defines('ELPP_DEFAULT_LOG_FILE="plugins/clocks.dll.log"')
  add_defines('XXH_INLINE_ALL')

  if is_mode('release') then
    add_defines('GLZ_ALWAYS_INLINE=[[clang::always_inline]] inline')
  else
    add_defines('NOT_RELEASE_MODE')
  end

  add_rules('i18n-codegen', 'i18n-validation')


target('tester')
  set_kind('binary')

  add_files('tester.cpp')
  add_deps('clocks')

  if is_mode('release') then
    set_enabled(false)
  end
