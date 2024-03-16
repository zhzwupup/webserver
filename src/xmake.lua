add_rules("mode.debug", "mode.release")

toolchain("gcc13")
    set_kind("standalone")
    set_bindir("/opt/gcc13-2/bin/")

target("simple")
  set_kind("shared")
  add_includedirs("../include/")
  add_links("spdlog", "fmt")
  add_files("*.cpp")
  set_toolchains("gcc13")
  add_cxxflags("-std=c++23")
