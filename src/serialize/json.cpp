#include "json.hpp"

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

std::string toJson(const Serializable &value) {
  tg3json_value tg3jsonValue = tg3jsonValueFor(value.serializeInfo());

  size_t size;
  char *result = tg3json_stringify_pretty(&tg3jsonValue, 2, &size);

  tg3json_value_free(&tg3jsonValue);

  return std::string(result, size);
}

}; // namespace ser
