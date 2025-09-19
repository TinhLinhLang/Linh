#pragma once
#include "Value/Value.hpp"
#include "LiVM.hpp"
#include <string>
#include <cstdint>

namespace Linh
{
    using Value = Linh::Value;
    using Array = Linh::Array;
    using Map = Linh::Map;
    using Byte = Linh::Byte;

    std::string type_of(const Value &val);

    // Hàm format số thực theo quy tắc của Linh
    std::string format_float(float value);


    // Hàm chuyển đổi kiểu dữ liệu
    std::string to_str(const Value &val);
    int8_t to_int8(const Value &val);
    int16_t to_int16(const Value &val);
    int32_t to_int32(const Value &val);
    int64_t to_int64(const Value &val);
    uint8_t to_uint8(const Value &val);
    uint16_t to_uint16(const Value &val);
    uint32_t to_uint32(const Value &val);
    uint64_t to_uint64(const Value &val);
    float to_float32(const Value &val);
    double to_float64(const Value &val);
    bool to_bool(const Value &val);
    
    // Type aliases for backward compatibility
    inline int32_t to_int(const Value &val) { return to_int32(val); }
    inline float to_float(const Value &val) { return to_float32(val); }
    inline uint32_t to_uint(const Value &val) { return to_uint32(val); }

    // Hàm chuyển string thành array<byte> (encoding: "utf-8" mặc định)
    Array string_bytes(const std::string& s, const std::string& encoding = "utf-8");
    Array string_bytes(const Value& v, const std::string& encoding = "utf-8");

    // Định nghĩa hàm type() cho LiVM
    void type(LiVM &vm);

    // Hàm lấy độ dài cho array, map, string
    int32_t len(const Value &val);
}
