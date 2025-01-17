add_rules("mode.debug", "mode.release")

toolchain("gcc13")
    set_kind("standalone")
    set_bindir("/opt/gcc13-2/bin/")
    -- set_toolset("cc", "clang")

target("EventLoopTest")
  set_kind("binary")
  add_includedirs("../include/")
  add_includedirs("../ext/doctest/", "../ext/trompeloeil/")
  add_links("spdlog", "fmt")
  add_files("main.cpp", "EventLoopTest.cpp", "../src/EventLoop.cpp", "../src/Channel.cpp", "../src/Poller.cpp")

target("ChannelTest")
  set_kind("binary")
  add_includedirs("../include/")
  add_includedirs("../ext/doctest/", "../ext/trompeloeil/")
  add_links("spdlog", "fmt")
  add_files("main.cpp", "ChannelTest.cpp", "../src/Channel.cpp")

target("PollerTest")
  set_kind("binary")
  add_includedirs("../include/")
  add_includedirs("../ext/doctest/", "../ext/trompeloeil/")
  add_links("spdlog", "fmt")
  add_files("main.cpp", "PollerTest.cpp", "../src/Poller.cpp")
  set_toolchains("gcc13")
  add_cxxflags("-std=c++23")

target("ThreadTest")
  set_kind("binary")
  add_includedirs("../include/")
  add_includedirs("../ext/doctest/", "../ext/trompeloeil/")
  add_links("spdlog", "fmt")
  add_files("../src/Thread.cpp", "./ThreadTest.cpp")
  set_toolchains("gcc13")
  add_cxxflags("-std=c++23")

target("EventLoopThreadPoolTest")
  set_kind("binary")
  add_includedirs("../include/")
  add_includedirs("../ext/doctest/", "../ext/trompeloeil/")
  add_links("spdlog", "fmt")
  add_files("../src/EventLoopThreadPool.cpp", "../src/EventLoopThread.cpp", "../src/Thread.cpp", "./EventLoopThreadPoolTest.cpp")
  set_toolchains("gcc13")
  add_cxxflags("-std=c++23")
