#include "LiVM/Variable/type.hpp"
#include "LiVM/Functional/Func.hpp"
#include "LiVM/LiVM.hpp"
#include <fmt/core.h>
#include <stdexcept>
#include <cstdio>
#include <cmath>

namespace Linh
{
    // ============================================================================
    // VM Integration
    // ============================================================================
    
    void type(LiVM &vm)
    {
        vm.type();
    }

    // ============================================================================
    // Utility Functions
    // ============================================================================
    
    // Convert string to array<byte> (UTF-8 only for now)
    Value string_bytes(const std::string& s, const std::string& encoding)
    {
        if (!encoding.empty() && encoding != "utf-8" && encoding != "UTF-8")
            throw std::runtime_error("Only utf-8 encoding is supported");
        
        Value arr = Value::make_array();
        auto& vec = arr.as_array_ref();
        vec.reserve(s.size());
        for (unsigned char c : s)
            vec.push_back(Value(static_cast<uint8_t>(c)));
        return arr;
    }

    Value string_bytes(const Value& v, const std::string& encoding)
    {
        if (v.is_string())
            return string_bytes(v.as_string_ref(), encoding);
        throw std::runtime_error("Value is not a string");
    }

    // ============================================================================
    // Formatting Functions
    // ============================================================================
    
    std::string format_float(float value)
    {
        if (std::isnan(value)) return "NaN";
        if (std::isinf(value)) return value > 0 ? "+Inf" : "-Inf";
        if (value == 0.0f) return "0.0";
        
        std::string str = fmt::format("{:.6g}", value);
        
        if (str.find('e') != std::string::npos || str.find('E') != std::string::npos) {
            return str;
        }
        
        if (str.find('.') == std::string::npos) {
            return str + ".0";
        }
        
        size_t dot_pos = str.find('.');
        if (dot_pos != std::string::npos) {
            while (str.length() > dot_pos + 2 && str.back() == '0') {
                str.pop_back();
            }
        }
        
        return str;
    }
    
    std::string format_double(double value)
    {
        if (std::isnan(value)) return "NaN";
        if (std::isinf(value)) return value > 0 ? "+Inf" : "-Inf";
        if (value == 0.0) return "0.0";
        
        std::string str = fmt::format("{:.15g}", value);
        
        if (str.find('e') != std::string::npos || str.find('E') != std::string::npos) {
            return str;
        }
        
        if (str.find('.') == std::string::npos) {
            return str + ".0";
        }
        
        size_t dot_pos = str.find('.');
        if (dot_pos != std::string::npos) {
            while (str.length() > dot_pos + 2 && str.back() == '0') {
                str.pop_back();
            }
        }
        
        return str;
    }

    // Helper: format Array
    std::string format_array(const std::vector<Value> &arr)
    {
        std::string result = "[";
        for (size_t i = 0; i < arr.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            
            const Value& val = arr[i];
            
            if (val.is_string())
            {
                result += "\"" + val.as_string_ref() + "\"";
            }
            else if (val.get_type() == ValueType::Float32)
            {
                result += format_float(val.data.as_float32);
            }
            else if (val.get_type() == ValueType::Float64)
            {
                result += format_double(val.data.as_float64);
            }
            else
            {
                result += to_str(val);
            }
        }
        result += "]";
        return result;
    }

    // Helper: format Map
    std::string format_map(const std::unordered_map<std::string, Value> &map)
    {
        std::string result = "{";
        bool first = true;
        for (const auto &[key, value] : map)
        {
            if (!first)
                result += ", ";
            
            std::string key_str = "\"" + key + "\"";
            
            std::string value_str;
            if (value.is_string())
            {
                value_str = "\"" + value.as_string_ref() + "\"";
            }
            else if (value.get_type() == ValueType::Float32)
            {
                value_str = format_float(value.data.as_float32);
            }
            else if (value.get_type() == ValueType::Float64)
            {
                value_str = format_double(value.data.as_float64);
            }
            else
            {
                value_str = to_str(value);
            }
            
            result += key_str + ": " + value_str;
            first = false;
        }
        result += "}";
        return result;
    }

    // ============================================================================
    // Type Information Functions
    // ============================================================================
    
