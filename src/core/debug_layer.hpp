#pragma once

#include "src/core/layer.hpp"
#include "src/render/renderer.hpp"

#include <GLFW/glfw3.h>

class DebugLayer final : public Layer {
public:
  DebugLayer(GLFWwindow &window, Renderer &renderer);
  ~DebugLayer() override;

  void onAttach() override;
  void onDetach() override;
  void onUpdate(float dt) override;
  bool onEvent(const Event &event) override;

  bool isVisible() const { return m_visible; }
  void setVisible(bool v) { m_visible = v; }

private:
  void updateFreeFly(float dt);
  void syncYawPitchFromCamera();
  void buildUI(float dt);

  GLFWwindow &m_window;
  Renderer &m_renderer;

  bool m_visible = true;
  bool m_initialized = false;

  float m_yaw = -135.0f;
  float m_pitch = -20.0f;
  float m_moveSpeed = 2.5f;
  float m_lookSpeed = 0.15f;
  double m_lastX = 0.0;
  double m_lastY = 0.0;
  bool m_firstMouse = true;
  bool m_freeFlyEnabled = true;
};
