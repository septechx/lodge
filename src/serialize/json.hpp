#pragma once

#include "serialize.hpp"

#include <expected>

namespace ser {

enum class JsonError { Error };

std::string toJson(const Serializable &value);
std::string toJson(const Value &value);
std::expected<Value, JsonError> parseJson(const std::string &json);
std::expected<int, JsonError> fromJson(Deserialazable &out,
                                       const std::string &json);

}; // namespace ser
