#include "Value.hpp"
#include "LiVM/Functional/Func.hpp"
#include <stdexcept>
#include <cstring>
#include <utility>

namespace Linh
{
    // ============================================================================
    // Value Constructors and Destructor
    // ============================================================================
    
    Value::Value() noexcept : type(ValueType::Sol) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_int64 = 0;
    }
    
    Value::~Value() {
        cleanup();
    }
    
    Value::Value(const Value& other) : type(ValueType::Sol) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_int64 = 0;
        copy_from(other);
    }
    
    Value::Value(Value&& other) noexcept : type(ValueType::Sol) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_int64 = 0;
        move_from(std::move(other));
    }
    
    Value& Value::operator=(const Value& other) {
        if (this != &other) {
            cleanup();
            copy_from(other);
        }
        return *this;
    }
    
    Value& Value::operator=(Value&& other) noexcept {
        if (this != &other) {
            cleanup();
            move_from(std::move(other));
        }
        return *this;
    }
    
    // ============================================================================
    // Type-specific Constructors
    // ============================================================================
    
    Value::Value(bool v) noexcept : type(ValueType::Bool) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_bool = v;
    }
    
    Value::Value(int8_t v) noexcept : type(ValueType::Int8) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_int8 = v;
    }
    
    Value::Value(int16_t v) noexcept : type(ValueType::Int16) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_int16 = v;
    }
    
    Value::Value(int32_t v) noexcept : type(ValueType::Int32) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_int32 = v;
    }
    
    Value::Value(int64_t v) noexcept : type(ValueType::Int64) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_int64 = v;
    }
    
    Value::Value(uint8_t v) noexcept : type(ValueType::UInt8) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_uint8 = v;
    }
    
    Value::Value(uint16_t v) noexcept : type(ValueType::UInt16) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_uint16 = v;
    }
    
    Value::Value(uint32_t v) noexcept : type(ValueType::UInt32) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_uint32 = v;
    }
    
    Value::Value(uint64_t v) noexcept : type(ValueType::UInt64) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_uint64 = v;
    }
    
    Value::Value(float v) noexcept : type(ValueType::Float32) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_float32 = v;
    }
    
    Value::Value(double v) noexcept : type(ValueType::Float64) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_float64 = v;
    }
    
    Value::Value(const char* str) : type(ValueType::String) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_string = new std::string(str);
    }
    
    Value::Value(const std::string& str) : type(ValueType::String) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_string = new std::string(str);
    }
    
    Value::Value(std::string&& str) : type(ValueType::String) {
        std::memset(_padding, 0, sizeof(_padding));
        data.as_string = new std::string(std::move(str));
    }
    
    Value::Value(const std::shared_ptr<FunctionObject>& fn) : type(ValueType::Function) {
        data.as_function = fn.get();
    }
    
    // ============================================================================
    // Factory Methods
    // ============================================================================
    
    Value Value::make_array() {
        Value v;
        v.type = ValueType::Array;
        v.data.as_array = new std::vector<Value>();
        return v;
    }
    
    Value Value::make_map() {
        Value v;
        v.type = ValueType::Map;
        v.data.as_map = new std::unordered_map<std::string, Value>();
        return v;
    }
    
    Value Value::make_function(FunctionObject* fn) {
        Value v;
        v.type = ValueType::Function;
        v.data.as_function = fn;
        return v;
    }
    
    // ============================================================================
    // Safe Value Access Methods
    // ============================================================================
    
    bool Value::as_bool_safe() const {
        if (type != ValueType::Bool) {
            throw std::runtime_error("Value is not a bool");
        }
        return data.as_bool;
    }
    
    int64_t Value::as_int64_safe() const {
        switch (type) {
            case ValueType::Int8: return static_cast<int64_t>(data.as_int8);
            case ValueType::Int16: return static_cast<int64_t>(data.as_int16);
            case ValueType::Int32: return static_cast<int64_t>(data.as_int32);
            case ValueType::Int64: return data.as_int64;
            case ValueType::UInt8: return static_cast<int64_t>(data.as_uint8);
            case ValueType::UInt16: return static_cast<int64_t>(data.as_uint16);
            case ValueType::UInt32: return static_cast<int64_t>(data.as_uint32);
            case ValueType::UInt64: return static_cast<int64_t>(data.as_uint64);
            default:
                throw std::runtime_error("Value is not an integer type");
        }
    }
    
    uint64_t Value::as_uint64_safe() const {
        switch (type) {
            case ValueType::UInt8: return static_cast<uint64_t>(data.as_uint8);
            case ValueType::UInt16: return static_cast<uint64_t>(data.as_uint16);
            case ValueType::UInt32: return static_cast<uint64_t>(data.as_uint32);
            case ValueType::UInt64: return data.as_uint64;
            case ValueType::Int8: return static_cast<uint64_t>(data.as_int8);
            case ValueType::Int16: return static_cast<uint64_t>(data.as_int16);
            case ValueType::Int32: return static_cast<uint64_t>(data.as_int32);
            case ValueType::Int64: return static_cast<uint64_t>(data.as_int64);
            default:
                throw std::runtime_error("Value is not an integer type");
        }
    }
    
    double Value::as_float64_safe() const {
        switch (type) {
            case ValueType::Float32: return static_cast<double>(data.as_float32);
            case ValueType::Float64: return data.as_float64;
            default:
                throw std::runtime_error("Value is not a float type");
        }
    }
    
    const std::string& Value::as_string_ref() const {
        if (type != ValueType::String) {
            throw std::runtime_error("Value is not a string");
        }
        return *static_cast<std::string*>(data.as_string);
    }
    
    std::vector<Value>& Value::as_array_ref() {
        if (type != ValueType::Array) {
            throw std::runtime_error("Value is not an array");
        }
        return *static_cast<std::vector<Value>*>(data.as_array);
    }
    
    const std::vector<Value>& Value::as_array_ref() const {
        if (type != ValueType::Array) {
            throw std::runtime_error("Value is not an array");
        }
        return *static_cast<std::vector<Value>*>(data.as_array);
    }
    
    std::unordered_map<std::string, Value>& Value::as_map_ref() {
        if (type != ValueType::Map) {
            throw std::runtime_error("Value is not a map");
        }
        return *static_cast<std::unordered_map<std::string, Value>*>(data.as_map);
    }
    
    const std::unordered_map<std::string, Value>& Value::as_map_ref() const {
        if (type != ValueType::Map) {
            throw std::runtime_error("Value is not a map");
        }
        return *static_cast<std::unordered_map<std::string, Value>*>(data.as_map);
    }
    
    FunctionObject& Value::as_function_ref() {
        if (type != ValueType::Function) {
            throw std::runtime_error("Value is not a function");
        }
        return *static_cast<FunctionObject*>(data.as_function);
    }
    
    const FunctionObject& Value::as_function_ref() const {
        if (type != ValueType::Function) {
            throw std::runtime_error("Value is not a function");
        }
        return *static_cast<FunctionObject*>(data.as_function);
    }
    
    // ============================================================================
    // Private Helper Methods
    // ============================================================================
    
    void Value::cleanup() {
        switch (type) {
            case ValueType::String:
                delete static_cast<std::string*>(data.as_string);
                break;
            case ValueType::Array:
                delete static_cast<std::vector<Value>*>(data.as_array);
                break;
            case ValueType::Map:
                delete static_cast<std::unordered_map<std::string, Value>*>(data.as_map);
                break;
            case ValueType::Function:
                // Functions are managed externally, don't delete
                break;
            default:
                // Primitive types don't need cleanup
                break;
        }
        type = ValueType::Sol;
        data.as_int64 = 0;
    }
    
    void Value::copy_from(const Value& other) {
        type = other.type;
        std::memcpy(_padding, other._padding, sizeof(_padding));
        
        switch (type) {
            case ValueType::String:
                data.as_string = new std::string(*static_cast<std::string*>(other.data.as_string));
                break;
            case ValueType::Array:
                data.as_array = new std::vector<Value>(*static_cast<std::vector<Value>*>(other.data.as_array));
                break;
            case ValueType::Map:
                data.as_map = new std::unordered_map<std::string, Value>(
                    *static_cast<std::unordered_map<std::string, Value>*>(other.data.as_map)
                );
                break;
            default:
                // For primitives and function pointers, just copy the data
                data = other.data;
                break;
        }
    }
    
    void Value::move_from(Value&& other) noexcept {
        type = other.type;
        std::memcpy(_padding, other._padding, sizeof(_padding));
        data = other.data;
        
        // Reset other to Sol state
        other.type = ValueType::Sol;
        other.data.as_int64 = 0;
    }
}
