#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include "LiVM/Variable/Value.hpp"

// Forward declarations
namespace Linh {
    class LiVM;
    struct Instruction;
    using BytecodeChunk = std::vector<Instruction>;
}

namespace Linh {
    // Thêm struct để lưu thông tin tham số
    struct FunctionParameter {
        std::string name;
        std::optional<std::string> type; // Kiểu dữ liệu (nếu có)
        bool is_static; // true nếu có 'vas' keyword
        
        FunctionParameter(const std::string& n, const std::optional<std::string>& t = std::nullopt, bool static_ = false)
            : name(n), type(t), is_static(static_) {}
    };

    // Environment for closures - maps variable names to values
    using ClosureEnvironment = std::unordered_map<std::string, Value>;

    struct FunctionObject {
        std::string name;
        std::vector<FunctionParameter> params;
        BytecodeChunk body; // Thân hàm dưới dạng bytecode
        ClosureEnvironment environment; // Environment for closures - captured variables
        bool is_closure = false; // Flag to indicate if this is a closure
        
        // Constructor for regular functions
        FunctionObject() = default;
        
        // Constructor for closures
        FunctionObject(const std::string& n, const std::vector<FunctionParameter>& p, 
                      const BytecodeChunk& b, const ClosureEnvironment& env);
    };

    using FunctionPtr = std::shared_ptr<FunctionObject>;

    // Tạo function object
    FunctionPtr create_function(const std::string& name, const std::vector<FunctionParameter>& params, const BytecodeChunk& body);
    
    // Tạo closure object
    FunctionPtr create_closure(const std::string& name, const std::vector<FunctionParameter>& params, 
                              const BytecodeChunk& body, const ClosureEnvironment& environment);
    
    // Gọi function object
    Value call_function(FunctionPtr fn, const std::vector<Value>& args, LiVM& vm);
} 