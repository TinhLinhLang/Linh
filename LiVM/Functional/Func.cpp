#include "Func.hpp"
#include "LiVM/LiVM.hpp"
#include "LinhC/Bytecode/Bytecode.hpp"
#include <iostream>

// Forward declarations for handler functions
namespace Linh {
    void handle_comparison(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
    void handle_ADD(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
    void handle_SUB(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
    void handle_MUL(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
    void handle_DIV(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
    void handle_CALL(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
    bool eval_condition(const Value& cond);
}

namespace Linh {
    // FunctionObject constructor implementation
    FunctionObject::FunctionObject(const std::string& n, const std::vector<FunctionParameter>& p, 
                                  const BytecodeChunk& b, const ClosureEnvironment& env)
        : name(n), params(p), body(b), environment(env), is_closure(true) {}

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
        auto original_vars = vm.variables;
        
        // Tạo môi trường local cho function - tạo bản sao để không ảnh hưởng đến caller
        std::unordered_map<int, Value> local_vars = original_vars;

        // Nếu là closure, restore captured environment
        if (fn->is_closure) {
#ifdef _DEBUG
            std::cerr << "[DEBUG] Restoring closure environment with " << fn->environment.size() << " variables" << std::endl;
            for (const auto& [var_name, var_value] : fn->environment) {
                std::cerr << "[DEBUG] Closure env: " << var_name << " = ";
                if (Linh::holds_alternative<int64_t>(var_value)) {
                    std::cerr << Linh::get<int64_t>(var_value);
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
                    local_vars[0] = var_value; // Assume count is at index 0
#ifdef _DEBUG
                    std::cerr << "[DEBUG] Restored count to index 0 with value: ";
                    if (Linh::holds_alternative<int64_t>(var_value)) {
                        std::cerr << Linh::get<int64_t>(var_value);
                    }
                    std::cerr << std::endl;
#endif
                } else if (var_name == "var_0") {
                    local_vars[0] = var_value;
                } else if (var_name == "var_1") {
                    local_vars[1] = var_value;
                } else if (var_name == "var_2") {
                    local_vars[2] = var_value;
                } else {
                    // For other variables, use a simple hash-based approach
                    int var_index = std::hash<std::string>{}(var_name) % 1000;
                    local_vars[var_index] = var_value;
                }
            }
        }

        // Bind tham số với giá trị
        for (size_t i = 0; i < fn->params.size(); ++i) {
            // TODO: Thêm type checking cho static parameters (vas)
#ifdef _DEBUG
            std::cerr << "[DEBUG] Binding parameter " << i << " with value index = " << args[i].get_type() << std::endl;
#endif
            local_vars[i] = args[i];
        }

        // Set local variables for function execution
        vm.variables = local_vars;

        // Lưu kích thước stack hiện tại để biết return value ở đâu
        size_t stack_base = vm.stack.size();

        // Thực thi thân hàm với IP riêng biệt
        size_t local_ip = 0;
        while (local_ip < fn->body.size()) {
            const auto &instr = fn->body[local_ip];
            
            switch (instr.opcode) {
                case OpCode::PUSH_INT:
                    vm.push(std::get<int64_t>(instr.operand));
                    break;
                case OpCode::LOAD_VAR: {
                    int var_index = std::get<int64_t>(instr.operand);
                    if (vm.variables.count(var_index)) {
                        vm.push(vm.variables[var_index]);
                    } else {
                        vm.push(Value());
                    }
                    break;
                }
                case OpCode::LTE:
                case OpCode::LT:
                case OpCode::GT:
                case OpCode::GTE:
                case OpCode::EQ:
                case OpCode::NEQ:
                    handle_comparison(vm, instr, fn->body, local_ip);
                    break;
                case OpCode::ADD:
                    handle_ADD(vm, instr, fn->body, local_ip);
                    break;
                case OpCode::SUB:
                    handle_SUB(vm, instr, fn->body, local_ip);
                    break;
                case OpCode::MUL:
                    handle_MUL(vm, instr, fn->body, local_ip);
                    break;
                case OpCode::DIV:
                    handle_DIV(vm, instr, fn->body, local_ip);
                    break;
                case OpCode::JMP_IF_FALSE: {
                    auto cond = vm.pop();
                    bool cond_val = eval_condition(cond);
                    if (!cond_val) {
                        local_ip = static_cast<size_t>(std::get<int64_t>(instr.operand));
                        continue; // Skip the local_ip++ at the end
                    }
                    break;
                }
                case OpCode::CALL: {
                    // Handle recursive function calls within the function
                    if (!vm.stack.empty() && vm.stack.back().get_type() == ValueType::Function) {
                        FunctionPtr recursive_fn = Linh::get<FunctionPtr>(vm.stack.back());
                        vm.pop(); // Pop function object
                        
                        // Collect arguments
                        std::vector<Value> args;
                        size_t expected_args = recursive_fn->params.size();
                        for (size_t i = 0; i < expected_args; ++i) {
                            if (vm.stack.empty()) {
                                std::cerr << "Error: Not enough arguments for recursive function " << recursive_fn->name << std::endl;
                                vm.push(Value());
                                break;
                            }
                            args.insert(args.begin(), vm.pop());
                        }
                        
                        // Call the function recursively
                        auto result = call_function(recursive_fn, args, vm);
                        // Result is already pushed by call_function
                    }
                    break;
                }
                case OpCode::RET:
                    goto function_return; // Exit the function
                default:
                    std::cerr << "VM: Unknown opcode in function: " << static_cast<int>(instr.opcode) << std::endl;
                    break;
            }
            local_ip++;
        }
        
        function_return:

        // Lấy kết quả từ stack của hàm (nếu có)
        Value result = Value(); // Default nothing
        if (vm.stack.size() > stack_base) {
            result = vm.stack.back();
            vm.stack.pop_back(); // Remove result from stack
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
                    if (Linh::holds_alternative<int64_t>(var_value)) {
                        std::cerr << Linh::get<int64_t>(var_value);
                    }
                    std::cerr << std::endl;
#endif
                }
            }
        }

        // Khôi phục variables và push kết quả
        vm.variables = original_vars;
        vm.push(result);

        return result;
    }
}
