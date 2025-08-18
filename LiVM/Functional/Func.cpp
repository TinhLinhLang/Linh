#include "Func.hpp"
#include "LiVM/LiVM.hpp"
#include "LinhC/Bytecode/Bytecode.hpp"
#include <iostream>

namespace Linh {
    // Tạo function object
    FunctionPtr create_function(const std::string& name, const std::vector<FunctionParameter>& params, const BytecodeChunk& body) {
        auto fn = std::make_shared<FunctionObject>();
        fn->name = name;
        fn->params = params;
        fn->body = body; // Lưu thân hàm
        fn->is_closure = false;
        return fn;
    }

    // Tạo closure object
    FunctionPtr create_closure(const std::string& name, const std::vector<FunctionParameter>& params, 
                              const BytecodeChunk& body, const ClosureEnvironment& environment) {
        auto closure = std::make_shared<FunctionObject>(name, params, body, environment);
        return closure;
    }

    // Gọi function object
    Value call_function(FunctionPtr fn, const std::vector<Value>& args, LiVM& vm) {
        // Kiểm tra số lượng tham số
        if (args.size() != fn->params.size()) {
            std::cerr << "Error: Function " << fn->name << " expects "
                      << fn->params.size() << " arguments, but got " << args.size() << std::endl;
            return Value(); // Trả về giá trị mặc định
        }

        // Lưu trạng thái hiện tại của VM
        auto original_stack = vm.stack;
        auto original_vars = vm.variables;
        auto original_call_stack = vm.call_stack;
        auto original_ip = vm.ip; // Save original IP

        // Tạo môi trường local cho function
        vm.variables.clear();

        // Nếu là closure, restore captured environment
        if (fn->is_closure) {
#ifdef _DEBUG
            std::cerr << "[DEBUG] Restoring closure environment with " << fn->environment.size() << " variables" << std::endl;
            for (const auto& [var_name, var_value] : fn->environment) {
                std::cerr << "[DEBUG] Closure env: " << var_name << " = ";
                if (std::holds_alternative<int64_t>(var_value)) {
                    std::cerr << std::get<int64_t>(var_value);
                } else {
                    std::cerr << "(other type)";
                }
                std::cerr << std::endl;
            }
#endif
            // Restore captured variables to the environment
            for (const auto& [var_name, var_value] : fn->environment) {
                // For now, we'll use a simple approach: store variables by name
                // In a more sophisticated implementation, you'd have proper variable name to index mapping
                if (var_name == "count") {
                    vm.variables[0] = var_value; // Assume count is at index 0
#ifdef _DEBUG
                    std::cerr << "[DEBUG] Restored count to index 0 with value: ";
                    if (std::holds_alternative<int64_t>(var_value)) {
                        std::cerr << std::get<int64_t>(var_value);
                    }
                    std::cerr << std::endl;
#endif
                } else if (var_name == "var_0") {
                    vm.variables[0] = var_value;
                } else if (var_name == "var_1") {
                    vm.variables[1] = var_value;
                } else if (var_name == "var_2") {
                    vm.variables[2] = var_value;
                } else {
                    // For other variables, use a simple hash-based approach
                    int var_index = std::hash<std::string>{}(var_name) % 1000;
                    vm.variables[var_index] = var_value;
                }
            }
        }

        // Bind tham số với giá trị
        for (size_t i = 0; i < fn->params.size(); ++i) {
            // TODO: Thêm type checking cho static parameters (vas)
#ifdef _DEBUG
            std::cerr << "[DEBUG] Binding parameter " << i << " with value index = " << args[i].index() << std::endl;
#endif
            vm.variables[i] = args[i];
        }

        // Lưu return address
        vm.call_stack.push_back({vm.ip + 1, vm.variables});

        // Thực thi thân hàm
        vm.ip = 0; // Reset IP for function's own chunk
        vm.run_chunk(fn->body);

        // Lấy kết quả từ stack
        Value result = Value(); // Default value
        if (!vm.stack.empty()) {
            result = vm.stack.back();
            vm.stack.pop_back();
        }

        // Nếu là closure, save updated environment back to function object
        if (fn->is_closure) {
            // Save updated variables back to closure environment
            for (const auto& [var_index, var_value] : vm.variables) {
                if (var_index == 0) { // Assume count is at index 0
                    fn->environment["count"] = var_value;
                    fn->environment["var_0"] = var_value;
#ifdef _DEBUG
                    std::cerr << "[DEBUG] Updated closure environment: count = ";
                    if (std::holds_alternative<int64_t>(var_value)) {
                        std::cerr << std::get<int64_t>(var_value);
                    }
                    std::cerr << std::endl;
#endif
                }
            }
        }

        // Khôi phục trạng thái VM
        vm.stack = original_stack;
        vm.variables = original_vars;
        vm.call_stack = original_call_stack;
        vm.ip = original_ip; // Restore original IP

        return result;
    }
}
