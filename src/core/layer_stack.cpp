#include "layer_stack.hpp"

#include <algorithm>
#include <ranges>

void LayerStack::pushLayer(Layer &layer) {
  m_layers.insert(m_layers.begin() + m_overlayStart, &layer);
  ++m_overlayStart;
  layer.onAttach();
}

void LayerStack::pushOverlay(Layer &overlay) {
  m_layers.push_back(&overlay);
  overlay.onAttach();
}

void LayerStack::popLayer(Layer &layer) {
  auto it = std::find(m_layers.begin(), m_layers.end(), &layer);
  if (it == m_layers.end())
    return;
  layer.onDetach();
  if (static_cast<std::size_t>(it - m_layers.begin()) < m_overlayStart)
    --m_overlayStart;
  m_layers.erase(it);
}

void LayerStack::onUpdate(float dt) {
  std::ranges::for_each(m_layers, [dt](Layer *layer) { layer->onUpdate(dt); });
}

void LayerStack::onEvent(const Event &event) {
  for (Layer *layer : m_layers | std::views::reverse) {
    if (layer->onEvent(event))
      break;
  }
}
