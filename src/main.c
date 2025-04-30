#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

#define WIDTH 640
#define HEIGHT 480
#define NAME "Vk by Vinny"

#define FAIL_AND_CLEANUP(msg)      \
  do {                             \
    printf("[ERROR] %s\n", (msg)); \
    exit_code = 1;                 \
    goto cleanup;                  \
  } while (0)

static const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation",
};

#ifdef NDEBUG
static const bool enable_validation_layers = false;
#else
static const bool enable_validation_layers = true;
#endif

typedef struct {
  bool graphics_family_has_value;
  uint32_t graphics_family;

  bool present_family_has_value;
  uint32_t present_family;
} QueueFamilyIndices;

// utility functions
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

  if (vkEnumerateInstanceLayerProperties(&layer_count, NULL) != VK_SUCCESS) {
    return false;
  }

  VkLayerProperties* available_layers = malloc(sizeof(VkLayerProperties) * layer_count);
  if (available_layers == NULL) {
    return false;
  }

  if (vkEnumerateInstanceLayerProperties(&layer_count, available_layers) != VK_SUCCESS) {
    free(available_layers);
    return false;
  }

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

static const char** get_required_extensions(uint32_t* count) {
  uint32_t glfw_ext_count = 0;
  const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
  if (glfw_ext_count == 0) {
    return NULL;
  }

  if (enable_validation_layers) {
    *count = glfw_ext_count + 1;
    const char** extensions = malloc(sizeof(const char*) * (*count));
    if (extensions == NULL) {
      return NULL;
    }

    memcpy(extensions, glfw_exts, sizeof(const char*) * glfw_ext_count);
    extensions[glfw_ext_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    return extensions;
  } else {
    *count = glfw_ext_count;
    const char** extensions = malloc(sizeof(const char*) * (*count));
    if (extensions == NULL) {
      return NULL;
    }

    memcpy(extensions, glfw_exts, sizeof(const char*) * glfw_ext_count);
    return extensions;
  }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                     VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                                                     void* p_user_data) {
  printf("validation layer: %s\n", p_callback_data->pMessage);
  return VK_FALSE;
}

static void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT* create_info) {
  create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info->pfnUserCallback = debug_callback;
  create_info->pNext = NULL;
}

static bool is_complete(QueueFamilyIndices* indices) {
  return indices->graphics_family_has_value && indices->present_family_has_value;
}

static QueueFamilyIndices find_queue_families(VkSurfaceKHR surface, VkPhysicalDevice device) {
  QueueFamilyIndices indices = {0};

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

  VkQueueFamilyProperties* queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
  if (queue_families == NULL) {
    return indices;
  }

  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

  for (uint32_t i = 0; i < queue_family_count; i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics_family = i;
      indices.graphics_family_has_value = true;
    }

    VkBool32 present_support = false;
    if (vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support) != VK_SUCCESS) {
      free(queue_families);
      return indices;
    }

    if (present_support) {
      indices.present_family = i;
      indices.present_family_has_value = true;
    }

    if (is_complete(&indices)) {
      break;
    }
  }

  free(queue_families);
  return indices;
}

static bool is_device_suitable(VkSurfaceKHR surface, VkPhysicalDevice device) {
  QueueFamilyIndices indices = find_queue_families(surface, device);
  return is_complete(&indices);
}

static void glfw_error_cb(int error_code, const char* description) {
  printf("[GLFW] %d: %s\n", error_code, description);
}

