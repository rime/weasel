target("TestWeaselIPC")
  set_kind("binary")
  add_files("./*.cpp")
  add_deps("WeaselIPC", "WeaselIPCServer")
  add_rules("subcmd")
  before_build(function(target)
    local target_dir = path.join(target:targetdir(), target:name())
    if not os.exists(target_dir) then
      os.mkdir(target_dir)
    end
    target:set("targetdir", target_dir)
  end)
