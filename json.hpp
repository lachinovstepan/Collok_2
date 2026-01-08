// json.hpp - минимальная версия
#pragma once

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

namespace nlohmann {

    class json {
    public:
        enum class value_t { null, object, array, string, boolean, number_integer, number_unsigned, number_float };

        json() : type(value_t::null) {}
        json(const std::string& v) : type(value_t::string), string_value(v) {}
        json(int v) : type(value_t::number_integer), int_value(v) {}
        json(bool v) : type(value_t::boolean), bool_value(v) {}
        json(const std::map<std::string, json>& v) : type(value_t::object), object_value(v) {}
        json(const std::vector<json>& v) : type(value_t::array), array_value(v) {}

        // Простые методы для нашего использования
        template<typename T>
        T value(const std::string& key, T default_value) const {
            if (type != value_t::object) return default_value;
            auto it = object_value.find(key);
            if (it == object_value.end()) return default_value;

            // Простейшая конвертация
            if constexpr (std::is_same_v<T, std::string>) {
                return it->second.type == value_t::string ? it->second.string_value : std::to_string(it->second.int_value);
            }
            else if constexpr (std::is_same_v<T, int>) {
                return it->second.int_value;
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return it->second.bool_value;
            }
            return default_value;
        }

        bool contains(const std::string& key) const {
            return type == value_t::object && object_value.find(key) != object_value.end();
        }

        std::string dump(int indent = -1) const {
            std::ostringstream oss;

            switch (type) {
            case value_t::string:
                oss << "\"" << string_value << "\"";
                break;
            case value_t::number_integer:
                oss << int_value;
                break;
            case value_t::boolean:
                oss << (bool_value ? "true" : "false");
                break;
            case value_t::object: {
                oss << "{";
                bool first = true;
                for (const auto& [key, val] : object_value) {
                    if (!first) oss << ",";
                    first = false;
                    oss << "\"" << key << "\":" << val.dump();
                }
                oss << "}";
                break;
            }
            case value_t::array: {
                oss << "[";
                bool first = true;
                for (const auto& val : array_value) {
                    if (!first) oss << ",";
                    first = false;
                    oss << val.dump();
                }
                oss << "]";
                break;
            }
            default:
                oss << "null";
            }

            return oss.str();
        }

        static json parse(const std::string& s) {
            // Простейший парсер для тестирования
            json result;

            if (s.empty()) return result;

            if (s[0] == '{') {
                // Простой объект типа {"key": "value"}
                result.type = value_t::object;
                std::string content = s.substr(1, s.size() - 2);
                size_t colon_pos = content.find(':');
                if (colon_pos != std::string::npos) {
                    std::string key = content.substr(1, content.find('"', 1) - 1);
                    std::string value = content.substr(colon_pos + 1);

                    // Убираем кавычки если есть
                    if (value.front() == '"' && value.back() == '"') {
                        value = value.substr(1, value.size() - 2);
                    }

                    result.object_value[key] = json(value);
                }
            }
            else if (s[0] == '[') {
                result.type = value_t::array;
            }
            else if (s[0] == '"') {
                result.type = value_t::string;
                result.string_value = s.substr(1, s.size() - 2);
            }
            else if (s == "true" || s == "false") {
                result.type = value_t::boolean;
                result.bool_value = (s == "true");
            }
            else {
                try {
                    result.type = value_t::number_integer;
                    result.int_value = std::stoi(s);
                }
                catch (...) {
                    result.type = value_t::string;
                    result.string_value = s;
                }
            }

            return result;
        }

        // Оператор [] для удобства
        json& operator[](const std::string& key) {
            if (type != value_t::object) {
                type = value_t::object;
            }
            return object_value[key];
        }

        const json& operator[](const std::string& key) const {
            static json empty;
            auto it = object_value.find(key);
            if (it != object_value.end()) return it->second;
            return empty;
        }

    private:
        value_t type;
        std::string string_value;
        int int_value = 0;
        bool bool_value = false;
        std::map<std::string, json> object_value;
        std::vector<json> array_value;
    };

} // namespace nlohmann