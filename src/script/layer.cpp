#include "layer.hpp"

ScriptLayer::ScriptLayer(Scene &scene, GLFWwindow *window)
    : m_scripts(scene, window) {}

void ScriptLayer::onAttach() { m_scripts.loadSceneScripts(); }

void ScriptLayer::onUpdate(float dt) { m_scripts.onUpdate(dt); }

bool ScriptLayer::onEvent(const Event &event) {
  m_scripts.onEvent(event);
  return false;
}
