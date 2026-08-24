#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <print>

#include "consts.hpp"
#include "render/renderer.hpp"
#include "utils.hpp"

int main() {
  int init_result = glfwInit();
  if (init_result == 0) {
    std::println(stderr, "glfw init failed");
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  auto window =
      Scoped(glfwCreateWindow(WIDTH, HEIGHT, "Hello world", nullptr, nullptr),
             [](GLFWwindow *window) {
               glfwDestroyWindow(window);
               glfwTerminate();
             });
  if (!window.get()) {
    std::println(stderr, "glfw window failed");
    return 1;
  }

  Renderer renderer(*window.get());

  while (!glfwWindowShouldClose(window.get())) {
    glfwPollEvents();

    if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window.get(), GLFW_TRUE);

    renderer.drawFrame();
  }
}
