#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <stack>
#include <new>
#include <iostream>

// Forward declarations
namespace Linh {
    struct FunctionObject;
    struct Value;
    
    // Opaque handle types for ABI stability
    using ArrayHandle = void*;
    using MapHandle = void*;
    using FunctionHandle = void*;
    using StringHandle = void*;
}

namespace Linh
{
    // Type enumeration for ABI-stable type identification
    // This enum has a fixed size and stable values for cross-compilation
    enum class ValueType : uint8_t {
        Sol = 0,      // null/void type
        Bool = 1,
        Int8 = 2,
        Int16 = 3,
        Int32 = 4,
        Int64 = 5,
        UInt8 = 6,
        UInt16 = 7,
        UInt32 = 8,
        UInt64 = 9,
        Float32 = 10,
        Float64 = 11,
        String = 12,
        Array = 13,
        Map = 14,
        Function = 15
    };
    
    // Comparison operators for ValueType
    inline bool operator==(ValueType a, ValueType b) noexcept {
        return static_cast<uint8_t>(a) == static_cast<uint8_t>(b);
    }
    
    inline bool operator!=(ValueType a, ValueType b) noexcept {
        return static_cast<uint8_t>(a) != static_cast<uint8_t>(b);
    }
    
    // Stream output operator for ValueType
    inline std::ostream& operator<<(std::ostream& os, ValueType type) {
        return os << static_cast<int>(type);
    }

    // ABI-stable Value structure
    // Uses type-tagged union with fixed layout for cross-language compatibility
    // Total size: 16 bytes (8-byte aligned) for optimal performance
    struct Value {
        // Type tag (1 byte) + 7 bytes padding for alignment
        ValueType type;
        uint8_t _padding[7];
        
        // Data union (8 bytes) - holds primitive values or opaque pointers
        union {
            bool as_bool;
            int8_t as_int8;
            int16_t as_int16;
            int32_t as_int32;
            int64_t as_int64;
            uint8_t as_uint8;
            uint16_t as_uint16;
            uint32_t as_uint32;
            uint64_t as_uint64;
            float as_float32;
            double as_float64;
            StringHandle as_string;      // Opaque pointer to std::string*
            ArrayHandle as_array;        // Opaque pointer to std::vector<Value>*
            MapHandle as_map;            // Opaque pointer to std::unordered_map<std::string, Value>*
            FunctionHandle as_function;  // Opaque pointer to FunctionObject*
        } data;
        
        // Constructors
        Value() noexcept;
        ~Value();
        Value(const Value& other);
        Value(Value&& other) noexcept;
        Value& operator=(const Value& other);
        Value& operator=(Value&& other) noexcept;
        
        // Type-specific constructors
        Value(bool v) noexcept;
        Value(int8_t v) noexcept;
        Value(int16_t v) noexcept;
        Value(int32_t v) noexcept;
        Value(int64_t v) noexcept;
        Value(uint8_t v) noexcept;
        Value(uint16_t v) noexcept;
        Value(uint32_t v) noexcept;
        Value(uint64_t v) noexcept;
        Value(float v) noexcept;
        Value(double v) noexcept;
        Value(const char* str);
        Value(const std::string& str);
        Value(std::string&& str);
        Value(const std::shared_ptr<FunctionObject>& fn);
        
        // Type checking
        ValueType get_type() const noexcept { return type; }
        bool is_sol() const noexcept { return type == ValueType::Sol; }
        bool is_bool() const noexcept { return type == ValueType::Bool; }
        bool is_int() const noexcept { return type >= ValueType::Int8 && type <= ValueType::Int64; }
        bool is_uint() const noexcept { return type >= ValueType::UInt8 && type <= ValueType::UInt64; }
        bool is_float() const noexcept { return type == ValueType::Float32 || type == ValueType::Float64; }
        bool is_number() const noexcept { return is_int() || is_uint() || is_float(); }
        bool is_string() const noexcept { return type == ValueType::String; }
        bool is_array() const noexcept { return type == ValueType::Array; }
        bool is_map() const noexcept { return type == ValueType::Map; }
        bool is_function() const noexcept { return type == ValueType::Function; }
        
        // Value access (with type checking)
        bool as_bool_safe() const;
        int64_t as_int64_safe() const;
        uint64_t as_uint64_safe() const;
        double as_float64_safe() const;
        const std::string& as_string_ref() const;
        std::vector<Value>& as_array_ref();
        const std::vector<Value>& as_array_ref() const;
        std::unordered_map<std::string, Value>& as_map_ref();
        const std::unordered_map<std::string, Value>& as_map_ref() const;
        FunctionObject& as_function_ref();
        const FunctionObject& as_function_ref() const;
        
