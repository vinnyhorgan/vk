workspace "vk"
  configurations { "debug", "release" }
  architecture "x64"
  location "build"

project "vk"
  kind "ConsoleApp"
  language "C"
  staticruntime "on"

  targetdir "%{wks.location}/bin/%{cfg.buildcfg}"
  objdir "%{wks.location}/obj/%{cfg.buildcfg}"

  files {
    "src/**.h",
    "src/**.c",
  }

  filter "configurations:debug"
    defines { "DEBUG" }
    symbols "on"

  filter "configurations:release"
    defines { "NDEBUG" }
    optimize "on"
