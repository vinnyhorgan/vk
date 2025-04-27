#include <stdio.h>

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

static void glfw_error_cb(int error_code, const char* description) {
  printf("[GLFW] %d: %s\n", error_code, description);
}

int main() {
  int exit_code = 0;
  GLFWwindow* window = NULL;
  VkInstance instance = VK_NULL_HANDLE;

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
  VkApplicationInfo appInfo = {0};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = NAME;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = NAME;
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  uint32_t glfw_ext_count = 0;
  const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
  if (glfw_ext_count == 0) {
    printf("failed to get required instance extensions\n");
    exit_code = 1;
    goto cleanup;
  }

  createInfo.enabledExtensionCount = glfw_ext_count;
  createInfo.ppEnabledExtensionNames = glfw_exts;
  createInfo.enabledLayerCount = 0;

  if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
    printf("failed to create vulkan instance\n");
    exit_code = 1;
    goto cleanup;
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
