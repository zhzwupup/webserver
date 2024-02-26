add_rules("mode.debug", "mode.release")

target("EventLoopTest")
  set_kind("binary")
  add_includedirs("../include/")
  add_includedirs("../ext/doctest/", "../ext/trompeloeil/")
  add_links("spdlog", "fmt")
  add_files("main.cpp", "EventLoopTest.cpp", "../src/EventLoop.cpp")
