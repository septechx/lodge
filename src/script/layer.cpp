#include "layer.hpp"

ScriptLayer::ScriptLayer(Scene &scene) : m_scripts(scene) {}

void ScriptLayer::onAttach() { m_scripts.loadSceneScripts(); }

void ScriptLayer::onUpdate(float dt) { m_scripts.updateScripts(dt); }
