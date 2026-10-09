target("WeaselUI")
  set_kind("static")
  add_links("user32", "Shlwapi", "dwmapi","shcore", "gdi32", "Shell32", "d2d1", "dwrite", 'dxgi', 'd3d11', 'dcomp')
  add_links('windowscodecs', 'ole32')
  set_languages("c++17")
  add_files("./*.cpp")
  add_defines('INITGUID')
  if is_plat('windows') then
    add_cxflags("/utf-8")
    add_cxflags("/Zi")
    add_cxflags("/FS")
    add_cxflags("-Fd$(builddir)/$(targetname).pdb")
    add_ldflags("/DEBUG", {force = true})
  elseif is_plat('mingw') then
    add_cxflags("-g", {force = true})
    add_ldflags('-static-libgcc -static-libstdc++ -static', {force=true})
    add_ldflags("-municode -mwindows", {force = true})
  end

