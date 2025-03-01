set_project('clocks')

clocks_version_major = 0
clocks_version_minor = 9
clocks_version_patch = 0
set_version(clocks_version_major .. '.' .. clocks_version_minor .. '.' .. clocks_version_patch)

add_configfiles('src/config.hpp.in')
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


add_includedirs('extern/TrafficMonitor/include')
add_includedirs('vcpkg_installed/$(arch)-$(plat)-static/include')
add_linkdirs('vcpkg_installed/$(arch)-$(plat)-static/lib')
add_syslinks('WindowsApp', 'User32')
add_defines('NOMINMAX', 'WIN32_LEAN_AND_MEAN', 'UNICODE', '_UNICODE', 'IMGUI_USE_WCHAR32')

includes('src/i18n')


target('clocks')
  set_kind('shared')

  add_files('src/*.cpp', 'src/**/*.cpp', 'src/clocks.rc')
  remove_files('src/i18n/schemagen.cpp')
  add_files('vcpkg_installed/$(arch)-$(plat)-static/include/easylogging++.cc')

  add_packages('sdl')
  add_deps('imgui')

  add_syslinks('kernel32', 'gdi32', 'winmm', 'imm32', 'ole32', 'oleaut32', 'version', 'uuid', 'advapi32', 'setupapi', 'shell32')

  add_cxxflags('-Wno-c23-extensions', '--embed-dir=assets', '--embed-dir=locale')

  add_defines('AUTO_INITIALIZE_EASYLOGGINGPP', 'ELPP_NO_DEFAULT_LOG_FILE', 'XXH_INLINE_ALL')

  if is_mode('release') then
    add_defines('GLZ_ALWAYS_INLINE=[[clang::always_inline]] inline')
  else
    add_defines('NOT_RELEASE_MODE')
  end

  add_rules('i18n-codegen', 'i18n-validation')


target('imgui')
  set_kind('static')
  add_files('extern/imgui/*.cpp')
  add_files('extern/imgui/backends/imgui_impl_sdl3.cpp')
  add_files('extern/imgui/backends/imgui_impl_sdlgpu3.cpp')
  add_files('extern/imgui/misc/freetype/imgui_freetype.cpp')
  add_files('extern/imgui/misc/cpp/imgui_stdlib.cpp')
  add_files('extern/imgui_toggle/*.cpp')
  add_includedirs('extern/imgui', {public = true})
  add_includedirs('extern/imgui/misc/cpp', {public = true})
  add_includedirs('extern/imgui_toggle', {public = true})

  add_packages('sdl')
  add_links('freetype')

  add_defines('IMGUI_DISABLE_DEFAULT_FONT', 'IMGUI_ENABLE_FREETYPE')


package('sdl')
  add_deps('cmake')
  set_sourcedir('extern/SDL')
  add_versions("3.2.5", "deadfe0c98c3d0d047a0ef6d4effa48eb99a4afb")
  on_install(function (package) 
    import('package.tools.cmake').install(package, {
      shared = false,
      SDL_SHARED = 'OFF',
      SDL_STATIC = 'ON'
    })
  end)
package_end()
add_requires('sdl')


target('tester')
  set_kind('binary')

  add_files('tester.cpp')
  add_deps('clocks')

  if is_mode('release') then
    set_enabled(false)
  end
