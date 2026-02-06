target("WeaselUI")
  set_kind("static")
  add_files("./*.cpp")
  add_cxflags("/openmp")  -- Enable OpenMP for parallel processing

