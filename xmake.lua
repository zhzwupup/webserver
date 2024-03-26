add_rules("mode.debug", "mode.release")

toolchain("gcc13")
    set_kind("standalone")
    set_bindir("/opt/gcc13-2/bin/")

target("tinyserver")
  set_kind("shared")
  add_includedirs("./include/")
  add_links("spdlog", "fmt")
  add_files("./src/*.cpp")
  set_toolchains("gcc13")
  add_cxxflags("-std=c++23")

target("echoServer")
  set_kind("binary")
  add_includedirs("./include/")
  add_links("spdlog", "fmt")
  add_files("./example/echoServer/echoServer.cpp")
  add_deps("tinyserver")

-- target("httpparser")
--   set_kind("shared")
--   add_files("./example/httpServer/ext/http_parser.c")
--   set_toolchains("gcc13")
  
target("httpServer")
  set_kind("binary")
  add_includedirs("./include", "./example/httpServer/ext/")
  add_links("spdlog", "fmt")
  add_files("./example/httpServer/*.cpp", "./example/httpServer/ext/http_parser.c")
  -- add_deps("tinyserver", "httpparser")
  add_deps("tinyserver")
  set_toolchains("gcc13")
  add_cxxflags("-std=c++23")

target("http")
  set_kind("shared")
  add_includedirs("./include", "./example/httpServer/ext/")
  add_links("spdlog", "fmt")
  add_files("./example/httpServer/*.cpp", "./example/httpServer/ext/http_parser.c")
  remove_files("./example/httpServer/main.cpp")
  -- add_deps("tinyserver", "httpparser")
  add_deps("tinyserver")
  set_toolchains("gcc13")
  add_cxxflags("-std=c++23")
--
--
target("wsServer")
  set_kind("binary")
  add_includedirs("./include", "./example/httpServer/", "./example/httpServer/ext/")
  add_links("spdlog", "fmt")
  add_files("./example/websocketServer/*.cpp")
  add_deps("http")
  set_toolchains("gcc13")
  add_cxxflags("-std=c++23")


includes("./tests/xmake.lua")
