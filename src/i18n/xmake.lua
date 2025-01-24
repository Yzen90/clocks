target('schemagen')
  set_kind('binary')

  add_files('schemagen.cpp')

  add_defines('GLZ_ALWAYS_INLINE=[[clang::always_inline]] inline')
  add_rules('i18n-shared')


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


local codegen_target = "src/i18n/locales.cpp"

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

    local localizations = os.files('i18n/*.json')

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


    local schemagen = '$(buildir)/$(plat)/$(arch)/schemagen.exe'
    if not os.isfile(schemagen) then
      raise('schemagen not found, run deps script.')
    end

    local schema_source = 'src/i18n/schema.hpp'
    if option.get('rebuild') or depend.is_changed(schema_source, target) then
      print('Running i18n schema codegen...')
      local temp_file = vformat('$(buildir)/tmp.json')
      local target_file = 'l10n.schema.json'

      os.run(schemagen)
      os.execv('node_modules/node-jq/bin/jq', {'-f', 'require-all-fields.jq', target_file}, {stdout = temp_file})
      os.mv(temp_file, target_file)

      depend.save(schema_source, target)
    end
  end)


rule('i18n-validation')
  add_deps('i18n-codegen')

  before_build(function (target)
    local depend = import('../../modules/depend')

    local localizations = 'i18n/*.json'
    local schema = 'l10n.schema.json'

    local dependencies = os.files(localizations)
    table.insert(dependencies, schema)

    local tag = 'validation'

    if depend.any_files_changed(dependencies, target, tag) then
      print('Running i18n validation...')
      os.run('node node_modules/ajv-cli/dist/index.js -s ' .. schema .. ' -d ' .. localizations .. ' --errors=text')

      depend.save_only_changed(dependencies, target, tag)
    end
  end)
