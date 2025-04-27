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

  local vulkan_sdk = os.getenv("VULKAN_SDK")
  if not vulkan_sdk then
    error("VULKAN_SDK environment variable is not set")
  end

  includedirs {
    "vendor/glfw/include",
    path.join(vulkan_sdk, "Include"),
  }

  libdirs {
    path.join(vulkan_sdk, "Lib"),
  }

  links {
    "vulkan-1",
  }

  filter "system:windows"
    defines {
      "_CRT_SECURE_NO_WARNINGS",
      "_GLFW_WIN32",
    }

    links { "dwmapi" }

  filter "configurations:debug"
    defines { "DEBUG" }
    symbols "on"

  filter "configurations:release"
    defines { "NDEBUG" }
    optimize "on"
