#pragma once
#include <variant>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <memory> // Thêm dòng này cho shared_ptr
#include <mutex>
#include <unordered_set>
#include <stack>

// Forward declaration
namespace Linh {
    struct FunctionObject;
    using FunctionPtr = std::shared_ptr<FunctionObject>;
}

namespace Linh
{
    struct Value; // Forward declaration
    using Array = std::shared_ptr<std::vector<Value>>;
    using Map = std::shared_ptr<std::unordered_map<std::string, Value>>;
    using Byte = uint8_t;

    using VariantType = std::variant<
        std::monostate,
        bool,
        int8_t,
        int16_t,
        int32_t,
        int64_t,
        uint8_t,
        uint16_t,
        uint32_t,
        uint64_t,
        float,      // float32
        double,     // float64
        std::string,
        Array,
        Map,
        std::shared_ptr<FunctionObject>
    >;

    // String interning singleton
    class StringInterner {
    public:
        static StringInterner& instance() {
            static StringInterner inst;
            return inst;
        }
        // Trả về reference tới string đã intern
        const std::string& intern(const std::string& str) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto [it, inserted] = pool_.insert(str);
            return *it;
        }
    private:
        std::unordered_set<std::string> pool_;
        std::mutex mutex_;
        StringInterner() = default;
        StringInterner(const StringInterner&) = delete;
        StringInterner& operator=(const StringInterner&) = delete;
    };

    // ObjectPool template cho Array/Map
    template <typename T>
    class ObjectPool {
    public:
        static ObjectPool& instance() {
            static ObjectPool inst;
            return inst;
        }
        std::shared_ptr<T> acquire() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!pool_.empty()) {
                auto obj = pool_.top();
                pool_.pop();
                return obj;
            }
            return std::make_shared<T>();
        }
        void release(std::shared_ptr<T> obj) {
            std::lock_guard<std::mutex> lock(mutex_);
            obj->clear();
            pool_.push(obj);
        }
    private:
        std::stack<std::shared_ptr<T>> pool_;
        std::mutex mutex_;
        ObjectPool() = default;
        ObjectPool(const ObjectPool&) = delete;
        ObjectPool& operator=(const ObjectPool&) = delete;
    };

    // Factory cho Array/Map/ByteArray
    inline Array make_array() {
        // Simpler and safer lifetime management: standard shared_ptr
        return std::make_shared<std::vector<Value>>();
    }
    inline Map make_map() {
        return std::make_shared<std::unordered_map<std::string, Value>>();
    }

    // Helper functions để tương tác với StringInterning
    inline std::string intern_string(const std::string& s) {
        return StringInterner::instance().intern(s);
    }

    struct Value : public VariantType {
        using VariantType::VariantType;
        Value() : VariantType() {}
        Value(const VariantType &v) : VariantType(v) {}
        Value(const std::string& s) : VariantType(std::in_place_type<std::string>, StringInterner::instance().intern(s)) {}
    // Tạo Value từ array/map mới (dùng pool)
    static Value new_array() { return Value(make_array()); }
    static Value new_map() { return Value(make_map()); }
    };
}
