function _get_dependfile_path(file, target, tag)
  if tag then
    return 'build/.deps/' .. target:name() .. '/' .. file .. '.' .. tag .. '.d'
  end

  return 'build/.deps/' .. target:name() .. '/' .. file .. '.d'
end

function any_files_changed(files, target, tag)
  for _, file in ipairs(files) do
    if is_changed(file, target, tag) then
      return true
    end
  end

  return false
end

function is_changed(file, target, tag)
  local dependfile = _get_dependfile_path(file, target, tag)

  if os.isfile(dependfile) then
    local dependinfo = try { function() return io.load(dependfile) end }
    if type(dependinfo.lastmtime) == 'number' then
      if os.mtime(file) == dependinfo.lastmtime then
        return false
      end
    end
  end

  return true
end

function save(file, target, tag)
  local mtime = os.mtime(file)
  if mtime == 0 then
    raise('[depend.save] ' .. file .. ' does not exists.')
  end

  io.save(_get_dependfile_path(file, target, tag), {files = {file}, lastmtime = mtime})
end

function save_only_changed(files, target, tag)
  for _, file in ipairs(files) do
    if is_changed(file, target, tag) then
      save(file, target, tag)
    end
  end
end
