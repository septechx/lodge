#include "input_state.hpp"

#include <GLFW/glfw3.h>

#include <cctype>
#include <string>

namespace {

std::string lowerOf(std::string_view name) {
  std::string out;
  out.reserve(name.size());
  for (char c : name)
    out.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  return out;
}

} // namespace

void InputState::onEvent(const Event &event) {
  if (const auto *press = std::get_if<events::KeyPressed>(&event)) {
    m_keysDown.insert(press->key);
  } else if (const auto *release = std::get_if<events::KeyReleased>(&event)) {
    m_keysDown.erase(release->key);
  } else if (const auto *press =
                 std::get_if<events::MouseButtonPressed>(&event)) {
    m_buttonsDown.insert(press->button);
  } else if (const auto *release =
                 std::get_if<events::MouseButtonReleased>(&event)) {
    m_buttonsDown.erase(release->button);
  } else if (const auto *moved = std::get_if<events::MouseMoved>(&event)) {
    if (m_hasMousePos) {
      m_deltaX += moved->x - m_mouseX;
      m_deltaY += moved->y - m_mouseY;
    }
    m_mouseX = moved->x;
    m_mouseY = moved->y;
    m_hasMousePos = true;
  } else if (const auto *scrolled =
                 std::get_if<events::MouseScrolled>(&event)) {
    m_scrollX += scrolled->xoffset;
    m_scrollY += scrolled->yoffset;
  }
}

void InputState::endFrame() {
  m_deltaX = 0.0;
  m_deltaY = 0.0;
  m_scrollX = 0.0;
  m_scrollY = 0.0;
}

bool InputState::isKeyDown(int key) const { return m_keysDown.count(key) != 0; }

bool InputState::isMouseDown(int button) const {
  return m_buttonsDown.count(button) != 0;
}

