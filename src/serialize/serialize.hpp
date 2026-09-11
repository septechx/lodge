#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ser {

class Value {
public:
  using Null = std::monostate;
  using Bool = bool;
  using Int = int32_t;
  using Real = float;
  using String = std::string;
  using Array = std::vector<Value>;
  using Map = std::unordered_map<std::string, Value>;

  Value() : m_value(Null{}) {};
  Value(Null value) : m_value(value) {}
  Value(Bool value) : m_value(value) {}
  Value(Int value) : m_value(value) {}
  Value(Real value) : m_value(value) {}
  Value(String value) : m_value(std::move(value)) {}
  Value(const char *value) : m_value(String(value)) {}
  Value(Array value) : m_value(std::move(value)) {}
  Value(Map value) : m_value(std::move(value)) {}

  template <typename Visitor> decltype(auto) visit(Visitor &&visitor) const {
    return std::visit(std::forward<Visitor>(visitor), m_value);
  }

private:
  std::variant<Null, Bool, Int, Real, String, Array, Map> m_value;
};

class Serializable {
public:
  virtual ~Serializable() = default;

  virtual Value serializeInfo() const = 0;
};

}; // namespace ser
