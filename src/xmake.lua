add_rules("mode.debug", "mode.release")

target("simple")
  set_kind("shared")
  add_includedirs("../include/")
  add_links("spdlog", "fmt")
  add_files("EventLoop.cpp")
