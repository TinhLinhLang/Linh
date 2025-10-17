#pragma once
#include "LiVM/Variable/Value.hpp"
#include <string>
#include <cstdint>

// Forward declaration
namespace Linh {
    class LiVM;
}

namespace Linh
{
    using Byte = uint8_t;

    // ============================================================================
    // Type Information Functions
    // ============================================================================
    
    // Get type name as string
    std::string type_of(const Value &val);
    
    // Compare two Values for equality
    bool values_equal(const Value &a, const Value &b);
    
    // ============================================================================
    // Formatting Functions
    // ============================================================================
    
    // Format float with Linh's rules (similar to Go/Java)
    std::string format_float(float value);
    std::string format_double(double value);
    
    // Convert Value to string representation
    std::string to_str(const Value &val);
    
    // ============================================================================
    // Type Conversion Functions (with coercion)
    // ============================================================================
    
    // Convert to boolean
    bool to_bool(const Value &val);
    
    // Convert to signed integers
    int8_t to_int8(const Value &val);
    int16_t to_int16(const Value &val);
    int32_t to_int32(const Value &val);
    int64_t to_int64(const Value &val);
    
    // Convert to unsigned integers
    uint8_t to_uint8(const Value &val);
    uint16_t to_uint16(const Value &val);
    uint32_t to_uint32(const Value &val);
    uint64_t to_uint64(const Value &val);
    
    // Convert to floating point
    float to_float32(const Value &val);
    double to_float64(const Value &val);
    
    // Type aliases for backward compatibility
    inline int32_t to_int(const Value &val) { return to_int32(val); }
    inline float to_float(const Value &val) { return to_float32(val); }
    inline uint32_t to_uint(const Value &val) { return to_uint32(val); }
    
    // ============================================================================
    // Utility Functions
    // ============================================================================
    
    // Get length of array, map, or string
    int32_t len(const Value &val);
    
    // Convert string to array of bytes (UTF-8 encoding)
    Value string_bytes(const std::string& s, const std::string& encoding = "utf-8");
    Value string_bytes(const Value& v, const std::string& encoding = "utf-8");
    
    // ============================================================================
    // VM Integration
    // ============================================================================
    
    // Type function for LiVM
    void type(LiVM &vm);
}
