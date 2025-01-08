add_rules("mode.debug", "mode.release")

target("clocks")
  set_kind("shared")
  add_files("src/ClocksPlugin.cpp")
  add_includedirs("extern/TrafficMonitor/include")
