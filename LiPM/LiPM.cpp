#include <unordered_map>
#include <string>
#include <memory>
#include <cmath>
#include <functional>
#include "LiVM/Value/Value.hpp"
#include "LiVM/LiVM.hpp"
#include "../config.hpp"
#include "Package/time.hpp"
#include "Package/math.hpp"
#include "Package/fs.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Linh
{
    namespace LiPM
    {
        using MathFunction = std::function<Value(const Value&)>;
        using TimeFunction = std::function<Value(const Value&)>;
        using FsFunction = std::function<Value(const Value&)>;
        static std::unordered_map<std::string, std::unordered_map<std::string, Value>> default_packages;

        void initialize_default_packages()
        {
            // Initialize math package
            initialize_math_functions();
            
            // Initialize time package
            initialize_time_functions();
            
            // Initialize fs package
            initialize_fs_functions();
            initialize_fs_constants();
        }

        const std::unordered_map<std::string, Value>* get_package(const std::string& package_name)
        {
            if (default_packages.empty())
            {
                initialize_default_packages();
            }
            auto it = default_packages.find(package_name);
            if (it != default_packages.end())
            {
                return &(it->second);
            }
            return nullptr;
        }

        Value get_constant(const std::string& package_name, const std::string& constant_name) {
            if (package_name == "fs") {
                return Linh::LiPM::get_fs_constant(constant_name);
            }
            // Có thể mở rộng cho các package khác
            return Value{};
        }

        bool package_exists(const std::string& package_name)
        {
            // Built-in packages
            if (package_name == "math" || package_name == "time" || package_name == "fs") {
                return true;
            }
            
            if (default_packages.empty())
            {
                initialize_default_packages();
            }
            return default_packages.find(package_name) != default_packages.end();
        }

        std::vector<std::string> get_available_packages()
        {
            if (default_packages.empty())
            {
                initialize_default_packages();
            }
            std::vector<std::string> package_names;
            for (const auto& pair : default_packages)
            {
                package_names.push_back(pair.first);
            }
            return package_names;
        }

        std::vector<std::string> get_package_constants(const std::string& package_name)
        {
            const auto* package = get_package(package_name);
            if (package)
            {
                std::vector<std::string> constant_names;
                for (const auto& pair : *package)
                {
                    constant_names.push_back(pair.first);
                }
                return constant_names;
            }
            return {};
        }

    }
} 