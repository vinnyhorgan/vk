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

    "vendor/glfw/src/*.h",
    "vendor/glfw/src/*.c",
  }

  includedirs {
    "vendor/glfw/include",
  }

  filter "system:windows"
    defines {
      "_CRT_SECURE_NO_WARNINGS",
      "_GLFW_WIN32",
    }

  filter "configurations:debug"
    defines { "DEBUG" }
    symbols "on"

  filter "configurations:release"
    defines { "NDEBUG" }
    optimize "on"
