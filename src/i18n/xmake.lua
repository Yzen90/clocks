local schema_source = 'src/i18n/schema.hpp'
local schemagen_target = 'schemagen'

target(schemagen_target)
  set_kind('binary')

  add_files('schemagen.cpp')

  add_defines('GLZ_ALWAYS_INLINE=[[clang::always_inline]] inline')

  set_default(false)
  set_symbols('hidden')
  set_optimize('fastest')
  set_strip('all')
  set_targetdir('$(buildir)/$(plat)/$(arch)')

  after_build(function ()
    local depend = import('../../modules/depend')
    local arch = vformat('$(arch)')
    
    if depend.is_changed(schema_source, schemagen_target, arch) then
      depend.save(schema_source, schemagen_target, arch)
    end
  end)


local schema = 'l10n.schema.json'
local localization_files = 'i18n/*.json'

rule('i18n-codegen')
  before_build(function (target)
    import('core.base.option')
    local depend = import('../../modules/depend')

    local localizations = os.files(localization_files)

    if option.get('rebuild') or depend.any_files_changed(localizations, target) then
      print('Running l10n minify...')

      for _, l10n in ipairs(localizations) do
        local source = 'locale/' .. path.filename(l10n);
        os.execv('yq', {'-o=json', '-I=0'}, {stdin = l10n, stdout = source})

        if depend.is_changed(l10n, target) then
          depend.save(l10n, target)
        end
      end
    end

    if depend.is_changed(schema_source, schemagen_target, vformat('$(arch)')) then
      raise('schemagen needs rebuild, run deps script.')
    end

    if option.get('rebuild') or not os.isfile(schema) or depend.is_changed(schema_source, target) then
      print('Running l10n schema codegen...')
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
    import('core.base.option')
    local depend = import('../../modules/depend')

    local localizations = os.files(localization_files)
    table.insert(localizations, schema)

    local tag = 'validation'

    if option.get('rebuild') or depend.any_files_changed(localizations, target, tag) then
      print('Running i18n validation...')
      os.run('node node_modules/@jirutka/ajv-cli/lib/main.js validate --strict-schema -s ' .. schema .. ' ' .. localization_files)

      depend.save_only_changed(localizations, target, tag)
    end
  end)


rule('i18n-fonts')
  before_build(function (target)
    import('core.base.option')
    local depend = import('../../modules/depend')

    local dependencies = os.files(localization_files)
    table.join2(dependencies, os.files('fonts/*.ttf'))
    table.insert(dependencies, 'charsets.py')

    local tag = 'fonts'

    if option.get('rebuild') or depend.any_files_changed(dependencies, target, tag) then
      print('Running i18n font stripping...')
 
      os.run('py charsets.py')

      for _, charset_file in ipairs(os.files('build/*.charset')) do
        local font_file = path.basename(charset_file) .. '.ttf'

        os.runv('pyftsubset', {
          'fonts/' .. font_file,
          '--text-file=' .. charset_file,
          '--output-file=assets/' .. font_file,
          '--layout-features=*', '--glyph-names', '--symbol-cmap', '--legacy-cmap', '--notdef-glyph', '--recommended-glyphs'
        })

        depend.save_only_changed(dependencies, target, tag)
      end
    end
  end)
