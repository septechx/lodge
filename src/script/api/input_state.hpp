#pragma once

#include "src/core/event.hpp"

#include <optional>
#include <string_view>
#include <unordered_set>

class InputState {
public:
  void onEvent(const Event &event);
  void endFrame();

  bool isKeyDown(int key) const;
  bool isMouseDown(int button) const;

  double mouseX() const { return m_mouseX; }
  double mouseY() const { return m_mouseY; }
  double deltaX() const { return m_deltaX; }
  double deltaY() const { return m_deltaY; }
  double scrollX() const { return m_scrollX; }
  double scrollY() const { return m_scrollY; }
  bool hasMousePos() const { return m_hasMousePos; }

  static std::optional<int> keyFromString(std::string_view name);

  static std::optional<int> buttonFromString(std::string_view name);

private:
  std::unordered_set<int> m_keysDown;
  std::unordered_set<int> m_buttonsDown;
  double m_mouseX = 0.0;
  double m_mouseY = 0.0;
  double m_deltaX = 0.0;
  double m_deltaY = 0.0;
  double m_scrollX = 0.0;
  double m_scrollY = 0.0;
  bool m_hasMousePos = false;
};
