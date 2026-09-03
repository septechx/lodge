#pragma once

#include "src/asset/store.hpp"
#include "src/core/event.hpp"
#include "src/core/layer_stack.hpp"
#include "src/render/render_object.hpp"
#include "src/render/renderer.hpp"
#include "src/scene/extract.hpp"
#include "src/scene/scene.hpp"

#include <GLFW/glfw3.h>

#include <memory>
#include <vector>

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
  Engine(std::vector<std::string> args);
  ~Engine() = default;

  void run();

private:
  void buildScene();

  std::unique_ptr<GLFWwindow, GLFWWindowDeleter> m_window;
  std::unique_ptr<Renderer> m_renderer;
  std::unique_ptr<AssetStore> m_assets;
  std::unique_ptr<Scene> m_scene;
  std::vector<RenderObject> m_frameObjects;
  std::vector<FrameLight> m_frameLights;
  std::unique_ptr<EventQueue> m_events;
  std::unique_ptr<LayerStack> m_layers;
};
