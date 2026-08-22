#pragma once

#include <expected>
#include <filesystem>
#include <string>

enum class ReadFileError { Error };

std::expected<std::string, ReadFileError>
readFileToString(const std::filesystem::path &path);
