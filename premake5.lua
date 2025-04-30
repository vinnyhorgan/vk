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

  if os.host() == "windows" then
    local vulkan_sdk = os.getenv("VULKAN_SDK")
    if not vulkan_sdk then
      error("VULKAN_SDK environment variable is not set")
    end

    includedirs {
      path.join(vulkan_sdk, "Include"),
    }

    libdirs {
      path.join(vulkan_sdk, "Lib"),
    }
  end

  filter "system:windows"
    defines {
      "_CRT_SECURE_NO_WARNINGS",
      "_GLFW_WIN32",
    }

    files { "vk.rc" }

    links {
      "vulkan-1",
      "dwmapi",
    }

  filter "system:linux"
    defines {
      "_GLFW_X11",
    }

    links {
      "m",
      "vulkan",
    }

  filter "configurations:debug"
    defines { "DEBUG" }
    symbols "on"

  filter "configurations:release"
    defines { "NDEBUG" }
    optimize "on"
