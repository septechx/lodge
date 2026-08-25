#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <chrono>
#include <print>
#include <variant>

#include "consts.hpp"
#include "core/event.hpp"
#include "core/layer.hpp"
#include "core/layer_stack.hpp"
#include "render/renderer.hpp"
#include "utils.hpp"

class ControlLayer final : public Layer {
public:
  constexpr ControlLayer(GLFWwindow &window) : m_window(window) {}

  bool onEvent(const Event &event) override {
    const auto *key = std::get_if<events::KeyPressed>(&event);
    if (key != nullptr) {
      if (key->key == GLFW_KEY_ESCAPE) {
        m_shouldQuit = true;
        return true;
      } else if (key->key == GLFW_KEY_F11) {
        if (m_isFullscreen) {
          glfwSetWindowMonitor(&m_window, nullptr, 0, 0, WIDTH, HEIGHT, 0);
          m_isFullscreen = false;
        } else {
          GLFWmonitor *monitor = glfwGetPrimaryMonitor();
          const GLFWvidmode *videoMode = glfwGetVideoMode(monitor);
          glfwSetWindowMonitor(&m_window, monitor, 0, 0, videoMode->width,
                               videoMode->height, videoMode->refreshRate);
          m_isFullscreen = true;
        }
        return true;
      }
    }
    return false;
  }

  bool shouldQuit() const { return m_shouldQuit; }

private:
  bool m_shouldQuit = false;
  bool m_isFullscreen = false;
  GLFWwindow &m_window;
};

int main() {
  int init_result = glfwInit();
  if (init_result == 0) {
    std::println(stderr, "glfw init failed");
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  auto window =
      Scoped(glfwCreateWindow(WIDTH, HEIGHT, "Lodge", nullptr, nullptr),
             [](GLFWwindow *window) {
               glfwDestroyWindow(window);
               glfwTerminate();
             });
  if (!window.get()) {
    std::println(stderr, "glfw window failed");
    return 1;
  }

  Renderer renderer(*window.get());

  EventQueue events;
  events.attach(*window.get());

  LayerStack layers;
  ControlLayer controlLayer(*window.get());
  layers.pushOverlay(controlLayer);

  auto last = std::chrono::steady_clock::now();
  while (!glfwWindowShouldClose(window.get()) && !controlLayer.shouldQuit()) {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;

    glfwPollEvents();

    Event event;
    while (events.poll(event)) {
      const auto *resize = std::get_if<events::WindowResized>(&event);
      if (resize != nullptr) {
        renderer.onResize(resize->width, resize->height);
      }
      layers.onEvent(event);
    }

    layers.onUpdate(dt);
    renderer.drawFrame();
  }
}
