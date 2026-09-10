#pragma once

#include "src/core/layer.hpp"
#include "src/script/manager.hpp"

class ScriptLayer final : public Layer {
public:
  ScriptLayer() = default;
  ~ScriptLayer() = default;

  void onAttach() override;
  void onUpdate(float dt) override;

private:
  ScriptManager m_scripts;
};
