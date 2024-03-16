includes("../../src/xmake.lua")

target("echoServer")
  set_kind("binary")
  add_includedirs("../../include")
  add_links("spdlog", "fmt")
  add_files("*.cpp")
  add_deps("simple")
