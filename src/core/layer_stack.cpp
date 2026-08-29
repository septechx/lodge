#include "layer_stack.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

void LayerStack::pushLayer(std::string id, std::unique_ptr<Layer> layer) {
  layer->onAttach();

  m_layers.insert(m_layers.begin() + m_overlayStart,
                  Entry{std::move(id), std::move(layer)});

  ++m_overlayStart;
}

void LayerStack::pushOverlay(std::string id, std::unique_ptr<Layer> layer) {
  layer->onAttach();

  m_layers.push_back(Entry{std::move(id), std::move(layer)});
}

void LayerStack::popLayer(const std::string &id) {
  auto it = std::ranges::find(m_layers, id, &Entry::id);

  if (it == m_layers.end())
    return;

  it->layer->onDetach();

  if (static_cast<std::size_t>(it - m_layers.begin()) < m_overlayStart)
    --m_overlayStart;

  m_layers.erase(it);
}

Layer *LayerStack::getLayer(const std::string &id) {
  auto it = std::ranges::find(m_layers, id, &Entry::id);

  if (it == m_layers.end())
    return nullptr;

  return it->layer.get();
}

const Layer *LayerStack::getLayer(const std::string &id) const {
  auto it = std::ranges::find(m_layers, id, &Entry::id);

  if (it == m_layers.end())
    return nullptr;

  return it->layer.get();
}

void LayerStack::onUpdate(float dt) {
  std::ranges::for_each(m_layers,
                        [dt](Entry &entry) { entry.layer->onUpdate(dt); });
}

void LayerStack::onEvent(const Event &event) {
  for (Entry &entry : m_layers | std::views::reverse) {
    if (entry.layer->onEvent(event))
      break;
  }
}
