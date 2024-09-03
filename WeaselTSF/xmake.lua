target("WeaselTSF")
  set_kind("shared")
  add_files("./*.cpp", "WeaselTSF.def")
  add_rules("add_rcfiles", "use_weaselconstants")
  add_deps("WeaselIPC", "WeaselUI")
  local fname = ''
  if is_arch("x86") then
    fname = "weasel.dll"
  elseif is_arch("x64") then
    fname = "weaselx64.dll"
  elseif is_arch("arm") then
    fname = "weaselARM.dll"
  elseif is_arch("arm64") then
    fname = "weaselARM64.dll"
  end
  set_filename(fname)

  add_files("$(projectdir)/PerMonitorHighDPIAware.manifest")
  add_shflags("/DEBUG /OPT:REF /OPT:ICF")
  before_build(function(target)
    local target_dir = path.join(target:targetdir(), target:name())
    if not os.exists(target_dir) then
      os.mkdir(target_dir)
    end
    target:set("targetdir", target_dir)
  end)

  after_build(function(target)
    os.cp(path.join(target:targetdir(), "weasel*.dll"), "$(projectdir)/output")
    os.cp(path.join(target:targetdir(), "weasel*.pdb"), "$(projectdir)/output")
  end)
