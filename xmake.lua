-- 工作区的xmake.lua
set_project("weasel")

-- 定义全局变量
set_xmakever("2.9.4")
set_languages("c++17")
add_defines("UNICODE", "_UNICODE")
add_defines("WINDOWS")
add_defines("MSVC")

add_includedirs("$(projectdir)/include")
-- 设置Boost库的全局路径
boost_root = os.getenv("BOOST_ROOT")
boost_include_path = boost_root
boost_lib_path = boost_root .. "/stage/lib"
add_includedirs(boost_include_path)
add_linkdirs(boost_lib_path)
add_cxflags("/utf-8 /MP /O2 /Oi /Gm- /EHsc /MT /GS /Gy /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline /external:W3 /Gd /TP /FC")
add_ldflags("/TLBID:1 /DYNAMICBASE /NXCOMPAT")

-- 全局ATL lib路径
local atl_lib_dir = ''
dpi_manifest = ''
for include in string.gmatch(os.getenv("include"), "([^;]+)") do
  if atl_lib_dir=='' and include:match(".*ATLMFC\\include\\?$") then
    atl_lib_dir = include:replace("include$", "lib")
    dpi_manifest = include:replace("ATLMFC\\include$", "Include\\Manifest\\PerMonitorHighDPIAware.manifest")
  end
  add_includedirs(include)
end

add_includedirs("$(projectdir)/include/wtl")

if is_arch("x64") then
  add_linkdirs("$(projectdir)/lib64")
  add_linkdirs(atl_lib_dir .. "/x64")
elseif is_arch("x86") then
  add_linkdirs("$(projectdir)/lib")
  add_linkdirs(atl_lib_dir .. "/x86")
elseif is_arch("arm") then
  add_linkdirs(atl_lib_dir .. "/arm")
elseif is_arch("arm64") then
  add_linkdirs(atl_lib_dir .. "/arm64")
end

add_links("atls", "shell32", "advapi32", "gdi32", "user32", "uuid", "ole32")

includes("WeaselIPC", "WeaselUI", "WeaselTSF", "WeaselIME")

if is_arch("x64") or is_arch("x86") then
  includes("RimeWithWeasel", "WeaselIPCServer", "WeaselServer", "WeaselDeployer")
end

if is_arch("x86") then
  includes("WeaselSetup")
end

if is_mode("debug") then
  includes("test/TestWeaselIPC")
  includes("test/TestResponseParser")
else
  add_cxflags("/GL")
  add_ldflags("/LTCG /INCREMENTAL:NO", {force = true})
end

rule("subcmd")
  on_load(function(target)
    target:add("ldflags", "/SUBSYSTEM:CONSOLE")
  end)
rule("subwin")
  on_load(function(target)
    target:add("ldflags", "/SUBSYSTEM:WINDOWS")
  end)

rule("add_rcfiles")
  on_load(function(target)
    target:add("files", path.join(target:scriptdir(), "*.rc"),
      {defines = {"VERSION_MAJOR=" .. os.getenv("VERSION_MAJOR"),
      "VERSION_MINOR=" .. os.getenv("VERSION_MINOR"),
      "VERSION_PATCH=" .. os.getenv("VERSION_PATCH"),
      "FILE_VERSION=" .. os.getenv("FILE_VERSION"),
      "PRODUCT_VERSION=" .. os.getenv("PRODUCT_VERSION")
    }})
  end)
rule("use_weaselconstants")
  on_load(function(target)
    function check_include_weasel_constants_in_dir(dir)
      local files = os.files(path.join(dir, "**.h"))
      table.join2(files, os.files(path.join(dir, "**.cpp")))
      for _, file in ipairs(files) do
        local content = io.readfile(file)
        if content:find('#include%s+"WeaselConstants%.h"') or content:find('#include%s+<WeaselConstants%.h>') then
          return true
        end
      end
      return false
    end
    if check_include_weasel_constants_in_dir(target:scriptdir()) then
      target:add("defines", {
        "VERSION_MAJOR=" .. os.getenv("VERSION_MAJOR"),
        "VERSION_MINOR=" .. os.getenv("VERSION_MINOR"),
        "VERSION_PATCH=" .. os.getenv("VERSION_PATCH"),
        "FILE_VERSION=" .. os.getenv("FILE_VERSION"),
        "PRODUCT_VERSION=" .. os.getenv("PRODUCT_VERSION")
    })
    end
  end)
