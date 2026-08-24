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

class QuitLayer final : public Layer {
public:
  bool onEvent(const Event &event) override {
    const auto *key = std::get_if<events::KeyPressed>(&event);
    if (key != nullptr && key->key == GLFW_KEY_ESCAPE) {
      m_shouldQuit = true;
      return true;
    }
    return false;
  }

  bool shouldQuit() const { return m_shouldQuit; }

private:
  bool m_shouldQuit = false;
};

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

  EventQueue events;
  events.attach(*window.get());

  LayerStack layers;
  QuitLayer quitLayer;
  layers.pushOverlay(quitLayer);

  auto last = std::chrono::steady_clock::now();
  while (!glfwWindowShouldClose(window.get()) && !quitLayer.shouldQuit()) {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;

    glfwPollEvents();

    Event event;
    while (events.poll(event))
      layers.onEvent(event);

    layers.onUpdate(dt);
    renderer.drawFrame();
  }
}