int main() {
  int exit_code = 0;

  bool glfw_initialized = false;
  GLFWwindow* window = NULL;

  VkInstance instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;

  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;

  VkQueue graphics_queue = VK_NULL_HANDLE;
  VkQueue present_queue = VK_NULL_HANDLE;

  VkPhysicalDevice* gpus = NULL;

  const char** exts = NULL;

  // setup glfw
  glfwSetErrorCallback(glfw_error_cb);

  if (glfwInit() == GLFW_FALSE) {
    FAIL_AND_CLEANUP("failed to initialize glfw");
  }

  glfw_initialized = true;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, NAME, NULL, NULL);
  if (window == NULL) {
    FAIL_AND_CLEANUP("failed to create window");
  }

  // setup vulkan...
  if (enable_validation_layers && !check_validation_layer_support()) {
    FAIL_AND_CLEANUP("validation layers requested, but not available");
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
  exts = get_required_extensions(&ext_count);
  if (exts == NULL) {
    FAIL_AND_CLEANUP("failed to get required extensions");
  }

  create_info.enabledExtensionCount = ext_count;
  create_info.ppEnabledExtensionNames = exts;

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
    FAIL_AND_CLEANUP("failed to create vulkan instance");
  }

  // setup debug messenger
  if (enable_validation_layers) {
    if (create_debug_utils_messenger_ext(instance, &create_info, NULL, &debug_messenger) != VK_SUCCESS) {
      FAIL_AND_CLEANUP("failed to set up debug messenger");
    }
  }

  // create surface
  if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
    FAIL_AND_CLEANUP("failed to create window surface");
  }

  // pick gpu
  uint32_t gpu_count = 0;
  if (vkEnumeratePhysicalDevices(instance, &gpu_count, NULL) != VK_SUCCESS) {
    FAIL_AND_CLEANUP("failed to enumerate physical devices");
  }

  if (gpu_count == 0) {
    FAIL_AND_CLEANUP("failed to find GPUs with vulkan support");
  }

  gpus = malloc(sizeof(VkPhysicalDevice) * gpu_count);
  if (gpus == NULL) {
    FAIL_AND_CLEANUP("failed to allocate memory for GPUs");
  }

  if (vkEnumeratePhysicalDevices(instance, &gpu_count, gpus) != VK_SUCCESS) {
    FAIL_AND_CLEANUP("failed to enumerate physical devices");
  }

  for (uint32_t i = 0; i < gpu_count; i++) {
    if (is_device_suitable(surface, gpus[i])) {
      physical_device = gpus[i];

      VkPhysicalDeviceProperties device_properties;
      vkGetPhysicalDeviceProperties(physical_device, &device_properties);
      printf("selected GPU: %s\n", device_properties.deviceName);

      break;
    }
  }

  if (physical_device == VK_NULL_HANDLE) {
    FAIL_AND_CLEANUP("failed to find a suitable GPU");
  }

  // create logical device
  QueueFamilyIndices indices = find_queue_families(surface, physical_device);

  uint32_t unique_queue_families[2];
  uint32_t unique_count = 0;

  if (indices.graphics_family_has_value) {
    unique_queue_families[unique_count++] = indices.graphics_family;
  }

  if (indices.present_family_has_value && indices.present_family != indices.graphics_family) {
    unique_queue_families[unique_count++] = indices.present_family;
  }

  float queue_priority = 1.0f;
  VkDeviceQueueCreateInfo queue_create_infos[2];

  for (uint32_t i = 0; i < unique_count; i++) {
    queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[i].pNext = NULL;
    queue_create_infos[i].flags = 0;
    queue_create_infos[i].queueFamilyIndex = unique_queue_families[i];
    queue_create_infos[i].queueCount = 1;
    queue_create_infos[i].pQueuePriorities = &queue_priority;
  }

  VkPhysicalDeviceFeatures device_features = {0};

  VkDeviceCreateInfo device_create_info = {0};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  device_create_info.queueCreateInfoCount = unique_count;
  device_create_info.pQueueCreateInfos = queue_create_infos;
  device_create_info.pEnabledFeatures = &device_features;

  device_create_info.enabledExtensionCount = 0;

  if (enable_validation_layers) {
    device_create_info.enabledLayerCount = sizeof(validation_layers) / sizeof(validation_layers[0]);
    device_create_info.ppEnabledLayerNames = validation_layers;
  } else {
    device_create_info.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physical_device, &device_create_info, NULL, &device) != VK_SUCCESS) {
    FAIL_AND_CLEANUP("failed to create logical device");
  }

  vkGetDeviceQueue(device, indices.graphics_family, 0, &graphics_queue);
  vkGetDeviceQueue(device, indices.present_family, 0, &present_queue);

  // finish window setup

#ifdef _WIN32
  HWND hwnd = glfwGetWin32Window(window);
  if (hwnd == NULL) {
    FAIL_AND_CLEANUP("failed to get window handle");
  }

  BOOL dark = TRUE;
  DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
#endif

  glfwShowWindow(window);

  // main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  // cleanup
cleanup:
  if (gpus != NULL) {
    free(gpus);
  }

  if (device != VK_NULL_HANDLE) {
    vkDestroyDevice(device, NULL);
  }

  if (exts != NULL) {
    free((void*)exts);
  }

  if (enable_validation_layers && debug_messenger != VK_NULL_HANDLE) {
    destroy_debug_utils_messenger_ext(instance, debug_messenger, NULL);
  }

  if (surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance, surface, NULL);
  }

  if (instance != VK_NULL_HANDLE) {
    vkDestroyInstance(instance, NULL);
  }

  if (window != NULL) {
    glfwDestroyWindow(window);
  }

  if (glfw_initialized) {
    glfwTerminate();
  }

  return exit_code;
}
