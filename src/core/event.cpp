#include "event.hpp"

void EventQueue::push(Event event) { m_events.push_back(std::move(event)); }

bool EventQueue::poll(Event &out) {
  if (m_events.empty())
    return false;
  out = std::move(m_events.front());
  m_events.pop_front();
  return true;
}

EventQueue &queueOf(GLFWwindow *window) {
  return *static_cast<EventQueue *>(glfwGetWindowUserPointer(window));
}

void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
  queueOf(window).push(events::WindowResized{
      static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)});
}

void keyCallback(GLFWwindow *window, int key, int /*scancode*/, int action,
                 int mods) {
  switch (action) {
  case GLFW_PRESS:
    queueOf(window).push(events::KeyPressed{key, mods});
    break;
  case GLFW_RELEASE:
    queueOf(window).push(events::KeyReleased{key, mods});
    break;
  default:
    break;
  }
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
  switch (action) {
  case GLFW_PRESS:
    queueOf(window).push(events::MouseButtonPressed{button, mods});
    break;
  case GLFW_RELEASE:
    queueOf(window).push(events::MouseButtonReleased{button, mods});
    break;
  default:
    break;
  }
}

void cursorPosCallback(GLFWwindow *window, double x, double y) {
  queueOf(window).push(events::MouseMoved{x, y});
}

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
  queueOf(window).push(events::MouseScrolled{xoffset, yoffset});
}

void EventQueue::attach(GLFWwindow &window) {
  glfwSetWindowUserPointer(&window, this);
  glfwSetFramebufferSizeCallback(&window, framebufferResizeCallback);
  glfwSetKeyCallback(&window, keyCallback);
  glfwSetMouseButtonCallback(&window, mouseButtonCallback);
  glfwSetCursorPosCallback(&window, cursorPosCallback);
  glfwSetScrollCallback(&window, scrollCallback);
}
