#include <GLFW/glfw3.h>

int main() {
  glfwInit();
  GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