std::optional<int> InputState::keyFromString(std::string_view name) {
  if (name.size() == 1) {
    char c =
        static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    if (c >= 'A' && c <= 'Z')
      return static_cast<int>(c);
    if (c >= '0' && c <= '9')
      return static_cast<int>(c);
    switch (c) {
    case ' ':
      return GLFW_KEY_SPACE;
    case '\'':
      return GLFW_KEY_APOSTROPHE;
    case ',':
      return GLFW_KEY_COMMA;
    case '-':
      return GLFW_KEY_MINUS;
    case '.':
      return GLFW_KEY_PERIOD;
    case '/':
      return GLFW_KEY_SLASH;
    case ';':
      return GLFW_KEY_SEMICOLON;
    case '=':
      return GLFW_KEY_EQUAL;
    case '[':
      return GLFW_KEY_LEFT_BRACKET;
    case '\\':
      return GLFW_KEY_BACKSLASH;
    case ']':
      return GLFW_KEY_RIGHT_BRACKET;
    case '`':
      return GLFW_KEY_GRAVE_ACCENT;
    default:
      return std::nullopt;
    }
  }

  std::string lower = lowerOf(name);
  if (lower == "space")
    return GLFW_KEY_SPACE;
  if (lower == "escape" || lower == "esc")
    return GLFW_KEY_ESCAPE;
  if (lower == "enter" || lower == "return")
    return GLFW_KEY_ENTER;
  if (lower == "tab")
    return GLFW_KEY_TAB;
  if (lower == "backspace")
    return GLFW_KEY_BACKSPACE;
  if (lower == "insert")
    return GLFW_KEY_INSERT;
  if (lower == "delete")
    return GLFW_KEY_DELETE;
  if (lower == "right")
    return GLFW_KEY_RIGHT;
  if (lower == "left")
    return GLFW_KEY_LEFT;
  if (lower == "down")
    return GLFW_KEY_DOWN;
  if (lower == "up")
    return GLFW_KEY_UP;
  if (lower == "pageup" || lower == "page_up")
    return GLFW_KEY_PAGE_UP;
  if (lower == "pagedown" || lower == "page_down")
    return GLFW_KEY_PAGE_DOWN;
  if (lower == "home")
    return GLFW_KEY_HOME;
  if (lower == "end")
    return GLFW_KEY_END;
  if (lower == "capslock" || lower == "caps_lock")
    return GLFW_KEY_CAPS_LOCK;
  if (lower == "scrolllock" || lower == "scroll_lock")
    return GLFW_KEY_SCROLL_LOCK;
  if (lower == "numlock" || lower == "num_lock")
    return GLFW_KEY_NUM_LOCK;
  if (lower == "printscreen" || lower == "print_screen")
    return GLFW_KEY_PRINT_SCREEN;
  if (lower == "pause")
    return GLFW_KEY_PAUSE;
  if (lower == "shift")
    return GLFW_KEY_LEFT_SHIFT;
  if (lower == "leftshift" || lower == "left_shift")
    return GLFW_KEY_LEFT_SHIFT;
  if (lower == "rightshift" || lower == "right_shift")
    return GLFW_KEY_RIGHT_SHIFT;
  if (lower == "control" || lower == "ctrl")
    return GLFW_KEY_LEFT_CONTROL;
  if (lower == "leftcontrol" || lower == "left_control" ||
      lower == "leftctrl" || lower == "left_ctrl")
    return GLFW_KEY_LEFT_CONTROL;
  if (lower == "rightcontrol" || lower == "right_control" ||
      lower == "rightctrl" || lower == "right_ctrl")
    return GLFW_KEY_RIGHT_CONTROL;
  if (lower == "alt")
    return GLFW_KEY_LEFT_ALT;
  if (lower == "leftalt" || lower == "left_alt")
    return GLFW_KEY_LEFT_ALT;
  if (lower == "rightalt" || lower == "right_alt")
    return GLFW_KEY_RIGHT_ALT;
  if (lower == "super" || lower == "gui" || lower == "meta" ||
      lower == "windows" || lower == "command")
    return GLFW_KEY_LEFT_SUPER;
  if (lower == "leftsuper" || lower == "left_super")
    return GLFW_KEY_LEFT_SUPER;
  if (lower == "rightsuper" || lower == "right_super")
    return GLFW_KEY_RIGHT_SUPER;
  if (lower == "menu")
    return GLFW_KEY_MENU;
  if (lower == "minus")
    return GLFW_KEY_MINUS;
  if (lower == "equal")
    return GLFW_KEY_EQUAL;
  if (lower == "leftbracket" || lower == "left_bracket")
    return GLFW_KEY_LEFT_BRACKET;
  if (lower == "rightbracket" || lower == "right_bracket")
    return GLFW_KEY_RIGHT_BRACKET;
  if (lower == "backslash")
    return GLFW_KEY_BACKSLASH;
  if (lower == "semicolon")
    return GLFW_KEY_SEMICOLON;
  if (lower == "apostrophe" || lower == "quote")
    return GLFW_KEY_APOSTROPHE;
  if (lower == "grave" || lower == "backquote" || lower == "grave_accent")
    return GLFW_KEY_GRAVE_ACCENT;
  if (lower == "comma")
    return GLFW_KEY_COMMA;
  if (lower == "period" || lower == "dot")
    return GLFW_KEY_PERIOD;
  if (lower == "slash")
    return GLFW_KEY_SLASH;
  if (lower == "world1" || lower == "world_1")
    return GLFW_KEY_WORLD_1;
  if (lower == "world2" || lower == "world_2")
    return GLFW_KEY_WORLD_2;

  // F1..F25
  if (lower.size() >= 2 && lower[0] == 'f') {
    int n = 0;
    for (size_t i = 1; i < lower.size(); ++i) {
      if (!std::isdigit(static_cast<unsigned char>(lower[i])))
        return std::nullopt;
      n = n * 10 + (lower[i] - '0');
    }
    if (n >= 1 && n <= 25)
      return GLFW_KEY_F1 + (n - 1);
    return std::nullopt;
  }

  // kp0..kp9, kpdecimal, kpdivide, kpmultiply, kpsubtract, kpadd, kpenter,
  // kpequal
  if (lower.size() >= 3 && lower[0] == 'k' && lower[1] == 'p') {
    std::string_view rest(lower.c_str() + 2, lower.size() - 2);
    if (rest.size() == 1 && rest[0] >= '0' && rest[0] <= '9')
      return GLFW_KEY_KP_0 + (rest[0] - '0');
    if (rest == "decimal")
      return GLFW_KEY_KP_DECIMAL;
    if (rest == "divide")
      return GLFW_KEY_KP_DIVIDE;
    if (rest == "multiply")
      return GLFW_KEY_KP_MULTIPLY;
    if (rest == "subtract")
      return GLFW_KEY_KP_SUBTRACT;
    if (rest == "add")
      return GLFW_KEY_KP_ADD;
    if (rest == "enter")
      return GLFW_KEY_KP_ENTER;
    if (rest == "equal")
      return GLFW_KEY_KP_EQUAL;
    return std::nullopt;
  }

  return std::nullopt;
}

std::optional<int> InputState::buttonFromString(std::string_view name) {
  std::string lower = lowerOf(name);
  if (lower == "left" || lower == "button1" || lower == "1")
    return GLFW_MOUSE_BUTTON_LEFT;
  if (lower == "right" || lower == "button2" || lower == "2")
    return GLFW_MOUSE_BUTTON_RIGHT;
  if (lower == "middle" || lower == "button3" || lower == "3")
    return GLFW_MOUSE_BUTTON_MIDDLE;
  if (lower == "button4" || lower == "4")
    return GLFW_MOUSE_BUTTON_4;
  if (lower == "button5" || lower == "5")
    return GLFW_MOUSE_BUTTON_5;
  if (lower == "button6" || lower == "6")
    return GLFW_MOUSE_BUTTON_6;
  if (lower == "button7" || lower == "7")
    return GLFW_MOUSE_BUTTON_7;
  if (lower == "button8" || lower == "8")
    return GLFW_MOUSE_BUTTON_8;
  return std::nullopt;
}
