#include "os.hpp"
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <cstdio>
#if defined(_WIN32)
  #include <windows.h>
  #include <direct.h>
  #define linh_getcwd _getcwd
  #define linh_chdir  _chdir
#else
  #include <unistd.h>
  #include <limits.h>
  #define linh_getcwd getcwd
  #define linh_chdir  chdir
#endif

namespace Linh {
namespace Std {

    static std::unordered_map<std::string, OsFunction> os_functions;
    static std::unordered_map<std::string, Value> os_constants;

    // Return compile-time detected OS name as string Value
    Value os_name(const Value&) {
    #if defined(_WIN32) || defined(_WIN64)
        return Value(std::string("windows"));
    #elif defined(__APPLE__) && defined(__MACH__)
        return Value(std::string("macos"));
    #elif defined(__ANDROID__)
        return Value(std::string("android"));
    #elif defined(__linux__)
        return Value(std::string("linux"));
    #elif defined(__unix__)
        return Value(std::string("unix"));
    #else
        return Value(std::string("unknown"));
    #endif
    }

    // Return compile-time detected architecture name
    Value os_arch(const Value&) {
    #if defined(__x86_64__) || defined(_M_X64)
        return Value(std::string("x86_64"));
    #elif defined(__i386) || defined(_M_IX86)
        return Value(std::string("x86"));
    #elif defined(__aarch64__) || defined(_M_ARM64)
        return Value(std::string("arm64"));
    #elif defined(__arm__) || defined(_M_ARM)
        return Value(std::string("arm"));
    #elif defined(__ppc64__) || defined(__powerpc64__)
        return Value(std::string("ppc64"));
    #elif defined(__ppc__) || defined(__powerpc__)
        return Value(std::string("ppc"));
    #elif defined(__mips__)
        return Value(std::string("mips"));
    #elif defined(__riscv)
        return Value(std::string("riscv"));
    #else
        return Value(std::string("unknown"));
    #endif
    }

    // Get current working directory
    Value os_getcwd(const Value&) {
    #if defined(_WIN32)
        char buffer[MAX_PATH];
        if (linh_getcwd(buffer, MAX_PATH)) {
            return Value(std::string(buffer));
        }
        return Value{}; // sol on failure
    #else
        char buffer[PATH_MAX];
        if (linh_getcwd(buffer, sizeof(buffer))) {
            return Value(std::string(buffer));
        }
        return Value{}; // sol on failure
    #endif
    }

    // Change current working directory
    Value os_chdir(const Value& v) {
        if (!std::holds_alternative<std::string>(v)) return Value{}; // expect string path
        const auto& path = std::get<std::string>(v);
        int rc = linh_chdir(path.c_str());
        if (rc == 0) return Value(true);
        return Value(false);
    }

    void initialize_os_functions() {
        if (!os_functions.empty() && !os_constants.empty()) return;
        
        // Initialize constants
        if (os_constants.empty()) {
            os_constants["name"] = os_name(Value{});
            os_constants["arch"] = os_arch(Value{});
        }
        
        // Initialize functions (excluding name and arch which are now constants)
        if (os_functions.empty()) {
            os_functions["getcwd"] = os_getcwd;
            os_functions["chdir"] = os_chdir;
        }
    }

    OsFunction get_os_function(const std::string& function_name) {
        if (os_functions.empty()) initialize_os_functions();
        auto it = os_functions.find(function_name);
        if (it != os_functions.end()) return it->second;
        return nullptr;
    }

    std::vector<std::string> get_os_functions() {
        if (os_functions.empty()) initialize_os_functions();
        std::vector<std::string> names;
        names.reserve(os_functions.size());
        for (const auto& kv : os_functions) names.push_back(kv.first);
        return names;
    }

    Value get_os_constant(const std::string& constant_name) {
        if (os_constants.empty()) initialize_os_functions();
        auto it = os_constants.find(constant_name);
        if (it != os_constants.end()) return it->second;
        return Value{};
    }

    std::vector<std::string> get_os_constants() {
        if (os_constants.empty()) initialize_os_functions();
        std::vector<std::string> names;
        names.reserve(os_constants.size());
        for (const auto& kv : os_constants) names.push_back(kv.first);
        return names;
    }
}
}