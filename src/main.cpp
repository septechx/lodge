#include "src/engine/engine.hpp"

#include <GLFW/glfw3.h>

#include <cstring>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
  bool enableDebug = argc >= 2 && strcmp(argv[1], "--debug") == 0;

  if (enableDebug) {
    spdlog::set_level(spdlog::level::trace);
  }

  Engine engine(enableDebug);
  engine.run();
}
