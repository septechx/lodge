#include "utils.hpp"

#include <fstream>
#include <vector>

#if defined(__linux__)
#include <unistd.h>
#endif

std::expected<std::string, ReadFileError>
readFileToString(const std::filesystem::path &path) {
  std::ifstream file(path);

  if (!file) {
    return std::unexpected(ReadFileError::Error);
  }

  return std::string(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>());
}

std::filesystem::path executableDir() {
#if defined(__linux__)
  char buf[4096];
  ssize_t len = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (len > 0) {
    buf[len] = '\0';
    std::error_code ec;
    auto dir = std::filesystem::canonical(std::filesystem::path(buf), ec)
                   .parent_path();
    if (!ec)
      return dir;
    return std::filesystem::path(buf).parent_path();
  }
#endif
  return {};
}

std::filesystem::path resolveScenePath(const std::filesystem::path &override) {
  std::error_code ec;
  if (!override.empty()) {
    if (std::filesystem::exists(override, ec))
      return override;
    return {};
  }

  std::vector<std::filesystem::path> candidates = {
      "scene.json",
  };

  const auto exeDir = executableDir();
  if (!exeDir.empty()) {
    candidates.push_back(exeDir / "scene.json");
    candidates.push_back(
        (exeDir / "../share/lodge/scene.json").lexically_normal());
  }

#ifdef LODGE_INSTALL_DATADIR
  candidates.emplace_back(std::filesystem::path(LODGE_INSTALL_DATADIR) /
                          "scene.json");
#endif

  for (const auto &candidate : candidates) {
    if (std::filesystem::exists(candidate, ec))
      return candidate;
  }
  return {};
}
