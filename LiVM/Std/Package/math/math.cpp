#include "math.hpp"
#include <cmath>
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <variant>
#include "LiVM/Variable/Value.hpp"

namespace Linh {
namespace Std {
    static std::unordered_map<std::string, MathFunction> math_functions;
    static std::unordered_map<std::string, double> math_constants = {
        {"pi", 3.141592653589793},
        {"e", 2.718281828459045},
        {"tau", 6.283185307179586},
        {"phi", 1.618033988749895}
    };

    // Các hàm toán học (math_abs, math_ceil, ...)
    static Value math_abs(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::abs(Linh::get<int64_t>(val));
        else if (Linh::holds_alternative<double>(val))
            return std::abs(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return val;
        else
            return Value{};
    }
    static Value math_ceil(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return val;
        else if (Linh::holds_alternative<double>(val))
            return std::ceil(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return val;
        else
            return Value{};
    }
    static Value math_floor(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return val;
        else if (Linh::holds_alternative<double>(val))
            return std::floor(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return val;
        else
            return Value{};
    }
    static Value math_round(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return val;
        else if (Linh::holds_alternative<double>(val))
            return std::round(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return val;
        else
            return Value{};
    }
    static Value math_trunc(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return val;
        else if (Linh::holds_alternative<double>(val))
            return std::trunc(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return val;
        else
            return Value{};
    }
    static Value math_sin(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::sin(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::sin(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::sin(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_cos(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::cos(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::cos(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::cos(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_tan(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::tan(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::tan(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::tan(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_asin(const Value& val) {
        double v = 0;
        if (Linh::holds_alternative<int64_t>(val))
            v = static_cast<double>(Linh::get<int64_t>(val));
        else if (Linh::holds_alternative<double>(val))
            v = Linh::get<double>(val);
        else if (Linh::holds_alternative<uint64_t>(val))
            v = static_cast<double>(Linh::get<uint64_t>(val));
        else
            return Value{};
        if (v < -1.0 || v > 1.0) return Value{};
        return std::asin(v);
    }
    static Value math_acos(const Value& val) {
        double v = 0;
        if (Linh::holds_alternative<int64_t>(val))
            v = static_cast<double>(Linh::get<int64_t>(val));
        else if (Linh::holds_alternative<double>(val))
            v = Linh::get<double>(val);
        else if (Linh::holds_alternative<uint64_t>(val))
            v = static_cast<double>(Linh::get<uint64_t>(val));
        else
            return Value{};
        if (v < -1.0 || v > 1.0) return Value{};
        return std::acos(v);
    }
    static Value math_atan(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::atan(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::atan(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::atan(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_radians(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return Linh::get<int64_t>(val) * math_constants["pi"] / 180.0;
        else if (Linh::holds_alternative<double>(val))
            return Linh::get<double>(val) * math_constants["pi"] / 180.0;
        else if (Linh::holds_alternative<uint64_t>(val))
            return Linh::get<uint64_t>(val) * math_constants["pi"] / 180.0;
        else
            return Value{};
    }
    static Value math_sinh(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::sinh(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::sinh(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::sinh(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_cosh(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::cosh(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::cosh(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::cosh(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_tanh(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::tanh(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::tanh(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::tanh(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_asinh(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::asinh(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::asinh(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::asinh(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_acosh(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::acosh(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::acosh(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::acosh(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_atanh(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::atanh(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::atanh(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::atanh(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_sqrt(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val)) {
            int64_t v = Linh::get<int64_t>(val);
            if (v < 0) return Value{};
            return std::sqrt(static_cast<double>(v));
        } else if (Linh::holds_alternative<double>(val)) {
            double v = Linh::get<double>(val);
            if (v < 0) return Value{};
            return std::sqrt(v);
        } else if (Linh::holds_alternative<uint64_t>(val)) {
            return std::sqrt(static_cast<double>(Linh::get<uint64_t>(val)));
        } else {
            return Value{};
        }
    }
    static Value math_cbrt(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::cbrt(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::cbrt(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::cbrt(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_exp(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::exp(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::exp(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::exp(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_expm1(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val))
            return std::expm1(static_cast<double>(Linh::get<int64_t>(val)));
        else if (Linh::holds_alternative<double>(val))
            return std::expm1(Linh::get<double>(val));
        else if (Linh::holds_alternative<uint64_t>(val))
            return std::expm1(static_cast<double>(Linh::get<uint64_t>(val)));
        else
            return Value{};
    }
    static Value math_log(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val)) {
            int64_t v = Linh::get<int64_t>(val);
            if (v <= 0) return Value{};
            return std::log(static_cast<double>(v));
        } else if (Linh::holds_alternative<double>(val)) {
            double v = Linh::get<double>(val);
            if (v <= 0) return Value{};
            return std::log(v);
        } else if (Linh::holds_alternative<uint64_t>(val)) {
            uint64_t v = Linh::get<uint64_t>(val);
            if (v == 0) return Value{};
            return std::log(static_cast<double>(v));
        } else {
            return Value{};
        }
    }
    static Value math_log1p(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val)) {
            int64_t v = Linh::get<int64_t>(val);
            if (v <= -1) return Value{};
            return std::log1p(static_cast<double>(v));
        } else if (Linh::holds_alternative<double>(val)) {
            double v = Linh::get<double>(val);
            if (v <= -1) return Value{};
            return std::log1p(v);
        } else if (Linh::holds_alternative<uint64_t>(val)) {
            return std::log1p(static_cast<double>(Linh::get<uint64_t>(val)));
        } else {
            return Value{};
        }
    }
    static Value math_log10(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val)) {
            int64_t v = Linh::get<int64_t>(val);
            if (v <= 0) return Value{};
            return std::log10(static_cast<double>(v));
        } else if (Linh::holds_alternative<double>(val)) {
            double v = Linh::get<double>(val);
            if (v <= 0) return Value{};
            return std::log10(v);
        } else if (Linh::holds_alternative<uint64_t>(val)) {
            uint64_t v = Linh::get<uint64_t>(val);
            if (v == 0) return Value{};
            return std::log10(static_cast<double>(v));
        } else {
            return Value{};
        }
    }
    static Value math_log2(const Value& val) {
        if (Linh::holds_alternative<int64_t>(val)) {
            int64_t v = Linh::get<int64_t>(val);
            if (v <= 0) return Value{};
            return std::log2(static_cast<double>(v));
        } else if (Linh::holds_alternative<double>(val)) {
            double v = Linh::get<double>(val);
            if (v <= 0) return Value{};
            return std::log2(v);
        } else if (Linh::holds_alternative<uint64_t>(val)) {
            uint64_t v = Linh::get<uint64_t>(val);
            if (v == 0) return Value{};
            return std::log2(static_cast<double>(v));
        } else {
            return Value{};
        }
    }
    void initialize_math_functions() {
        if (!math_functions.empty()) return;
        math_functions["abs"] = math_abs;
        math_functions["ceil"] = math_ceil;
        math_functions["floor"] = math_floor;
        math_functions["round"] = math_round;
        math_functions["trunc"] = math_trunc;
        math_functions["sin"] = math_sin;
        math_functions["cos"] = math_cos;
        math_functions["tan"] = math_tan;
        math_functions["asin"] = math_asin;
        math_functions["acos"] = math_acos;
        math_functions["atan"] = math_atan;
        math_functions["radians"] = math_radians;
        math_functions["sinh"] = math_sinh;
        math_functions["cosh"] = math_cosh;
        math_functions["tanh"] = math_tanh;
        math_functions["asinh"] = math_asinh;
        math_functions["acosh"] = math_acosh;
        math_functions["atanh"] = math_atanh;
        math_functions["sqrt"] = math_sqrt;
        math_functions["cbrt"] = math_cbrt;
        math_functions["exp"] = math_exp;
        math_functions["expm1"] = math_expm1;
        math_functions["log"] = math_log;
        math_functions["log1p"] = math_log1p;
        math_functions["log10"] = math_log10;
        math_functions["log2"] = math_log2;
    }
    MathFunction get_math_function(const std::string& function_name) {
        if (math_functions.empty()) initialize_math_functions();
        auto it = math_functions.find(function_name);
        if (it != math_functions.end()) return it->second;
        return nullptr;
    }
    std::vector<std::string> get_math_functions() {
        if (math_functions.empty()) initialize_math_functions();
        std::vector<std::string> names;
        for (const auto& pair : math_functions) names.push_back(pair.first);
        return names;
    }
    double get_math_constant(const std::string& constant_name) {
        auto it = math_constants.find(constant_name);
        if (it != math_constants.end()) return it->second;
        return 0.0;
    }
    std::vector<std::string> get_math_constants() {
        std::vector<std::string> names;
        for (const auto& pair : math_constants) names.push_back(pair.first);
        return names;
    }
}
}
