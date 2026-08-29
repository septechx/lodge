#pragma once

#include "src/core/event.hpp"

class Layer {
public:
  virtual ~Layer() = default;

  Layer(const Layer &) = delete;
  Layer &operator=(const Layer &) = delete;

  virtual void onAttach() {}
  virtual void onDetach() {}
  virtual void onUpdate(float dt) { (void)dt; }

  virtual bool onEvent(const Event &event) {
    (void)event;
    return false;
  }

protected:
  Layer() = default;
};
