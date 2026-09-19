#pragma once

#include "src/core/layer.hpp"
#include "src/script/manager.hpp"

struct GLFWwindow;

class ScriptLayer final : public Layer {
public:
  ScriptLayer(Scene &scene, GLFWwindow *window = nullptr);
  ~ScriptLayer() = default;

  void onAttach() override;
  void onUpdate(float dt) override;
  bool onEvent(const Event &event) override;

private:
  ScriptManager m_scripts;
};
