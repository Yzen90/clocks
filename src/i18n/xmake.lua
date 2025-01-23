target('schema-generate')
  set_kind('binary')

  add_files('schema-generate.cpp')

  add_defines('GLZ_ALWAYS_INLINE=[[clang::always_inline]] inline')
  add_rules('i18n-shared')


target('makeheaders')
  set_kind('binary')

  add_files('$(projectdir)/extern/makeheaders/makeheaders.c')

  add_includedirs('$(projectdir)//dummy')
  add_cflags('-Wno-deprecated-declarations')
  add_defines('WIN32')
  add_rules('i18n-shared')


rule('i18n-shared')
  on_config(function (target)
    target:set('symbols', 'hidden')
    target:set('optimize', 'fastest')
    target:set('strip', 'all')
  end)


rule('i18n-codegen')
  before_build(function (target)
    import('core.base.option')
    local depend = import('../../modules/depend')

    local localizations = os.files('i18n/*.json')
    local codegen_target = 'src/i18n/locales.cpp'

    if option.get('rebuild') or not os.isfile(codegen_target)
        or not os.isfile('src/i18n/locales.hpp') or depend.any_files_changed(localizations, target) then
      
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
      os.run('build/$(plat)/$(arch)/' .. get_config('mode') .. '/makeheaders ' .. codegen_target)
    end
  end)

  before_link(function (target)
    import('core.base.option')
    local depend = import('../../modules/depend')

    local schema_source = 'src/i18n/schema.hpp'
    if option.get('rebuild') or depend.is_changed(schema_source, target) then
      print('Running i18n schema codegen...')

      os.run('build/$(plat)/$(arch)/' .. get_config('mode') .. '/schema-generate')
      os.execv('node_modules/node-jq/bin/jq', {'-f', 'require-all-fields.jq', 'i18n.schema.json'}, {stdout = 'build/tmp.json'})
      os.mv('build/tmp.json', 'i18n.schema.json')

      depend.save(schema_source, target)
    end
  end)


rule('i18n-validation')
  add_deps('i18n-codegen')

  before_link(function (target)
    local depend = import('../../modules/depend')

    local dependencies = os.files('i18n/*.json')
    table.insert(dependencies, 'i18n.schema.json')

    local tag = 'validation'

    if depend.any_files_changed(dependencies, target, tag) then
      print('Running i18n validation...')
      os.run('node node_modules/ajv-cli/dist/index.js -s i18n.schema.json -d i18n/*.json --errors=text')

      depend.save_only_changed(dependencies, target, tag)
    end
  end)
