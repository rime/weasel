target("WeaselServer")
  set_kind("binary")
  add_files("./*.cpp")
  add_rules("add_rcfiles", "subwin")
  add_links("imm32", "kernel32", "rime")
  add_deps("WeaselUI", "WeaselIPC", "RimeWithWeasel", "WeaselIPCServer")

  add_files("$(projectdir)/PerMonitorHighDPIAware.manifest")
  add_ldflags("/DEBUG /OPT:REF /OPT:ICF /LARGEADDRESSAWARE /ERRORREPORT:QUEUE")
  set_policy("windows.manifest.uac", "invoker")
  before_build(function(target)
    local target_dir = path.join(target:targetdir(), target:name())
    if not os.exists(target_dir) then
      os.mkdir(target_dir)
    end
    target:set("targetdir", target_dir)
  end)
  after_build(function(target)
    if is_arch("x86") then
      os.cp(path.join(target:targetdir(), "WeaselServer.exe"), "$(projectdir)/output/Win32")
      os.cp(path.join(target:targetdir(), "WeaselServer.pdb"), "$(projectdir)/output/Win32")
    else
      os.cp(path.join(target:targetdir(), "WeaselServer.exe"), "$(projectdir)/output")
      os.cp(path.join(target:targetdir(), "WeaselServer.pdb"), "$(projectdir)/output")
    end
  end)

