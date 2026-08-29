#pragma once

#include "src/core/layer.hpp"

class ControlLayer final : public Layer {
public:
  constexpr ControlLayer(GLFWwindow &window) : m_window(window) {}

  bool onEvent(const Event &event) override;
  bool shouldQuit() const;

private:
  bool m_shouldQuit = false;
  bool m_isFullscreen = false;
  GLFWwindow &m_window;
};
