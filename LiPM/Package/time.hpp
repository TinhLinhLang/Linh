#pragma once
#include <string>
#include <vector>
#include <functional>
#include "../../LiVM/Value/Value.hpp"

namespace Linh {
namespace LiPM {
    using TimeFunction = std::function<Value(const Value&)>;
    
    // Khai báo hàm time
    Value time_time(const Value& v);
    Value time_sleep(const Value& v);
    Value time_process_time(const Value& v);
    
    // Các hàm quản lý time constants
    void initialize_time_constants();
    double get_time_constant(const std::string& constant_name);
    std::vector<std::string> get_time_constants();
    
    // Các hàm quản lý time functions
    void initialize_time_functions();
    TimeFunction get_time_function(const std::string& function_name);
    std::vector<std::string> get_time_functions();
}
} 