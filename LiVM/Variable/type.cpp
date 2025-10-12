#include "LiVM/Variable/type.hpp"
#include "LiVM/Functional/Func.hpp"
#include <fmt/core.h>
#include <stdexcept>
#include <cstdio>
#include <cmath>

namespace Linh
{
    // Định nghĩa hàm type() cho LiVM (wrapper)
    void type(LiVM &vm)
    {
        vm.type();
    }


    // Convert string to array<byte> (UTF-8 only for now)
    Array string_bytes(const std::string& s, const std::string& encoding)
    {
        Array arr = make_array();
        if (!encoding.empty() && encoding != "utf-8" && encoding != "UTF-8")
            throw std::runtime_error("Only utf-8 encoding is supported");
        arr->reserve(s.size());
        for (unsigned char c : s)
            arr->push_back(Value(static_cast<Byte>(c)));
        return arr;
    }

    Array string_bytes(const Value& v, const std::string& encoding)
    {
        if (std::holds_alternative<std::string>(v))
            return string_bytes(std::get<std::string>(v), encoding);
        throw std::runtime_error("Value is not a string");
    }

    /* Khung vực định nghĩa format của Linh */
    // 1. format số thực với quy tắc làm tròn giống Go/Java
    std::string format_float(float value)
    {
        // Xử lý các trường hợp đặc biệt
        if (std::isnan(value)) return "NaN";
        if (std::isinf(value)) return value > 0 ? "+Inf" : "-Inf";
        if (value == 0.0f) return "0.0";  // Float zero luôn hiển thị với .0
        
        // Sử dụng độ chính xác thích hợp cho float32 (6-7 chữ số có nghĩa)
        std::string str = fmt::format("{:.6g}", value);
        
        // Nếu có ký hiệu khoa học, trả về trực tiếp
        if (str.find('e') != std::string::npos || str.find('E') != std::string::npos) {
            return str;
        }
        
        // Nếu kết quả không có dấu chấm (số nguyên), thêm .0 để phân biệt với int
        if (str.find('.') == std::string::npos) {
            return str + ".0";
        }
        
        // Xử lý số thập phân thông thường
        // Loại bỏ số 0 thừa ở cuối nhưng luôn giữ lại ít nhất một chữ số sau dấu chấm
        size_t dot_pos = str.find('.');
        if (dot_pos != std::string::npos) {
            // Loại bỏ số 0 ở cuối nhưng giữ lại ít nhất một chữ số sau dấu chấm
            while (str.length() > dot_pos + 2 && str.back() == '0') {
                str.pop_back();
            }
        }
        
        return str;
    }

    // 2. format Array
    std::string format_array(const Array &arr)
    {
        std::string result = "[";
        for (size_t i = 0; i < arr->size(); ++i)
        {
            if (i > 0)
                result += ", ";
            
            const Value& val = (*arr)[i];
            
            // Xử lý chuỗi: thêm dấu ngoặc kép
            if (std::holds_alternative<std::string>(val))
            {
                result += "\"" + std::get<std::string>(val) + "\"";
            }
            // Xử lý số thực: sử dụng format_float
            else if (std::holds_alternative<float>(val))
            {
                result += format_float(std::get<float>(val));
            }
            // Các loại khác: sử dụng to_str như cũ
            else
            {
                result += to_str(val);
            }
        }
        result += "]";
        return result;
    }

