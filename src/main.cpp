#include "src/consts.hpp"
#include "src/core/debug_layer.hpp"
#include "src/core/event.hpp"
#include "src/core/layer.hpp"
#include "src/core/layer_stack.hpp"
#include "src/utils.hpp"

#include <GLFW/glfw3.h>

#include <chrono>
#include <cstring>
#include <spdlog/spdlog.h>
#include <variant>

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

int main(int argc, char **argv) {
  bool enableDebug = argc >= 2 && strcmp(argv[1], "--debug") == 0;

  if (enableDebug) {
    spdlog::set_level(spdlog::level::trace);
  }

  int init_result = glfwInit();
  if (init_result == 0) {
    spdlog::error("glfw init failed");
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
    spdlog::error("glfw window failed");
    return 1;
  }

  Renderer renderer(*window.get());

  EventQueue events;
  events.attach(*window.get());

  LayerStack layers;
  ControlLayer controlLayer(*window.get());
  layers.pushLayer(controlLayer);

  DebugLayer debugLayer(*window.get(), renderer);
  if (enableDebug) {
    layers.pushOverlay(debugLayer);
  }

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
