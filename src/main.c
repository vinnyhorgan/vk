#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#define WIDTH 640
#define HEIGHT 480
#define NAME "Vk by Vinny"

static const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation",
};

#ifdef NDEBUG
static const bool enable_validation_layers = false;
#else
static const bool enable_validation_layers = true;
#endif

static VkResult create_debug_utils_messenger_ext(VkInstance instance,
                                                 const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
                                                 const VkAllocationCallbacks* p_allocator,
                                                 VkDebugUtilsMessengerEXT* p_debug_messenger) {
  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != NULL) {
    return func(instance, p_create_info, p_allocator, p_debug_messenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

static void destroy_debug_utils_messenger_ext(VkInstance instance,
                                              VkDebugUtilsMessengerEXT debug_messenger,
                                              const VkAllocationCallbacks* p_allocator) {
  PFN_vkDestroyDebugUtilsMessengerEXT func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != NULL) {
    func(instance, debug_messenger, p_allocator);
  }
}

static bool check_validation_layer_support() {
  uint32_t layer_count = 0;
  vkEnumerateInstanceLayerProperties(&layer_count, NULL);

  VkLayerProperties* available_layers = malloc(sizeof(VkLayerProperties) * layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

  for (uint32_t i = 0; i < sizeof(validation_layers) / sizeof(validation_layers[0]); i++) {
    bool layer_found = false;

    for (uint32_t j = 0; j < layer_count; j++) {
      if (strcmp(validation_layers[i], available_layers[j].layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      free(available_layers);
      return false;
    }
  }

  free(available_layers);
  return true;
}

static const char* get_required_extensions(uint32_t* count) {
  uint32_t glfw_ext_count = 0;
  const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

  if (glfw_ext_count == 0) {
    printf("failed to get required instance extensions\n");
    return NULL;
  }

  if (enable_validation_layers) {
    *count = glfw_ext_count + 1;
    const char** extensions = malloc(sizeof(const char*) * (*count));
    memcpy(extensions, glfw_exts, sizeof(const char*) * glfw_ext_count);
    extensions[glfw_ext_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    return (const char*)extensions;
  } else {
    *count = glfw_ext_count;
    const char** extensions = malloc(sizeof(const char*) * (*count));
    memcpy(extensions, glfw_exts, sizeof(const char*) * glfw_ext_count);
    return (const char*)extensions;
  }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                     VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                                                     void* p_user_data) {
  printf("validation layer: %s\n", p_callback_data->pMessage);
  return VK_FALSE;
}

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT* create_info) {
  create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info->pfnUserCallback = debug_callback;
}

static void glfw_error_cb(int error_code, const char* description) {
  printf("[GLFW] %d: %s\n", error_code, description);
}

int main() {
  int exit_code = 0;

  GLFWwindow* window = NULL;
  VkInstance instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

  // setup glfw
  glfwSetErrorCallback(glfw_error_cb);

  if (glfwInit() == GLFW_FALSE) {
    printf("failed to initialize glfw\n");
    exit_code = 1;
    goto cleanup;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, NAME, NULL, NULL);
  if (window == NULL) {
    printf("failed to create window\n");
    exit_code = 1;
    goto cleanup;
  }

  // setup vulkan...
  if (enable_validation_layers && !check_validation_layer_support()) {
    printf("validation layers requested, but not available\n");
    exit_code = 1;
    goto cleanup;
  }

  VkApplicationInfo app_info = {0};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = NAME;
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = NAME;
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  uint32_t ext_count = 0;
  const char* exts = get_required_extensions(&ext_count);

  create_info.enabledExtensionCount = ext_count;
  create_info.ppEnabledExtensionNames = (const char* const*)exts;

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
  if (enable_validation_layers) {
    create_info.enabledLayerCount = sizeof(validation_layers) / sizeof(validation_layers[0]);
    create_info.ppEnabledLayerNames = validation_layers;

    populate_debug_messenger_create_info(&debug_create_info);
    create_info.pNext = &debug_create_info;
  } else {
    create_info.enabledLayerCount = 0;
    create_info.pNext = NULL;
  }

  if (vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) {
    printf("failed to create vulkan instance\n");
    exit_code = 1;
    goto cleanup;
  }

  // setup debug messenger
  if (enable_validation_layers) {
    VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
    populate_debug_messenger_create_info(&create_info);

    if (create_debug_utils_messenger_ext(instance, &create_info, NULL, &debug_messenger) != VK_SUCCESS) {
      printf("failed to set up debug messenger\n");
      exit_code = 1;
      goto cleanup;
    }
  }

  // finish window setup
  HWND hwnd = glfwGetWin32Window(window);
  if (hwnd == NULL) {
    printf("failed to get window handle\n");
    exit_code = 1;
    goto cleanup;
  }

  BOOL dark = TRUE;
  DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

  glfwShowWindow(window);

  // main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  // cleanup
cleanup:
  if (exts != NULL) {
    free((void*)exts);
  }

  if (enable_validation_layers && debug_messenger != VK_NULL_HANDLE) {
    destroy_debug_utils_messenger_ext(instance, debug_messenger, NULL);
  }

  if (instance != VK_NULL_HANDLE) {
    vkDestroyInstance(instance, NULL);
  }

  if (window != NULL) {
    glfwDestroyWindow(window);
  }

  if (glfwInit() == GLFW_TRUE) {
    glfwTerminate();
  }

  return exit_code;
}
