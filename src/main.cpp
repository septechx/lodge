#include "src/engine/engine.hpp"

#include <GLFW/glfw3.h>

#include <cstring>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    if (std::strncmp(argv[i], "--", 2) == 0) {
      args.emplace_back(argv[i] + 2);
    }
  }

  if (std::ranges::find(args, "debug") != args.end()) {
    spdlog::set_level(spdlog::level::trace);
  }

  Engine engine(std::move(args));
  engine.run();
}
