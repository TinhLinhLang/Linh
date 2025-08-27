#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "../Value/Value.hpp"

namespace Linh
{
    namespace Std
    {
        // Function type for math functions
        using MathFunction = std::function<Value(const Value&)>;
        
        // Function type for time functions
        using TimeFunction = std::function<Value(const Value&)>;
        
        // Function type for fs functions
        using FsFunction = std::function<Value(const Value&)>;
        using JsonFunction = std::function<Value(const Value&)>;
        using OsFunction = std::function<Value(const Value&)>;
        
        // Initialize default packages
        void initialize_default_packages();

        // Get a package by name
        const std::unordered_map<std::string, Value>* get_package(const std::string& package_name);

        // Get a specific constant from a package
        Value get_constant(const std::string& package_name, const std::string& constant_name);

        // Get a math function by name
        MathFunction get_math_function(const std::string& function_name);

        // Get a time function by name
        TimeFunction get_time_function(const std::string& function_name);

        // Get a fs function by name
        FsFunction get_fs_function(const std::string& function_name);
        
        // Get a json function by name
        JsonFunction get_json_function(const std::string& function_name);
        
        // Get an os function by name
        OsFunction get_os_function(const std::string& function_name);

        // Get constants from packages
        double get_math_constant(const std::string& constant_name);
        double get_time_constant(const std::string& constant_name);
        Value get_os_constant(const std::string& constant_name);
        Value get_fs_constant(const std::string& constant_name);

        // Initialize functions
        void initialize_math_functions();
        void initialize_time_functions();
        void initialize_fs_functions();
        void initialize_fs_constants();
        void initialize_json_functions();
        void initialize_os_functions();

        // Check if a package exists
        bool package_exists(const std::string& package_name);

        // Get all available package names
        std::vector<std::string> get_available_packages();

        // Get all constants in a package
        std::vector<std::string> get_package_constants(const std::string& package_name);

        // Get all functions in math package
        std::vector<std::string> get_math_functions();

        // Get all functions in time package
        std::vector<std::string> get_time_functions();

        // Get all functions in fs package
        std::vector<std::string> get_fs_functions();
        
        // Get all functions in json package
        std::vector<std::string> get_json_functions();
        
        // Get all functions in os package
        std::vector<std::string> get_os_functions();
    }
} 