#pragma once

#include "serialize.hpp"

namespace ser {

std::string toJson(const Serializable &value);
void fromJson(Deserialazable &out, const std::string &json);

}; // namespace ser