        // Factory methods for complex types
        static Value make_array();
        static Value make_map();
        static Value make_function(FunctionObject* fn);
        
    private:
        void cleanup();
        void copy_from(const Value& other);
        void move_from(Value&& other) noexcept;
    };
    
    // Type aliases for backward compatibility
    using Array = std::vector<Value>*;
    using Map = std::unordered_map<std::string, Value>*;
    using Byte = uint8_t;
    
    // String interning for memory optimization (optional, can be disabled)
    class StringInterner {
    public:
        static StringInterner& instance() {
            static StringInterner inst;
            return inst;
        }
        const std::string* intern(const std::string& str) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto [it, inserted] = pool_.insert(str);
            return &(*it);
        }
    private:
        std::unordered_set<std::string> pool_;
        std::mutex mutex_;
        StringInterner() = default;
        StringInterner(const StringInterner&) = delete;
        StringInterner& operator=(const StringInterner&) = delete;
    };
    
    // ============================================================================
    // Backward Compatibility Helpers
    // ============================================================================
    
    // Forward declaration for FunctionPtr
    using FunctionPtr = std::shared_ptr<FunctionObject>;
    
    // Helper template for std::holds_alternative replacement
    template<typename T>
    inline bool holds_alternative(const Value& v) {
        if constexpr (std::is_same_v<T, std::monostate>) {
            return v.get_type() == ValueType::Sol;
        } else if constexpr (std::is_same_v<T, bool>) {
            return v.get_type() == ValueType::Bool;
        } else if constexpr (std::is_same_v<T, int8_t>) {
            return v.get_type() == ValueType::Int8;
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return v.get_type() == ValueType::Int16;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return v.get_type() == ValueType::Int32;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return v.get_type() == ValueType::Int64;
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            return v.get_type() == ValueType::UInt8;
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            return v.get_type() == ValueType::UInt16;
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return v.get_type() == ValueType::UInt32;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return v.get_type() == ValueType::UInt64;
        } else if constexpr (std::is_same_v<T, float>) {
            return v.get_type() == ValueType::Float32;
        } else if constexpr (std::is_same_v<T, double>) {
            return v.get_type() == ValueType::Float64;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return v.get_type() == ValueType::String;
        } else if constexpr (std::is_same_v<T, FunctionPtr> || std::is_same_v<T, std::shared_ptr<FunctionObject>>) {
            return v.get_type() == ValueType::Function;
        } else {
            return false;
        }
    }
    
    // Helper template for std::get replacement
    template<typename T>
    inline T get(const Value& v) {
        if constexpr (std::is_same_v<T, bool>) {
            return v.data.as_bool;
        } else if constexpr (std::is_same_v<T, int8_t>) {
            return v.data.as_int8;
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return v.data.as_int16;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return v.data.as_int32;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return v.data.as_int64;
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            return v.data.as_uint8;
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            return v.data.as_uint16;
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return v.data.as_uint32;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return v.data.as_uint64;
        } else if constexpr (std::is_same_v<T, float>) {
            return v.data.as_float32;
        } else if constexpr (std::is_same_v<T, double>) {
            return v.data.as_float64;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return v.as_string_ref();
        } else if constexpr (std::is_same_v<T, Array>) {
            return static_cast<Array>(v.data.as_array);
        } else if constexpr (std::is_same_v<T, Map>) {
            return static_cast<Map>(v.data.as_map);
        } else if constexpr (std::is_same_v<T, FunctionPtr> || std::is_same_v<T, std::shared_ptr<FunctionObject>>) {
            // Return a shared_ptr wrapping the raw pointer (non-owning)
            return std::shared_ptr<FunctionObject>(static_cast<FunctionObject*>(v.data.as_function), [](FunctionObject*){});
        } else {
            throw std::runtime_error("Invalid type for get");
        }
    }
    
    // Reference versions for Array and Map
    template<>
    inline std::vector<Value>& get<std::vector<Value>&>(const Value& v) {
        return const_cast<Value&>(v).as_array_ref();
    }
    
    template<>
    inline std::unordered_map<std::string, Value>& get<std::unordered_map<std::string, Value>&>(const Value& v) {
        return const_cast<Value&>(v).as_map_ref();
    }
    
    // Factory functions for backward compatibility
    inline Value make_array() {
        return Value::make_array();
    }
    
    inline Value make_map() {
        return Value::make_map();
    }
}
