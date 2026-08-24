#pragma once

#include <GLFW/glfw3.h>

#include <cstdint>
#include <deque>
#include <variant>

namespace events {
struct WindowResized {
  std::uint32_t width;
  std::uint32_t height;
};

struct KeyPressed {
  int key;
  int mods;
};

struct KeyReleased {
  int key;
  int mods;
};

struct MouseButtonPressed {
  int button;
  int mods;
};

struct MouseButtonReleased {
  int button;
  int mods;
};

struct MouseMoved {
  double x;
  double y;
};

struct MouseScrolled {
  double xoffset;
  double yoffset;
};
}; // namespace events

using Event =
    std::variant<events::WindowResized, events::KeyPressed, events::KeyReleased,
                 events::MouseButtonPressed, events::MouseButtonReleased,
                 events::MouseMoved, events::MouseScrolled>;

class EventQueue {
public:
  void push(Event event);
  bool poll(Event &out);

  void attach(GLFWwindow &window);

private:
  std::deque<Event> m_events;
};
