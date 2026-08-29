#pragma once

#include "src/core/layer.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class LayerStack {
public:
  void pushLayer(std::string id, std::unique_ptr<Layer> layer);
  void pushOverlay(std::string id, std::unique_ptr<Layer> layer);

  void popLayer(const std::string &id);

  Layer *getLayer(const std::string &id);
  const Layer *getLayer(const std::string &id) const;

  void onUpdate(float dt);
  void onEvent(const Event &event);

private:
  struct Entry {
    std::string id;
    std::unique_ptr<Layer> layer;
  };

  std::vector<Entry> m_layers;
  std::size_t m_overlayStart = 0;
};
