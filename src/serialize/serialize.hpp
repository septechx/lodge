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

  bool isNull() const { return std::get_if<Null>(&m_value) != nullptr; }
  bool isBool() const { return std::get_if<Bool>(&m_value) != nullptr; }
  bool isInt() const { return std::get_if<Int>(&m_value) != nullptr; }
  bool isReal() const { return std::get_if<Real>(&m_value) != nullptr; }
  bool isString() const { return std::get_if<String>(&m_value) != nullptr; }
  bool isArray() const { return std::get_if<Array>(&m_value) != nullptr; }
  bool isMap() const { return std::get_if<Map>(&m_value) != nullptr; }

  const Bool &asBool() const { return std::get<Bool>(m_value); }
  Int asInt() const {
    if (const auto *v = std::get_if<Int>(&m_value))
      return *v;
    if (const auto *v = std::get_if<Real>(&m_value))
      return static_cast<Int>(*v);
    return std::get<Int>(m_value);
  }
  Real asReal() const {
    if (const auto *v = std::get_if<Real>(&m_value))
      return *v;
    if (const auto *v = std::get_if<Int>(&m_value))
      return static_cast<Real>(*v);
    return std::get<Real>(m_value);
  }
  const String &asString() const { return std::get<String>(m_value); }
  const Array &asArray() const { return std::get<Array>(m_value); }
  const Map &asMap() const { return std::get<Map>(m_value); }

  template <typename Visitor> decltype(auto) visit(Visitor &&visitor) const {
    return std::visit(std::forward<Visitor>(visitor), m_value);
  }

private:
  std::variant<Null, Bool, Int, Real, String, Array, Map> m_value;
};

class Serializable {
public:
  virtual ~Serializable() = default;

  virtual Value serialize() const = 0;
};

class Deserialazable {
public:
  virtual ~Deserialazable() = default;

  virtual void deserialize(Value value) = 0;
};

}; // namespace ser
