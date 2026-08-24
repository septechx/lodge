#pragma once

#include <expected>
#include <filesystem>
#include <string>

enum class ReadFileError { Error };

std::expected<std::string, ReadFileError>
readFileToString(const std::filesystem::path &path);

template <typename T, typename Deleter> class Scoped {
  T value;
  Deleter deleter;

public:
  constexpr Scoped(T value, Deleter deleter = {})
      : value(value), deleter(deleter) {}

  Scoped(const Scoped &) = delete;
  Scoped &operator=(const Scoped &) = delete;

  ~Scoped() { deleter(value); }

  constexpr T get() const { return value; }
};
