#pragma once
#include <string>
#include <functional>
#include <vector>
#include "../../Value/Value.hpp"

namespace Linh {
namespace Std {
    using MathFunction = std::function<Value(const Value&)>;
    // Khai báo các hàm toán học
    Value math_abs(const Value& v);
    Value math_ceil(const Value& v);
    Value math_floor(const Value& v);
    Value math_round(const Value& v);
    Value math_trunc(const Value& v);
    Value math_sin(const Value& v);
    Value math_cos(const Value& v);
    Value math_tan(const Value& v);
    Value math_asin(const Value& v);
    Value math_acos(const Value& v);
    Value math_atan(const Value& v);
    Value math_radians(const Value& v);
    Value math_sinh(const Value& v);
    Value math_cosh(const Value& v);
    Value math_tanh(const Value& v);
    Value math_asinh(const Value& v);
    Value math_acosh(const Value& v);
    Value math_atanh(const Value& v);
    Value math_sqrt(const Value& v);
    Value math_cbrt(const Value& v);
    Value math_exp(const Value& v);
    Value math_expm1(const Value& v);
    Value math_log(const Value& v);
    Value math_log1p(const Value& v);
    Value math_log10(const Value& v);
    Value math_log2(const Value& v);
    Value math_pow(const Value& v);
    Value math_min(const Value& v);
    Value math_max(const Value& v);
    Value math_clamp(const Value& v);
    Value math_deg(const Value& v);
    Value math_sign(const Value& v);
    Value math_isnan(const Value& v);
    Value math_isfinite(const Value& v);
    Value math_isinf(const Value& v);
    Value math_modf(const Value& v);
    Value math_fmod(const Value& v);
    Value math_remainder(const Value& v);
    Value math_copysign(const Value& v);
    Value math_nextafter(const Value& v);
    Value math_gamma(const Value& v);
    Value math_lgamma(const Value& v);
    Value math_erf(const Value& v);
    Value math_erfc(const Value& v);
    Value math_hypot(const Value& v);
    Value math_dist(const Value& v);
    Value math_lerp(const Value& v);
    Value math_smoothstep(const Value& v);
    Value math_step(const Value& v);
    Value math_frac(const Value& v);
    Value math_wrap(const Value& v);
    Value math_remap(const Value& v);
    Value math_random(const Value& v);
    Value math_seed(const Value& v);
    Value math_gcd(const Value& v);
    Value math_lcm(const Value& v);
    Value math_fact(const Value& v);
    Value math_binom(const Value& v);
    Value math_perm(const Value& v);
    Value math_deg2rad(const Value& v);
    Value math_rad2deg(const Value& v);
    Value math_logb(const Value& v);
    Value math_ilogb(const Value& v);
    Value math_scalbn(const Value& v);
    Value math_scalbln(const Value& v);
    Value math_frexp(const Value& v);
    Value math_ldexp(const Value& v);
    Value math_roundeven(const Value& v);
    Value math_nearbyint(const Value& v);
    Value math_rint(const Value& v);
    Value math_exp2(const Value& v);
    Value math_expm1(const Value& v);
    Value math_log1p(const Value& v);
    Value math_log2(const Value& v);
    Value math_log10(const Value& v);
    Value math_cbrt(const Value& v);
    Value math_fdim(const Value& v);
    Value math_fmax(const Value& v);
    Value math_fmin(const Value& v);
    Value math_fma(const Value& v);
    Value math_nan(const Value& v);
    Value math_infinity(const Value& v);
    Value math_pi(const Value& v);
    Value math_e(const Value& v);
    Value math_tau(const Value& v);
    Value math_phi(const Value& v);
    // ... thêm các hàm khác nếu cần

    // Các hàm quản lý math_functions
    void initialize_math_functions();
    MathFunction get_math_function(const std::string& function_name);
    std::vector<std::string> get_math_functions();
    double get_math_constant(const std::string& constant_name);
    std::vector<std::string> get_math_constants();
}
} 