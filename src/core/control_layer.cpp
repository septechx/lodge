#include "control_layer.hpp"

#include "src/consts.hpp"

bool ControlLayer::onEvent(const Event &event) {
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

bool ControlLayer::shouldQuit() const { return m_shouldQuit; }
