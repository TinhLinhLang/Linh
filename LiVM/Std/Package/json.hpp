#pragma once
#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include "../../Value/Value.hpp"
#include <simdjson.h>

namespace Linh {
namespace Std {
    using JsonFunction = std::function<Value(const Value&)>;

    // Forward declarations
    Value json_decode(const Value& v);
    Value json_encode(const Value& v);

    void initialize_json_functions();
    JsonFunction get_json_function(const std::string& function_name);
    std::vector<std::string> get_json_functions();

    // Implementation details
    inline std::unordered_map<std::string, JsonFunction>& json_functions_map() {
        static std::unordered_map<std::string, JsonFunction> fns;
        return fns;
    }

    // Helper: convert simdjson element -> Value recursively
    inline Value from_json_element(const simdjson::dom::element& el) {
        using simdjson::dom::element_type;
        switch (el.type()) {
            case element_type::OBJECT: {
                auto obj = el.get_object().value_unsafe();
                Map map = make_map();
                for (auto [k, v] : obj) {
                    // k is std::string_view
                    (*map)[std::string(k)] = from_json_element(v);
                }
                return Value(map);
            }
            case element_type::ARRAY: {
                auto arr = el.get_array().value_unsafe();
                Array a = make_array();
                for (auto v : arr) {
                    a->push_back(from_json_element(v));
                }
                return Value(a);
            }
            case element_type::STRING: {
                std::string_view sv = el.get_string().value_unsafe();
                return Value(std::string(sv));
            }
            case element_type::INT64: {
                int64_t iv = el.get_int64().value_unsafe();
                return Value(iv);
            }
            case element_type::UINT64: {
                uint64_t uv = el.get_uint64().value_unsafe();
                return Value(uv);
            }
            case element_type::DOUBLE: {
                double dv = el.get_double().value_unsafe();
                return Value(dv);
            }
            case element_type::BOOL: {
                bool bv = el.get_bool().value_unsafe();
                return Value(bv);
            }
            case element_type::NULL_VALUE:
            default:
                return Value{}; // sol
        }
    }

    // Helper: escape JSON string
    inline void escape_json_string(const std::string& s, std::string& out) {
        out.push_back('"');
        for (unsigned char c : s) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20) {
                        // control char -> \u00XX
                        static const char* hex = "0123456789abcdef";
                        out += "\\u00";
                        out.push_back(hex[(c >> 4) & 0xF]);
                        out.push_back(hex[c & 0xF]);
                    } else {
                        out.push_back(static_cast<char>(c));
                    }
            }
        }
        out.push_back('"');
    }

    // Helper: encode Value -> JSON recursively
    inline void encode_value(std::string& out, const Value& v) {
        switch (v.index()) {
            case 0: // sol/null
                out += "null"; break;
            case 1: // bool
                out += (std::get<bool>(v) ? "true" : "false"); break;
            case 2: { // int64
                out += std::to_string(std::get<int64_t>(v)); break; }
            case 3: { // uint64
                out += std::to_string(std::get<uint64_t>(v)); break; }
            case 4: { // double
                std::ostringstream oss; oss.setf(std::ios::fmtflags(0), std::ios::floatfield);
                oss << std::setprecision(15) << std::get<double>(v);
                out += oss.str(); break; }
            case 5: { // string
                const auto& s = std::get<std::string>(v);
                escape_json_string(s, out);
                break; }
            case 6: { // Array
                const auto& a = std::get<Array>(v);
                out.push_back('[');
                bool first = true;
                for (const auto& item : *a) {
                    if (!first) out.push_back(',');
                    first = false;
                    encode_value(out, item);
                }
                out.push_back(']');
                break; }
            case 7: { // Map
                const auto& m = std::get<Map>(v);
                out.push_back('{');
                bool first = true;
                for (const auto& kv : *m) {
                    if (!first) out.push_back(',');
                    first = false;
                    escape_json_string(kv.first, out);
                    out.push_back(':');
                    encode_value(out, kv.second);
                }
                out.push_back('}');
                break; }
            case 8: // FunctionPtr -> null
                out += "null"; break;
            case 9: { // Byte
                out += std::to_string(static_cast<uint64_t>(std::get<Byte>(v))); break; }
            case 10: { // ByteArray -> JSON array of numbers
                const auto& ba = std::get<ByteArray>(v);
                out.push_back('[');
                for (size_t i = 0; i < ba->size(); ++i) {
                    if (i) out.push_back(',');
                    out += std::to_string(static_cast<uint64_t>((*ba)[i]));
                }
                out.push_back(']');
                break; }
            default:
                out += "null"; break;
        }
    }

    // json.decode(string) -> any (map/array/primitive)
    inline Value json_decode(const Value& v) {
        if (!std::holds_alternative<std::string>(v)) return Value{}; // sol if not string
        const auto& json_str = std::get<std::string>(v);
        try {
            static thread_local simdjson::dom::parser parser;
            simdjson::dom::element doc = parser.parse(json_str);
            return from_json_element(doc);
        } catch (...) {
            return Value{}; // sol on error
        }
    }

    // json.encode(map|array|primitive) -> string
    inline Value json_encode(const Value& v) {
        std::string out;
        encode_value(out, v);
        return Value(out);
    }

    inline void initialize_json_functions() {
        auto& fns = json_functions_map();
        if (!fns.empty()) return;
        fns["decode"] = json_decode;
        fns["encode"] = json_encode;
    }

    inline JsonFunction get_json_function(const std::string& function_name) {
        auto& fns = json_functions_map();
        if (fns.empty()) initialize_json_functions();
        auto it = fns.find(function_name);
        if (it != fns.end()) return it->second;
        return nullptr;
    }

    inline std::vector<std::string> get_json_functions() {
        auto& fns = json_functions_map();
        if (fns.empty()) initialize_json_functions();
        std::vector<std::string> names;
        names.reserve(fns.size());
        for (const auto& kv : fns) names.push_back(kv.first);
        return names;
    }
}
}