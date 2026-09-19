#define TINYGLTF_JSON_C_IMPLEMENTATION

#include "src/engine/engine.hpp"

#include <GLFW/glfw3.h>

#include <cstring>
#include <filesystem>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
  std::vector<std::string> args;
  std::filesystem::path sceneOverride;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg(argv[i]);
    if (arg == "--scene") {
      if (i + 1 < argc) {
        sceneOverride = argv[++i];
      } else {
        spdlog::error("--scene requires a path");
        return 1;
      }
    } else if (arg.starts_with("--scene=")) {
      sceneOverride = std::string(arg.substr(strlen("--scene=")));
    } else if (arg.starts_with("--")) {
      args.emplace_back(arg.substr(2));
    } else if (sceneOverride.empty()) {
      sceneOverride = argv[i];
    } else {
      spdlog::warn("ignoring extra positional argument {}", argv[i]);
    }
  }

  if (std::ranges::find(args, "debug") != args.end()) {
    spdlog::set_level(spdlog::level::trace);
  }

  Engine engine(std::move(args), std::move(sceneOverride));
  engine.run();
}
