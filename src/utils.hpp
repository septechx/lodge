#pragma once

#include <expected>
#include <filesystem>
#include <string>

#define LDG_ASSERT(cond)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::println(stderr, "{}:{}: assertion failed: {}", __FILE__, __LINE__,  \
                   #cond);                                                     \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

enum class ReadFileError { Error };

std::expected<std::string, ReadFileError>
readFileToString(const std::filesystem::path &path);

template <typename T, typename Deleter> class Scoped {
public:
  constexpr Scoped(T value, Deleter deleter = {})
      : m_value(value), m_deleter(deleter) {}

  Scoped(const Scoped &) = delete;
  Scoped &operator=(const Scoped &) = delete;

  ~Scoped() { m_deleter(m_value); }

  constexpr T get() const { return m_value; }

private:
  T m_value;
  Deleter m_deleter;
};
