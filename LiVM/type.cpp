#include "type.hpp"
#include "Functional/Func.hpp"
#include <fmt/core.h>
#include <stdexcept>
#include <cstdio>

namespace Linh
{
    // Định nghĩa hàm type() cho LiVM (wrapper)
    void type(LiVM &vm)
    {
        vm.type();
    }

    // Chuyển string thành ByteArray (encoding: utf-8)
    ByteArray string_bytes(const std::string& s, const std::string& encoding)
    {
        // Hiện tại chỉ hỗ trợ utf-8
        ByteArray arr = make_bytearray();
        if (encoding == "utf-8" || encoding == "UTF-8" || encoding.empty())
        {
            arr->reserve(s.size());
            for (unsigned char c : s)
                arr->push_back(c);
        }
        else
        {
            // Có thể bổ sung các encoding khác nếu cần
            throw std::runtime_error("Only utf-8 encoding is supported");
        }
        return arr;
    }

    ByteArray string_bytes(const Value& v, const std::string& encoding)
    {
        if (std::holds_alternative<std::string>(v))
        {
            return string_bytes(std::get<std::string>(v), encoding);
        }
        throw std::runtime_error("Value is not a string");
    }

    /* Khung vực định nghĩa format của Linh */
    // 1. format số thực
    std::string format_float(double value)
    {
        // Bước 1: Chuyển thành string với độ chính xác cao bằng fmt
        std::string str = fmt::format("{:.17f}", value);

        // Bước 2: Tìm vị trí dấu chấm thập phân
        size_t dot_pos = str.find('.');
        if (dot_pos == std::string::npos)
        {
            return str; // Không có phần thập phân
        }

        // Bước 3: Loại bỏ số 0 cuối từ phần thập phân, nhưng giữ lại ít nhất một số 0
        size_t end_pos = str.length() - 1;
        while (end_pos > dot_pos + 1 && str[end_pos] == '0')
        {
            end_pos--;
        }

        // Bước 4: Nếu chỉ còn một số 0 sau dấu chấm, giữ lại
        if (end_pos == dot_pos + 1 && str[end_pos] == '0')
        {
            return str.substr(0, end_pos + 1); // Giữ lại "x.0"
        }

        // Bước 5: Giới hạn tối đa 15 chữ số có nghĩa (bao gồm cả phần nguyên)
        std::string result = str.substr(0, end_pos + 1);

        // Đếm số chữ số có nghĩa
        int significant_digits = 0;
        bool found_non_zero = false;

        for (char c : result)
        {
            if (c == '.')
                continue;
            if (c != '0')
                found_non_zero = true;
            if (found_non_zero)
                significant_digits++;
        }

        // Nếu có quá 15 chữ số có nghĩa, cắt bớt
        if (significant_digits > 15)
        {
            // Tìm vị trí để cắt
            int digits_to_keep = 15;
            size_t cut_pos = 0;
            int current_digits = 0;

            for (size_t i = 0; i < result.length(); i++)
            {
                if (result[i] == '.')
                    continue;
                if (result[i] != '0')
                {
                    current_digits++;
                    if (current_digits > digits_to_keep)
                    {
                        cut_pos = i;
                        break;
                    }
                }
                else if (current_digits > 0)
                {
                    current_digits++;
                    if (current_digits > digits_to_keep)
                    {
                        cut_pos = i;
                        break;
                    }
                }
            }

            if (cut_pos > 0)
            {
                result = result.substr(0, cut_pos);
                // Loại bỏ số 0 cuối sau khi cắt, nhưng giữ lại ít nhất một số 0
                while (result.back() == '0' && result.length() > dot_pos + 2)
                {
                    result.pop_back();
                }
            }
        }

        return result;
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
            else if (std::holds_alternative<double>(val))
            {
                result += format_float(std::get<double>(val));
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
            else if (std::holds_alternative<double>(value))
            {
                value_str = format_float(std::get<double>(value));
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
        if (std::holds_alternative<int64_t>(val))
            return "int";
        if (std::holds_alternative<uint64_t>(val))
            return "uint";
        if (std::holds_alternative<double>(val))
            return "float";
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
        if (std::holds_alternative<ByteArray>(val))
            return "bytearray";
        return "unknown";
    }

    std::string to_str(const Value &val)
    {
        if (std::holds_alternative<std::monostate>(val))
            return std::string("sol");
        if (std::holds_alternative<int64_t>(val))
            return std::to_string(std::get<int64_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return std::to_string(std::get<uint64_t>(val));
        if (std::holds_alternative<double>(val))
            return format_float(std::get<double>(val));
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
        if (std::holds_alternative<ByteArray>(val))
        {
            const auto &barr = std::get<ByteArray>(val);
            std::string result = "[";
            for (size_t i = 0; i < barr->size(); ++i)
            {
                if (i > 0)
                    result += ", ";
                char buf[5];
                std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>((*barr)[i]));
                result += buf;
            }
            result += "]";
            return result;
        }
        return "<unknown>";
    }

    int64_t to_int(const Value &val)
    {
        if (std::holds_alternative<int64_t>(val))
            return std::get<int64_t>(val);
        if (std::holds_alternative<uint64_t>(val))
            return static_cast<int64_t>(std::get<uint64_t>(val));
        if (std::holds_alternative<double>(val))
            return static_cast<int64_t>(std::get<double>(val));
        if (std::holds_alternative<std::string>(val))
        {
            try
            {
                return std::stoll(std::get<std::string>(val));
            }
            catch (...)
            {
                return 0;
            }
        }
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1 : 0;
        if (std::holds_alternative<Byte>(val))
            return static_cast<int64_t>(std::get<Byte>(val));
        return 0;
    }

    double to_float(const Value &val)
    {
        if (std::holds_alternative<double>(val))
            return std::get<double>(val);
        if (std::holds_alternative<int64_t>(val))
            return static_cast<double>(std::get<int64_t>(val));
        if (std::holds_alternative<uint64_t>(val))
            return static_cast<double>(std::get<uint64_t>(val));
        if (std::holds_alternative<std::string>(val))
        {
            try
            {
                return std::stod(std::get<std::string>(val));
            }
            catch (...)
            {
                return 0.0;
            }
        }
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1.0 : 0.0;
        if (std::holds_alternative<Byte>(val))
            return static_cast<double>(std::get<Byte>(val));
        return 0.0;
    }

    uint64_t to_uint(const Value &val)
    {
        if (std::holds_alternative<uint64_t>(val))
            return std::get<uint64_t>(val);
        if (std::holds_alternative<int64_t>(val))
            return static_cast<uint64_t>(std::max<int64_t>(0, std::get<int64_t>(val)));
        if (std::holds_alternative<double>(val))
            return static_cast<uint64_t>(std::max<double>(0.0, std::get<double>(val)));
        if (std::holds_alternative<std::string>(val))
        {
            try
            {
                auto v = std::stoull(std::get<std::string>(val));
                return v;
            }
            catch (...)
            {
                return 0;
            }
        }
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val) ? 1 : 0;
        if (std::holds_alternative<Byte>(val))
            return static_cast<uint64_t>(std::get<Byte>(val));
        return 0;
    }

    bool to_bool(const Value &val)
    {
        if (std::holds_alternative<bool>(val))
            return std::get<bool>(val);
        if (std::holds_alternative<int64_t>(val))
            return std::get<int64_t>(val) != 0;
        if (std::holds_alternative<double>(val))
            return std::get<double>(val) != 0.0;
        if (std::holds_alternative<std::string>(val))
            return !std::get<std::string>(val).empty();
        if (std::holds_alternative<Byte>(val))
            return std::get<Byte>(val) != 0;
        if (std::holds_alternative<ByteArray>(val))
            return !std::get<ByteArray>(val)->empty();
        return false;
    }

    int64_t len(const Value &val)
    {
        if (std::holds_alternative<Array>(val))
        {
            const auto &arr = std::get<Array>(val);
            return arr ? static_cast<int64_t>(arr->size()) : 0;
        }
        if (std::holds_alternative<Map>(val))
        {
            const auto &map = std::get<Map>(val);
            return map ? static_cast<int64_t>(map->size()) : 0;
        }
        if (std::holds_alternative<std::string>(val))
        {
            return static_cast<int64_t>(std::get<std::string>(val).size());
        }
        if (std::holds_alternative<ByteArray>(val))
        {
            const auto &barr = std::get<ByteArray>(val);
            return barr ? static_cast<int64_t>(barr->size()) : 0;
        }
        return 0;
    }
}
