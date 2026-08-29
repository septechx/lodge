#pragma once

#include <spdlog/spdlog.h>

#include <expected>
#include <filesystem>
#include <string>

#define LDG_ASSERT(cond)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      spdlog::error("{}:{}: assertion failed: {}", __FILE__, __LINE__, #cond); \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

enum class ReadFileError { Error };

std::expected<std::string, ReadFileError>
readFileToString(const std::filesystem::path &path);
