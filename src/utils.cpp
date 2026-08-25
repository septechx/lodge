#include "utils.hpp"

#include <fstream>

std::expected<std::string, ReadFileError>
readFileToString(const std::filesystem::path &path) {
  std::ifstream file(path);

  if (!file) {
    return std::unexpected(ReadFileError::Error);
  }

  return std::string(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>());
}
