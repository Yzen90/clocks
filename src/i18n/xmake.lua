
local schema_source = 'src/i18n/schema.hpp'
local schemagen_target = 'schemagen'

target(schemagen_target)
  set_kind('binary')

  add_files('schemagen.cpp')

  add_defines('GLZ_ALWAYS_INLINE=[[clang::always_inline]] inline')
  add_rules('i18n-shared')

  after_build(function ()
    local depend = import('../../modules/depend')
    local arch = vformat('$(arch)')
    
    if depend.is_changed(schema_source, schemagen_target, arch) then
      depend.save(schema_source, schemagen_target, arch)
    end
  end)


target('makeheaders')
  set_kind('binary')

  add_files('$(projectdir)/extern/makeheaders/makeheaders.c')

  add_includedirs('$(projectdir)/dummy')
  add_cflags('-Wno-deprecated-declarations', {force = true})
  add_defines('WIN32')
  add_rules('i18n-shared')


rule('i18n-shared')
  on_load(function (target)
    target:set('default', false)
    target:set('symbols', 'hidden')
    target:set('optimize', 'fastest')
    target:set('strip', 'all')
    target:set('targetdir', '$(buildir)/$(plat)/$(arch)')
  end)


local codegen_target = 'src/i18n/locales.cpp'
local schema = 'l10n.schema.json'
local localization_files = 'i18n/*.json'

rule('i18n-codegen')
  on_load(function (target)
    if not os.isfile(codegen_target) then
      io.writefile(codegen_target, '')
    end

    target:add('files', codegen_target)
  end)

  before_build(function (target)
    import('core.base.option')
    local depend = import('../../modules/depend')

    local makeheaders = '$(buildir)/$(plat)/$(arch)/makeheaders.exe'
    if not os.isfile(makeheaders) then
      raise('makeheaders not found, run deps script.')
    end

    local localizations = os.files(localization_files)

    if option.get('rebuild') or not os.isfile('src/i18n/locales.hpp')
      or depend.any_files_changed(localizations, target) then
      
      print('Running i18n codegen...')
      os.rm(codegen_target)
      local locales = io.open(codegen_target, 'a')

      for _, l10n in ipairs(localizations) do

        local source = 'locale/' .. path.basename(l10n);
        os.execv('yq', {'-o=json', '-I=0'}, {stdin = l10n, stdout = source})
        locales:write(os.iorunv('$(env PROGRAMFILES)/Git/usr/bin/xxd', {'-i', source}))

        if depend.is_changed(l10n, target) then
          depend.save(l10n, target)
        end
      end

      locales:close()
      os.run(makeheaders .. ' ' .. codegen_target)
    end


    if depend.is_changed(schema_source, schemagen_target, vformat('$(arch)')) then
      raise('schemagen needs rebuild, run deps script.')
    end

    if option.get('rebuild') or not os.isfile(schema) or depend.is_changed(schema_source, target) then
      print('Running i18n schema codegen...')
      local temp_file = vformat('$(buildir)/tmp.json')

      os.run('$(buildir)/$(plat)/$(arch)/schemagen.exe')
      os.execv('node_modules/node-jq/bin/jq', {'-f', 'require-all-fields.jq', schema}, {stdout = temp_file})
      os.run('node node_modules/lec/cmd-runner.js ' .. temp_file .. ' --eolc LF')
      os.mv(temp_file, schema)

      depend.save(schema_source, target)
    end
  end)


rule('i18n-validation')
  add_deps('i18n-codegen')

  before_build(function (target)
    local depend = import('../../modules/depend')

    local localizations = os.files(localization_files)
    table.insert(localizations, schema)

    local tag = 'validation'

    if depend.any_files_changed(localizations, target, tag) then
      print('Running i18n validation...')
      os.run('node node_modules/@jirutka/ajv-cli/lib/main.js validate --strict-schema -s ' .. schema .. ' ' .. localization_files)

      depend.save_only_changed(localizations, target, tag)
    end
  end)
