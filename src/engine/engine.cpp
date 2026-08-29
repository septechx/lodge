#include "engine.hpp"

#include "src/consts.hpp"
#include "src/core/control_layer.hpp"
#include "src/core/debug_layer.hpp"

#include <spdlog/spdlog.h>

Engine::Engine(bool enableDebug) {
  if (!glfwInit()) {
    spdlog::error("glfw init failed");
    std::exit(1);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_window.reset(glfwCreateWindow(WIDTH, HEIGHT, "Lodge", nullptr, nullptr));

  if (!m_window) {
    spdlog::error("glfw window failed");
    std::exit(1);
  }

  m_renderer = std::make_unique<Renderer>(*m_window);

  m_events = std::make_unique<EventQueue>();
  m_events->attach(*m_window);

  m_layers = std::make_unique<LayerStack>();

  m_layers->pushLayer("control", std::make_unique<ControlLayer>(*m_window));

  if (enableDebug) {
    m_layers->pushOverlay("debug",
                          std::make_unique<DebugLayer>(*m_window, *m_renderer));
  }
}

void Engine::run() {
  auto last = std::chrono::steady_clock::now();
  while (!glfwWindowShouldClose(m_window.get()) &&
         !static_cast<ControlLayer *>(m_layers->getLayer("control"))
              ->shouldQuit()) {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;

    glfwPollEvents();

    Event event;
    while (m_events->poll(event)) {
      const auto *resize = std::get_if<events::WindowResized>(&event);
      if (resize != nullptr) {
        m_renderer->onResize(resize->width, resize->height);
      }
      m_layers->onEvent(event);
    }

    m_layers->onUpdate(dt);
    m_renderer->drawFrame();
  }
}
