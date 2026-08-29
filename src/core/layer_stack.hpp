#pragma once

#include "src/core/layer.hpp"

#include <vector>

class LayerStack {
public:
  void pushLayer(Layer &layer);
  void pushOverlay(Layer &overlay);
  void popLayer(Layer &layer);

  void onUpdate(float dt);
  void onEvent(const Event &event);

private:
  std::vector<Layer *> m_layers;
  std::size_t m_overlayStart = 0;
};
