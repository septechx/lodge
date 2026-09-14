#include "layer.hpp"

ScriptLayer::ScriptLayer(Scene &scene) : m_scripts(scene) {}

void ScriptLayer::onAttach() { m_scripts.loadScript("test.lua"); }

void ScriptLayer::onUpdate(float dt) { m_scripts.updateScripts(dt); }
