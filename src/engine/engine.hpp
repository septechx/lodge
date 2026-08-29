#pragma once

#include "src/core/event.hpp"
#include "src/core/layer_stack.hpp"
#include "src/render/renderer.hpp"

#include <GLFW/glfw3.h>

#include <memory>

struct GLFWWindowDeleter {
  void operator()(GLFWwindow *window) const {
    if (window) {
      glfwDestroyWindow(window);
      glfwTerminate();
    }
  }
};

class Engine {
public:
  Engine(bool enableDebug);

  void run();

private:
  std::unique_ptr<GLFWwindow, GLFWWindowDeleter> m_window;
  std::unique_ptr<Renderer> m_renderer;
  std::unique_ptr<EventQueue> m_events;
  std::unique_ptr<LayerStack> m_layers;
};