    std::string type_of(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::Sol: return "sol";
            case ValueType::Bool: return "bool";
            case ValueType::Int8: return "int8";
            case ValueType::Int16: return "int16";
            case ValueType::Int32: return "int32";
            case ValueType::Int64: return "int64";
            case ValueType::UInt8: return "uint8";
            case ValueType::UInt16: return "uint16";
            case ValueType::UInt32: return "uint32";
            case ValueType::UInt64: return "uint64";
            case ValueType::Float32: return "float32";
            case ValueType::Float64: return "float64";
            case ValueType::String: return "str";
            case ValueType::Array: return "array";
            case ValueType::Map: return "map";
            case ValueType::Function: return "function";
            default: return "unknown";
        }
    }

    std::string to_str(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::Sol:
                return "sol";
            case ValueType::Bool:
                return val.data.as_bool ? "true" : "false";
            case ValueType::Int8:
                return std::to_string(val.data.as_int8);
            case ValueType::Int16:
                return std::to_string(val.data.as_int16);
            case ValueType::Int32:
                return std::to_string(val.data.as_int32);
            case ValueType::Int64:
                return std::to_string(val.data.as_int64);
            case ValueType::UInt8:
                return std::to_string(val.data.as_uint8);
            case ValueType::UInt16:
                return std::to_string(val.data.as_uint16);
            case ValueType::UInt32:
                return std::to_string(val.data.as_uint32);
            case ValueType::UInt64:
                return std::to_string(val.data.as_uint64);
            case ValueType::Float32:
                return format_float(val.data.as_float32);
            case ValueType::Float64:
                return format_double(val.data.as_float64);
            case ValueType::String:
                return val.as_string_ref();
            case ValueType::Array:
                return format_array(val.as_array_ref());
            case ValueType::Map:
                return format_map(val.as_map_ref());
            case ValueType::Function:
            {
                const auto &fn = val.as_function_ref();
                std::string result = fmt::format("<function {}(", fn.name);
                for (size_t i = 0; i < fn.params.size(); ++i)
                {
                    if (i > 0)
                        result += ", ";
                    const auto &param = fn.params[i];
                    if (param.is_static)
                        result += "vas ";
                    result += param.name;
                    if (param.type.has_value())
                    {
                        result += ": " + param.type.value();
                    }
                }
                result += ")>";
                return result;
            }
            default:
                return "<unknown>";
        }
    }

    // ============================================================================
    // Type Conversion Functions
    // ============================================================================
    
    bool to_bool(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::Bool:
                return val.data.as_bool;
            case ValueType::Int8:
                return val.data.as_int8 != 0;
            case ValueType::Int16:
                return val.data.as_int16 != 0;
            case ValueType::Int32:
                return val.data.as_int32 != 0;
            case ValueType::Int64:
                return val.data.as_int64 != 0;
            case ValueType::UInt8:
                return val.data.as_uint8 != 0;
            case ValueType::UInt16:
                return val.data.as_uint16 != 0;
            case ValueType::UInt32:
                return val.data.as_uint32 != 0;
            case ValueType::UInt64:
                return val.data.as_uint64 != 0;
            case ValueType::Float32:
                return val.data.as_float32 != 0.0f;
            case ValueType::Float64:
                return val.data.as_float64 != 0.0;
            case ValueType::String:
                return !val.as_string_ref().empty();
            default:
                return false;
        }
    }

    int32_t len(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::Array:
                return static_cast<int32_t>(val.as_array_ref().size());
            case ValueType::Map:
                return static_cast<int32_t>(val.as_map_ref().size());
            case ValueType::String:
                return static_cast<int32_t>(val.as_string_ref().size());
            default:
                return 0;
        }
    }

    int8_t to_int8(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::Int8: return val.data.as_int8;
            case ValueType::Int16: return static_cast<int8_t>(val.data.as_int16);
            case ValueType::Int32: return static_cast<int8_t>(val.data.as_int32);
            case ValueType::Int64: return static_cast<int8_t>(val.data.as_int64);
            case ValueType::UInt8: return static_cast<int8_t>(val.data.as_uint8);
            case ValueType::UInt16: return static_cast<int8_t>(val.data.as_uint16);
            case ValueType::UInt32: return static_cast<int8_t>(val.data.as_uint32);
            case ValueType::UInt64: return static_cast<int8_t>(val.data.as_uint64);
            case ValueType::Float32: return static_cast<int8_t>(val.data.as_float32);
            case ValueType::Float64: return static_cast<int8_t>(val.data.as_float64);
            case ValueType::Bool: return val.data.as_bool ? 1 : 0;
            case ValueType::String:
                try { return static_cast<int8_t>(std::stoi(val.as_string_ref())); }
                catch (...) { return 0; }
            default: return 0;
        }
    }

    int16_t to_int16(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::Int8: return static_cast<int16_t>(val.data.as_int8);
            case ValueType::Int16: return val.data.as_int16;
            case ValueType::Int32: return static_cast<int16_t>(val.data.as_int32);
            case ValueType::Int64: return static_cast<int16_t>(val.data.as_int64);
            case ValueType::UInt8: return static_cast<int16_t>(val.data.as_uint8);
            case ValueType::UInt16: return static_cast<int16_t>(val.data.as_uint16);
            case ValueType::UInt32: return static_cast<int16_t>(val.data.as_uint32);
            case ValueType::UInt64: return static_cast<int16_t>(val.data.as_uint64);
            case ValueType::Float32: return static_cast<int16_t>(val.data.as_float32);
            case ValueType::Float64: return static_cast<int16_t>(val.data.as_float64);
            case ValueType::Bool: return val.data.as_bool ? 1 : 0;
            case ValueType::String:
                try { return static_cast<int16_t>(std::stoi(val.as_string_ref())); }
                catch (...) { return 0; }
            default: return 0;
        }
    }

    int32_t to_int32(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::Int8: return static_cast<int32_t>(val.data.as_int8);
            case ValueType::Int16: return static_cast<int32_t>(val.data.as_int16);
            case ValueType::Int32: return val.data.as_int32;
            case ValueType::Int64: return static_cast<int32_t>(val.data.as_int64);
            case ValueType::UInt8: return static_cast<int32_t>(val.data.as_uint8);
            case ValueType::UInt16: return static_cast<int32_t>(val.data.as_uint16);
            case ValueType::UInt32: return static_cast<int32_t>(val.data.as_uint32);
            case ValueType::UInt64: return static_cast<int32_t>(val.data.as_uint64);
            case ValueType::Float32: return static_cast<int32_t>(val.data.as_float32);
            case ValueType::Float64: return static_cast<int32_t>(val.data.as_float64);
            case ValueType::Bool: return val.data.as_bool ? 1 : 0;
            case ValueType::String:
                try { return std::stoi(val.as_string_ref()); }
                catch (...) { return 0; }
            default: return 0;
        }
    }

    int64_t to_int64(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::Int8: return static_cast<int64_t>(val.data.as_int8);
            case ValueType::Int16: return static_cast<int64_t>(val.data.as_int16);
            case ValueType::Int32: return static_cast<int64_t>(val.data.as_int32);
            case ValueType::Int64: return val.data.as_int64;
            case ValueType::UInt8: return static_cast<int64_t>(val.data.as_uint8);
            case ValueType::UInt16: return static_cast<int64_t>(val.data.as_uint16);
            case ValueType::UInt32: return static_cast<int64_t>(val.data.as_uint32);
            case ValueType::UInt64: return static_cast<int64_t>(val.data.as_uint64);
            case ValueType::Float32: return static_cast<int64_t>(val.data.as_float32);
            case ValueType::Float64: return static_cast<int64_t>(val.data.as_float64);
            case ValueType::Bool: return val.data.as_bool ? 1 : 0;
            case ValueType::String:
                try { return std::stoll(val.as_string_ref()); }
                catch (...) { return 0; }
            default: return 0;
        }
    }

    uint8_t to_uint8(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::UInt8: return val.data.as_uint8;
            case ValueType::Int8: return static_cast<uint8_t>(std::max<int8_t>(0, val.data.as_int8));
            case ValueType::Int16: return static_cast<uint8_t>(std::max<int16_t>(0, val.data.as_int16));
            case ValueType::Int32: return static_cast<uint8_t>(std::max<int32_t>(0, val.data.as_int32));
            case ValueType::Int64: return static_cast<uint8_t>(std::max<int64_t>(0, val.data.as_int64));
            case ValueType::UInt16: return static_cast<uint8_t>(val.data.as_uint16);
            case ValueType::UInt32: return static_cast<uint8_t>(val.data.as_uint32);
            case ValueType::UInt64: return static_cast<uint8_t>(val.data.as_uint64);
            case ValueType::Float32: return static_cast<uint8_t>(std::max<float>(0.0f, val.data.as_float32));
            case ValueType::Float64: return static_cast<uint8_t>(std::max<double>(0.0, val.data.as_float64));
            case ValueType::Bool: return val.data.as_bool ? 1 : 0;
            case ValueType::String:
                try { return static_cast<uint8_t>(std::stoul(val.as_string_ref())); }
                catch (...) { return 0; }
            default: return 0;
        }
    }

    uint16_t to_uint16(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::UInt16: return val.data.as_uint16;
            case ValueType::Int8: return static_cast<uint16_t>(std::max<int8_t>(0, val.data.as_int8));
            case ValueType::Int16: return static_cast<uint16_t>(std::max<int16_t>(0, val.data.as_int16));
            case ValueType::Int32: return static_cast<uint16_t>(std::max<int32_t>(0, val.data.as_int32));
            case ValueType::Int64: return static_cast<uint16_t>(std::max<int64_t>(0, val.data.as_int64));
            case ValueType::UInt8: return static_cast<uint16_t>(val.data.as_uint8);
            case ValueType::UInt32: return static_cast<uint16_t>(val.data.as_uint32);
            case ValueType::UInt64: return static_cast<uint16_t>(val.data.as_uint64);
            case ValueType::Float32: return static_cast<uint16_t>(std::max<float>(0.0f, val.data.as_float32));
            case ValueType::Float64: return static_cast<uint16_t>(std::max<double>(0.0, val.data.as_float64));
            case ValueType::Bool: return val.data.as_bool ? 1 : 0;
            case ValueType::String:
                try { return static_cast<uint16_t>(std::stoul(val.as_string_ref())); }
                catch (...) { return 0; }
            default: return 0;
        }
    }

    uint32_t to_uint32(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::UInt32: return val.data.as_uint32;
            case ValueType::Int8: return static_cast<uint32_t>(std::max<int8_t>(0, val.data.as_int8));
            case ValueType::Int16: return static_cast<uint32_t>(std::max<int16_t>(0, val.data.as_int16));
            case ValueType::Int32: return static_cast<uint32_t>(std::max<int32_t>(0, val.data.as_int32));
            case ValueType::Int64: return static_cast<uint32_t>(std::max<int64_t>(0, val.data.as_int64));
            case ValueType::UInt8: return static_cast<uint32_t>(val.data.as_uint8);
            case ValueType::UInt16: return static_cast<uint32_t>(val.data.as_uint16);
            case ValueType::UInt64: return static_cast<uint32_t>(val.data.as_uint64);
            case ValueType::Float32: return static_cast<uint32_t>(std::max<float>(0.0f, val.data.as_float32));
            case ValueType::Float64: return static_cast<uint32_t>(std::max<double>(0.0, val.data.as_float64));
            case ValueType::Bool: return val.data.as_bool ? 1 : 0;
            case ValueType::String:
                try { return std::stoul(val.as_string_ref()); }
                catch (...) { return 0; }
            default: return 0;
        }
    }

    uint64_t to_uint64(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::UInt64: return val.data.as_uint64;
            case ValueType::Int8: return static_cast<uint64_t>(std::max<int8_t>(0, val.data.as_int8));
            case ValueType::Int16: return static_cast<uint64_t>(std::max<int16_t>(0, val.data.as_int16));
            case ValueType::Int32: return static_cast<uint64_t>(std::max<int32_t>(0, val.data.as_int32));
            case ValueType::Int64: return static_cast<uint64_t>(std::max<int64_t>(0, val.data.as_int64));
            case ValueType::UInt8: return static_cast<uint64_t>(val.data.as_uint8);
            case ValueType::UInt16: return static_cast<uint64_t>(val.data.as_uint16);
            case ValueType::UInt32: return static_cast<uint64_t>(val.data.as_uint32);
            case ValueType::Float32: return static_cast<uint64_t>(std::max<float>(0.0f, val.data.as_float32));
            case ValueType::Float64: return static_cast<uint64_t>(std::max<double>(0.0, val.data.as_float64));
            case ValueType::Bool: return val.data.as_bool ? 1 : 0;
            case ValueType::String:
                try { return std::stoull(val.as_string_ref()); }
                catch (...) { return 0; }
            default: return 0;
        }
    }

    float to_float32(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::Float32: return val.data.as_float32;
            case ValueType::Float64: return static_cast<float>(val.data.as_float64);
            case ValueType::Int8: return static_cast<float>(val.data.as_int8);
            case ValueType::Int16: return static_cast<float>(val.data.as_int16);
            case ValueType::Int32: return static_cast<float>(val.data.as_int32);
            case ValueType::Int64: return static_cast<float>(val.data.as_int64);
            case ValueType::UInt8: return static_cast<float>(val.data.as_uint8);
            case ValueType::UInt16: return static_cast<float>(val.data.as_uint16);
            case ValueType::UInt32: return static_cast<float>(val.data.as_uint32);
            case ValueType::UInt64: return static_cast<float>(val.data.as_uint64);
            case ValueType::Bool: return val.data.as_bool ? 1.0f : 0.0f;
            case ValueType::String:
                try { return std::stof(val.as_string_ref()); }
                catch (...) { return 0.0f; }
            default: return 0.0f;
        }
    }

    double to_float64(const Value &val)
    {
        switch (val.get_type()) {
            case ValueType::Float64: return val.data.as_float64;
            case ValueType::Float32: return static_cast<double>(val.data.as_float32);
            case ValueType::Int8: return static_cast<double>(val.data.as_int8);
            case ValueType::Int16: return static_cast<double>(val.data.as_int16);
            case ValueType::Int32: return static_cast<double>(val.data.as_int32);
            case ValueType::Int64: return static_cast<double>(val.data.as_int64);
            case ValueType::UInt8: return static_cast<double>(val.data.as_uint8);
            case ValueType::UInt16: return static_cast<double>(val.data.as_uint16);
            case ValueType::UInt32: return static_cast<double>(val.data.as_uint32);
            case ValueType::UInt64: return static_cast<double>(val.data.as_uint64);
            case ValueType::Bool: return val.data.as_bool ? 1.0 : 0.0;
            case ValueType::String:
                try { return std::stod(val.as_string_ref()); }
                catch (...) { return 0.0; }
            default: return 0.0;
        }
    }

    // ============================================================================
    // Value Comparison
    // ============================================================================
    
    bool values_equal(const Value &a, const Value &b)
    {
        // Different types are never equal
        if (a.get_type() != b.get_type()) {
            return false;
        }
        
        switch (a.get_type()) {
            case ValueType::Sol:
                return true;
            case ValueType::Bool:
                return a.data.as_bool == b.data.as_bool;
            case ValueType::Int8:
                return a.data.as_int8 == b.data.as_int8;
            case ValueType::Int16:
                return a.data.as_int16 == b.data.as_int16;
            case ValueType::Int32:
                return a.data.as_int32 == b.data.as_int32;
            case ValueType::Int64:
                return a.data.as_int64 == b.data.as_int64;
            case ValueType::UInt8:
                return a.data.as_uint8 == b.data.as_uint8;
            case ValueType::UInt16:
                return a.data.as_uint16 == b.data.as_uint16;
            case ValueType::UInt32:
                return a.data.as_uint32 == b.data.as_uint32;
            case ValueType::UInt64:
                return a.data.as_uint64 == b.data.as_uint64;
            case ValueType::Float32:
                return a.data.as_float32 == b.data.as_float32;
            case ValueType::Float64:
                return a.data.as_float64 == b.data.as_float64;
            case ValueType::String:
                return a.as_string_ref() == b.as_string_ref();
            case ValueType::Array: {
                const auto& arr_a = a.as_array_ref();
                const auto& arr_b = b.as_array_ref();
                if (arr_a.size() != arr_b.size()) return false;
                for (size_t i = 0; i < arr_a.size(); ++i) {
                    if (!values_equal(arr_a[i], arr_b[i])) return false;
                }
                return true;
            }
            case ValueType::Map: {
                const auto& map_a = a.as_map_ref();
                const auto& map_b = b.as_map_ref();
                if (map_a.size() != map_b.size()) return false;
                for (const auto& [key, value] : map_a) {
                    auto it = map_b.find(key);
                    if (it == map_b.end() || !values_equal(value, it->second)) {
                        return false;
                    }
                }
                return true;
            }
            case ValueType::Function:
                // Functions are compared by pointer equality
                return a.data.as_function == b.data.as_function;
            default:
                return false;
        }
    }
}
