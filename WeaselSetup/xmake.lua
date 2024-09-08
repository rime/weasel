target("WeaselSetup")
  set_kind("binary")
  add_files("./*.cpp")
  add_rules("add_rcfiles", "use_weaselconstants", "subwin")
  add_links("imm32", "kernel32")

  set_policy("windows.manifest.uac", "admin")
  add_files("$(projectdir)/PerMonitorHighDPIAware.manifest")
  add_ldflags("/DEBUG /OPT:REF /OPT:ICF /LARGEADDRESSAWARE /ERRORREPORT:QUEUE")

  before_build(function(target)
    local target_dir = path.join(target:targetdir(), target:name())
    if not os.exists(target_dir) then
      os.mkdir(target_dir)
    end
    target:set("targetdir", target_dir)
    local level = target:policy("windows.manifest.uac")
    if level then
      local level_maps = {
        invoker = "asInvoker",
        admin = "requireAdministrator",
        highest = "highestAvailable"
      }
      assert(level_maps[level], "unknown uac level %s, please set invoker, admin or highest", level)
      local ui = target:policy("windows.manifest.uac.ui") or false
      target:add("ldflags", "/manifest:embed", {("/manifestuac:level='%s' uiAccess='%s'"):format(level_maps[level], ui)}, {force = true, expand = false})
    end
  end)

  after_build(function(target)
      os.cp(path.join(target:targetdir(), "WeaselSetup.exe"), "$(projectdir)/output")
      os.cp(path.join(target:targetdir(), "WeaselSetup.pdb"), "$(projectdir)/output")
  end)
