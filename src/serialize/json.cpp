#include "json.hpp"

#include <cstdlib>
#include <spdlog/spdlog.h>
#include <tinygltf_json_c.h>

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

namespace ser {

static tg3json_value tg3jsonValueFor(const Value &value) {
  tg3json_value result;
  value.visit(overloaded{
      [&](const Value::Null &) { tg3json_value_init_null(&result); },
      [&](const Value::Bool &boolV) {
        tg3json_value_init_bool(&result, boolV);
      },
      [&](const Value::Int &intV) { tg3json_value_init_int(&result, intV); },
      [&](const Value::Real &realV) {
        tg3json_value_init_real(&result, realV);
      },
      [&](const Value::String &stringV) {
        tg3json_value_init_string(&result, stringV.c_str());
      },
      [&](const Value::Array &arrayV) {
        tg3json_value_init_array(&result);
        for (const Value &value : arrayV) {
          tg3json_value element = tg3jsonValueFor(value);
          tg3json_array_append_take(&result, &element);
        }
      },
      [&](const Value::Map &mapV) {
        tg3json_value_init_object(&result);
        for (const auto &[key, value] : mapV) {
          tg3json_value element = tg3jsonValueFor(value);
          tg3json_object_set_take(&result, key.c_str(), &element);
        }
      },
  });
  return result;
}

static Value valueFor(const tg3json_value &tg3jsonValue) {
  switch (tg3jsonValue.type) {
  case TG3JSON_NULL:
    return Value{};
  case TG3JSON_BOOL:
    return Value{tg3jsonValue.u.boolean != 0};
  case TG3JSON_INT:
    return Value{static_cast<Value::Int>(tg3jsonValue.u.integer)};
  case TG3JSON_REAL:
    return Value{static_cast<Value::Real>(tg3jsonValue.u.real)};
  case TG3JSON_STRING:
    return Value{
        Value::String(tg3jsonValue.u.string.ptr, tg3jsonValue.u.string.len)};
  case TG3JSON_ARRAY: {
    Value::Array array;
    array.reserve(tg3json_array_size(&tg3jsonValue));
    for (size_t i = 0, n = tg3json_array_size(&tg3jsonValue); i < n; ++i) {
      array.push_back(valueFor(*tg3json_array_get(&tg3jsonValue, i)));
    }
    return Value{std::move(array)};
  }
  case TG3JSON_OBJECT: {
    Value::Map map;
    map.reserve(tg3json_object_size(&tg3jsonValue));
    for (size_t i = 0, n = tg3json_object_size(&tg3jsonValue); i < n; ++i) {
      const tg3json_object_entry *entry = tg3json_object_at(&tg3jsonValue, i);
      map.emplace(std::string(entry->key, entry->key_len),
                  valueFor(*entry->value));
    }
    return Value{std::move(map)};
  }
  }
  return Value{};
}

std::string toJson(const Serializable &value) {
  tg3json_value tg3jsonValue = tg3jsonValueFor(value.serialize());

  size_t size;
  char *result = tg3json_stringify_pretty(&tg3jsonValue, 2, &size);

  tg3json_value_free(&tg3jsonValue);

  std::string out;
  if (result != nullptr) {
    out.assign(result, size);
    free(result);
  }
  return out;
}

void fromJson(Deserialazable &out, const std::string &json) {
  tg3json_value tg3jsonValue;
  const char *errorPos = nullptr;
  int ok = tg3json_parse(json.data(), json.data() + json.size(), 4096,
                         &tg3jsonValue, &errorPos);
  if (!ok) {
    spdlog::error("invalid JSON");
    exit(1);
  }

  Value value = valueFor(tg3jsonValue);
  tg3json_value_free(&tg3jsonValue);

  out.deserialize(std::move(value));
}

}; // namespace ser
