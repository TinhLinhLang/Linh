#pragma once
#include <string>
#include <vector>
#include <functional>
#include "../../Value/Value.hpp"

namespace Linh {
namespace Std {
    using OsFunction = std::function<Value(const Value&)>;

    // os.name() -> string: returns operating system name
    Value os_name(const Value& v);
    // os.arch() -> string: returns architecture name (e.g., x86_64, arm64, ...)
    Value os_arch(const Value& v);

    // Additional functions
    Value os_getcwd(const Value& v);
    Value os_chdir(const Value& v);

    // Registry
    void initialize_os_functions();
    OsFunction get_os_function(const std::string& function_name);
    std::vector<std::string> get_os_functions();
    Value get_os_constant(const std::string& constant_name);
    std::vector<std::string> get_os_constants();
}
}