#pragma once

#include "src/core/layer.hpp"
#include "src/script/manager.hpp"

class ScriptLayer final : public Layer {
public:
  ScriptLayer(Scene &scene);
  ~ScriptLayer() = default;

  void onAttach() override;
  void onUpdate(float dt) override;
  bool onEvent(const Event &event) override;

private:
  ScriptManager m_scripts;
};