    // 3. format Map
    std::string format_map(const Map &map)
    {
        std::string result = "{";
        bool first = true;
        for (const auto &[key, value] : *map)
        {
            if (!first)
                result += ", ";
            
            // Format key (luôn là string trong Map)
            std::string key_str = "\"" + key + "\"";
            
            // Format value
            std::string value_str;
            if (std::holds_alternative<std::string>(value))
            {
                value_str = "\"" + std::get<std::string>(value) + "\"";
            }
            else if (std::holds_alternative<float>(value))
            {
                value_str = format_float(std::get<float>(value));
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

    std::string type_of(const Value &val)
    {
        if (std::holds_alternative<std::monostate>(val))
            return "sol";
        if (std::holds_alternative<int8_t>(val))
            return "int8";
        if (std::holds_alternative<int16_t>(val))
            return "int16";
        if (std::holds_alternative<int32_t>(val))
            return "int32";
        if (std::holds_alternative<int64_t>(val))
            return "int64";
        if (std::holds_alternative<uint8_t>(val))
            return "uint8";
        if (std::holds_alternative<uint16_t>(val))
            return "uint16";
        if (std::holds_alternative<uint32_t>(val))
            return "uint32";
        if (std::holds_alternative<uint64_t>(val))
            return "uint64";
        if (std::holds_alternative<float>(val))
            return "float32";
        if (std::holds_alternative<double>(val))
            return "float64";
        if (std::holds_alternative<std::string>(val))
            return "str";
        if (std::holds_alternative<bool>(val))
            return "bool";
        if (std::holds_alternative<Array>(val))
            return "array";
        if (std::holds_alternative<Map>(val))
            return "map";
        if (std::holds_alternative<FunctionPtr>(val))
            return "function";
        if (std::holds_alternative<Byte>(val))
            return "byte";
        return "unknown";
    }

    std::string to_str(const Value &val)
    {
        if (std::holds_alternative<std::monostate>(val))
            return std::string("sol");
        if (std::holds_alternative<int8_t>(val))
            return std::to_string(std::get<int8_t>(val));
        if (std::holds_alternative<int16_t>(val))
            return std::to_string(std::get<int16_t>(val));
        if (std::holds_alternative<int32_t>(val))
            return std::to_string(std::get<int32_t>(val));
        if (std::holds_alternative<int64_t>(val))
            return std::to_string(std::get<int64_t>(val));
        if (std::holds_alternative<uint8_t>(val))
            return std::to_string(std::get<uint8_t>(val));
        if (std::holds_alternative<uint16_t>(val))
            return std::to_string(std::get<uint16_t>(val));
        if (std::holds_alternative<uint32_t>(val))
            return std::to_string(std::get<uint32_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return std::to_string(std::get<uint64_t>(val));
        if (std::holds_alternative<float>(val))
            return format_float(std::get<float>(val));
        if (std::holds_alternative<double>(val))
            return format_float(static_cast<float>(std::get<double>(val)));
        if (std::holds_alternative<std::string>(val))
            return std::get<std::string>(val);
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? "true" : "false";
        if (std::holds_alternative<Array>(val))
            return format_array(std::get<Array>(val));
        if (std::holds_alternative<Map>(val))
            return format_map(std::get<Map>(val));
        if (std::holds_alternative<FunctionPtr>(val))
        {
            const auto &fn = std::get<FunctionPtr>(val);
            std::string result = fmt::format("<function {}(", fn->name);
            for (size_t i = 0; i < fn->params.size(); ++i)
            {
                if (i > 0)
                    result += ", ";
                const auto &param = fn->params[i];
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
        if (std::holds_alternative<Byte>(val))
        {
            char buf[5];
            std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(std::get<Byte>(val)));
            return std::string(buf);
        }
        return "<unknown>";
    }

    // Add a new function to convert Value to std::string
    std::string to_string(const Value &val)
    {
        if (std::holds_alternative<std::string>(val))
            return std::get<std::string>(val);
        return to_str(val);
    }

    bool to_bool(const Value &val)
    {
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val);
        if (std::holds_alternative<int32_t>(val))
            return std::get<int32_t>(val) != 0;
        if (std::holds_alternative<float>(val))
            return std::get<float>(val) != 0.0f;
        if (std::holds_alternative<std::string>(val))
            return !std::get<std::string>(val).empty();
        if (std::holds_alternative<Byte>(val))
            return std::get<Byte>(val) != 0;
        return false;
    }

    int32_t len(const Value &val)
    {
        if (std::holds_alternative<Array>(val))
        {
            const auto &arr = std::get<Array>(val);
            return arr ? static_cast<int32_t>(arr->size()) : 0;
        }
        if (std::holds_alternative<Map>(val))
        {
            const auto &map = std::get<Map>(val);
            return map ? static_cast<int32_t>(map->size()) : 0;
        }
        if (std::holds_alternative<std::string>(val))
        {
            return static_cast<int32_t>(std::get<std::string>(val).size());
        }
        return 0;
    }

    // Specific type conversion functions
    int8_t to_int8(const Value &val)
    {
        if (std::holds_alternative<int8_t>(val))
            return std::get<int8_t>(val);
        if (std::holds_alternative<int16_t>(val))
            return static_cast<int8_t>(std::get<int16_t>(val));
        if (std::holds_alternative<int32_t>(val))
            return static_cast<int8_t>(std::get<int32_t>(val));
        if (std::holds_alternative<int64_t>(val))
            return static_cast<int8_t>(std::get<int64_t>(val));
        if (std::holds_alternative<uint8_t>(val))
            return static_cast<int8_t>(std::get<uint8_t>(val));
        if (std::holds_alternative<uint16_t>(val))
            return static_cast<int8_t>(std::get<uint16_t>(val));
        if (std::holds_alternative<uint32_t>(val))
            return static_cast<int8_t>(std::get<uint32_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return static_cast<int8_t>(std::get<uint64_t>(val));
        if (std::holds_alternative<float>(val))
            return static_cast<int8_t>(std::get<float>(val));
        if (std::holds_alternative<double>(val))
            return static_cast<int8_t>(std::get<double>(val));
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1 : 0;
        if (std::holds_alternative<std::string>(val))
        {
            try { return static_cast<int8_t>(std::stoi(std::get<std::string>(val))); }
            catch (...) { return 0; }
        }
        return 0;
    }

    int16_t to_int16(const Value &val)
    {
        if (std::holds_alternative<int16_t>(val))
            return std::get<int16_t>(val);
        if (std::holds_alternative<int8_t>(val))
            return static_cast<int16_t>(std::get<int8_t>(val));
        if (std::holds_alternative<int32_t>(val))
            return static_cast<int16_t>(std::get<int32_t>(val));
        if (std::holds_alternative<int64_t>(val))
            return static_cast<int16_t>(std::get<int64_t>(val));
        if (std::holds_alternative<uint8_t>(val))
            return static_cast<int16_t>(std::get<uint8_t>(val));
        if (std::holds_alternative<uint16_t>(val))
            return static_cast<int16_t>(std::get<uint16_t>(val));
        if (std::holds_alternative<uint32_t>(val))
            return static_cast<int16_t>(std::get<uint32_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return static_cast<int16_t>(std::get<uint64_t>(val));
        if (std::holds_alternative<float>(val))
            return static_cast<int16_t>(std::get<float>(val));
        if (std::holds_alternative<double>(val))
            return static_cast<int16_t>(std::get<double>(val));
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1 : 0;
        if (std::holds_alternative<std::string>(val))
        {
            try { return static_cast<int16_t>(std::stoi(std::get<std::string>(val))); }
            catch (...) { return 0; }
        }
        return 0;
    }

    int32_t to_int32(const Value &val)
    {
        if (std::holds_alternative<int32_t>(val))
            return std::get<int32_t>(val);
        if (std::holds_alternative<int8_t>(val))
            return static_cast<int32_t>(std::get<int8_t>(val));
        if (std::holds_alternative<int16_t>(val))
            return static_cast<int32_t>(std::get<int16_t>(val));
        if (std::holds_alternative<int64_t>(val))
            return static_cast<int32_t>(std::get<int64_t>(val));
        if (std::holds_alternative<uint8_t>(val))
            return static_cast<int32_t>(std::get<uint8_t>(val));
        if (std::holds_alternative<uint16_t>(val))
            return static_cast<int32_t>(std::get<uint16_t>(val));
        if (std::holds_alternative<uint32_t>(val))
            return static_cast<int32_t>(std::get<uint32_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return static_cast<int32_t>(std::get<uint64_t>(val));
        if (std::holds_alternative<float>(val))
            return static_cast<int32_t>(std::get<float>(val));
        if (std::holds_alternative<double>(val))
            return static_cast<int32_t>(std::get<double>(val));
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1 : 0;
        if (std::holds_alternative<std::string>(val))
        {
            try { return std::stoi(std::get<std::string>(val)); }
            catch (...) { return 0; }
        }
        return 0;
    }

    int64_t to_int64(const Value &val)
    {
        if (std::holds_alternative<int64_t>(val))
            return std::get<int64_t>(val);
        if (std::holds_alternative<int8_t>(val))
            return static_cast<int64_t>(std::get<int8_t>(val));
        if (std::holds_alternative<int16_t>(val))
            return static_cast<int64_t>(std::get<int16_t>(val));
        if (std::holds_alternative<int32_t>(val))
            return static_cast<int64_t>(std::get<int32_t>(val));
        if (std::holds_alternative<uint8_t>(val))
            return static_cast<int64_t>(std::get<uint8_t>(val));
        if (std::holds_alternative<uint16_t>(val))
            return static_cast<int64_t>(std::get<uint16_t>(val));
        if (std::holds_alternative<uint32_t>(val))
            return static_cast<int64_t>(std::get<uint32_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return static_cast<int64_t>(std::get<uint64_t>(val));
        if (std::holds_alternative<float>(val))
            return static_cast<int64_t>(std::get<float>(val));
        if (std::holds_alternative<double>(val))
            return static_cast<int64_t>(std::get<double>(val));
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1 : 0;
        if (std::holds_alternative<std::string>(val))
        {
            try { return std::stoll(std::get<std::string>(val)); }
            catch (...) { return 0; }
        }
        return 0;
    }

    uint8_t to_uint8(const Value &val)
    {
        if (std::holds_alternative<uint8_t>(val))
            return std::get<uint8_t>(val);
        if (std::holds_alternative<int8_t>(val))
            return static_cast<uint8_t>(std::max<int8_t>(0, std::get<int8_t>(val)));
        if (std::holds_alternative<int16_t>(val))
            return static_cast<uint8_t>(std::max<int16_t>(0, std::get<int16_t>(val)));
        if (std::holds_alternative<int32_t>(val))
            return static_cast<uint8_t>(std::max<int32_t>(0, std::get<int32_t>(val)));
        if (std::holds_alternative<int64_t>(val))
            return static_cast<uint8_t>(std::max<int64_t>(0, std::get<int64_t>(val)));
        if (std::holds_alternative<uint16_t>(val))
            return static_cast<uint8_t>(std::get<uint16_t>(val));
        if (std::holds_alternative<uint32_t>(val))
            return static_cast<uint8_t>(std::get<uint32_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return static_cast<uint8_t>(std::get<uint64_t>(val));
        if (std::holds_alternative<float>(val))
            return static_cast<uint8_t>(std::max<float>(0.0f, std::get<float>(val)));
        if (std::holds_alternative<double>(val))
            return static_cast<uint8_t>(std::max<double>(0.0, std::get<double>(val)));
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1 : 0;
        if (std::holds_alternative<std::string>(val))
        {
            try { return static_cast<uint8_t>(std::stoul(std::get<std::string>(val))); }
            catch (...) { return 0; }
        }
        return 0;
    }

    uint16_t to_uint16(const Value &val)
    {
        if (std::holds_alternative<uint16_t>(val))
            return std::get<uint16_t>(val);
        if (std::holds_alternative<int8_t>(val))
            return static_cast<uint16_t>(std::max<int8_t>(0, std::get<int8_t>(val)));
        if (std::holds_alternative<int16_t>(val))
            return static_cast<uint16_t>(std::max<int16_t>(0, std::get<int16_t>(val)));
        if (std::holds_alternative<int32_t>(val))
            return static_cast<uint16_t>(std::max<int32_t>(0, std::get<int32_t>(val)));
        if (std::holds_alternative<int64_t>(val))
            return static_cast<uint16_t>(std::max<int64_t>(0, std::get<int64_t>(val)));
        if (std::holds_alternative<uint8_t>(val))
            return static_cast<uint16_t>(std::get<uint8_t>(val));
        if (std::holds_alternative<uint32_t>(val))
            return static_cast<uint16_t>(std::get<uint32_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return static_cast<uint16_t>(std::get<uint64_t>(val));
        if (std::holds_alternative<float>(val))
            return static_cast<uint16_t>(std::max<float>(0.0f, std::get<float>(val)));
        if (std::holds_alternative<double>(val))
            return static_cast<uint16_t>(std::max<double>(0.0, std::get<double>(val)));
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1 : 0;
        if (std::holds_alternative<std::string>(val))
        {
            try { return static_cast<uint16_t>(std::stoul(std::get<std::string>(val))); }
            catch (...) { return 0; }
        }
        return 0;
    }

    uint32_t to_uint32(const Value &val)
    {
        if (std::holds_alternative<uint32_t>(val))
            return std::get<uint32_t>(val);
        if (std::holds_alternative<int8_t>(val))
            return static_cast<uint32_t>(std::max<int8_t>(0, std::get<int8_t>(val)));
        if (std::holds_alternative<int16_t>(val))
            return static_cast<uint32_t>(std::max<int16_t>(0, std::get<int16_t>(val)));
        if (std::holds_alternative<int32_t>(val))
            return static_cast<uint32_t>(std::max<int32_t>(0, std::get<int32_t>(val)));
        if (std::holds_alternative<int64_t>(val))
            return static_cast<uint32_t>(std::max<int64_t>(0, std::get<int64_t>(val)));
        if (std::holds_alternative<uint8_t>(val))
            return static_cast<uint32_t>(std::get<uint8_t>(val));
        if (std::holds_alternative<uint16_t>(val))
            return static_cast<uint32_t>(std::get<uint16_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return static_cast<uint32_t>(std::get<uint64_t>(val));
        if (std::holds_alternative<float>(val))
            return static_cast<uint32_t>(std::max<float>(0.0f, std::get<float>(val)));
        if (std::holds_alternative<double>(val))
            return static_cast<uint32_t>(std::max<double>(0.0, std::get<double>(val)));
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1 : 0;
        if (std::holds_alternative<std::string>(val))
        {
            try { return std::stoul(std::get<std::string>(val)); }
            catch (...) { return 0; }
        }
        return 0;
    }

    uint64_t to_uint64(const Value &val)
    {
        if (std::holds_alternative<uint64_t>(val))
            return std::get<uint64_t>(val);
        if (std::holds_alternative<int8_t>(val))
            return static_cast<uint64_t>(std::max<int8_t>(0, std::get<int8_t>(val)));
        if (std::holds_alternative<int16_t>(val))
            return static_cast<uint64_t>(std::max<int16_t>(0, std::get<int16_t>(val)));
        if (std::holds_alternative<int32_t>(val))
            return static_cast<uint64_t>(std::max<int32_t>(0, std::get<int32_t>(val)));
        if (std::holds_alternative<int64_t>(val))
            return static_cast<uint64_t>(std::max<int64_t>(0, std::get<int64_t>(val)));
        if (std::holds_alternative<uint8_t>(val))
            return static_cast<uint64_t>(std::get<uint8_t>(val));
        if (std::holds_alternative<uint16_t>(val))
            return static_cast<uint64_t>(std::get<uint16_t>(val));
        if (std::holds_alternative<uint32_t>(val))
            return static_cast<uint64_t>(std::get<uint32_t>(val));
        if (std::holds_alternative<float>(val))
            return static_cast<uint64_t>(std::max<float>(0.0f, std::get<float>(val)));
        if (std::holds_alternative<double>(val))
            return static_cast<uint64_t>(std::max<double>(0.0, std::get<double>(val)));
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1 : 0;
        if (std::holds_alternative<std::string>(val))
        {
            try { return std::stoull(std::get<std::string>(val)); }
            catch (...) { return 0; }
        }
        return 0;
    }

    float to_float32(const Value &val)
    {
        if (std::holds_alternative<float>(val))
            return std::get<float>(val);
        if (std::holds_alternative<double>(val))
            return static_cast<float>(std::get<double>(val));
        if (std::holds_alternative<int8_t>(val))
            return static_cast<float>(std::get<int8_t>(val));
        if (std::holds_alternative<int16_t>(val))
            return static_cast<float>(std::get<int16_t>(val));
        if (std::holds_alternative<int32_t>(val))
            return static_cast<float>(std::get<int32_t>(val));
        if (std::holds_alternative<int64_t>(val))
            return static_cast<float>(std::get<int64_t>(val));
        if (std::holds_alternative<uint8_t>(val))
            return static_cast<float>(std::get<uint8_t>(val));
        if (std::holds_alternative<uint16_t>(val))
            return static_cast<float>(std::get<uint16_t>(val));
        if (std::holds_alternative<uint32_t>(val))
            return static_cast<float>(std::get<uint32_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return static_cast<float>(std::get<uint64_t>(val));
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1.0f : 0.0f;
        if (std::holds_alternative<std::string>(val))
        {
            try { return std::stof(std::get<std::string>(val)); }
            catch (...) { return 0.0f; }
        }
        return 0.0f;
    }

    double to_float64(const Value &val)
    {
        if (std::holds_alternative<double>(val))
            return std::get<double>(val);
        if (std::holds_alternative<float>(val))
            return static_cast<double>(std::get<float>(val));
        if (std::holds_alternative<int8_t>(val))
            return static_cast<double>(std::get<int8_t>(val));
        if (std::holds_alternative<int16_t>(val))
            return static_cast<double>(std::get<int16_t>(val));
        if (std::holds_alternative<int32_t>(val))
            return static_cast<double>(std::get<int32_t>(val));
        if (std::holds_alternative<int64_t>(val))
            return static_cast<double>(std::get<int64_t>(val));
        if (std::holds_alternative<uint8_t>(val))
            return static_cast<double>(std::get<uint8_t>(val));
        if (std::holds_alternative<uint16_t>(val))
            return static_cast<double>(std::get<uint16_t>(val));
        if (std::holds_alternative<uint32_t>(val))
            return static_cast<double>(std::get<uint32_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return static_cast<double>(std::get<uint64_t>(val));
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1.0 : 0.0;
        if (std::holds_alternative<std::string>(val))
        {
            try { return std::stod(std::get<std::string>(val)); }
            catch (...) { return 0.0; }
        }
        return 0.0;
    }

    // Hàm so sánh hai Value
    bool values_equal(const Value &a, const Value &b)
    {
        // Nếu cùng kiểu variant, so sánh trực tiếp
        if (a.index() != b.index()) {
            return false;
        }
        
        return std::visit([](const auto& val_a, const auto& val_b) -> bool {
            using T = std::decay_t<decltype(val_a)>;
            using U = std::decay_t<decltype(val_b)>;
            
            if constexpr (std::is_same_v<T, U>) {
                if constexpr (std::is_same_v<T, Array>) {
                    // So sánh array: cùng size và tất cả elements bằng nhau
                    if (!val_a || !val_b) return val_a == val_b;
                    if (val_a->size() != val_b->size()) return false;
                    for (size_t i = 0; i < val_a->size(); ++i) {
                        if (!values_equal((*val_a)[i], (*val_b)[i])) return false;
                    }
                    return true;
                } else if constexpr (std::is_same_v<T, Map>) {
                    // So sánh map: cùng size và tất cả key-value pairs bằng nhau
                    if (!val_a || !val_b) return val_a == val_b;
                    if (val_a->size() != val_b->size()) return false;
                    for (const auto& [key, value] : *val_a) {
                        auto it = val_b->find(key);
                        if (it == val_b->end() || !values_equal(value, it->second)) {
                            return false;
                        }
                    }
                    return true;
                } else {
                    // Các kiểu primitive: so sánh trực tiếp
                    return val_a == val_b;
                }
            } else {
                return false;
            }
        }, a, b);
    }
}
