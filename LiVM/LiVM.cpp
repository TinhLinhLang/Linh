#include "LiVM.hpp"
#include "Value/Value.hpp" // Để sử dụng make_array() và make_map()
#include "iostream/iostream.hpp"
#include "Loop.hpp"
#include <cmath>
#include "Math/Math.hpp" // Thêm dòng này
#include <iomanip>
#include "type.hpp"
#include <variant>
#include "Std/Std.cpp" // Thêm dòng này cho LiPM support
#include "Std/Package/json.hpp" // Ensure inline json functions are visible
#include <functional>
#include <array>
#include <fmt/format.h>
#include <vector>
#include "Functional/Func.hpp" // Thêm dòng này cho Function support

#ifdef _DEBUG
// Helper to print a Value for debug
static void debug_print_value(const Linh::Value& v, std::ostream& os = std::cerr) {
    os << "[" << Linh::type_of(v) << "] ";
    os << Linh::to_str(v);
}
static void debug_print_stack(const std::vector<Linh::Value>& stack, std::ostream& os = std::cerr) {
    os << "[STACK size=" << stack.size() << "] ";
    for (const auto& v : stack) {
        debug_print_value(v, os);
        os << ", ";
    }
    os << std::endl;
}
static void debug_print_vars(const std::unordered_map<int, Linh::Value>& vars, std::ostream& os = std::cerr) {
    os << "[VARS size=" << vars.size() << "] ";
    for (const auto& kv : vars) {
        os << "idx=" << kv.first << ": ";
        debug_print_value(kv.second, os);
        os << "; ";
    }
    os << std::endl;
}
#endif

struct TryFrame
{
    size_t catch_ip;
    size_t finally_ip;
    size_t end_ip;
    std::string error_var;
    TryFrame(size_t c, size_t f, size_t e, std::string err = "error")
        : catch_ip(c), finally_ip(f), end_ip(e), error_var(std::move(err)) {}
};

namespace Linh
{

    LiVM::LiVM() {
        // Pre-reserve stack space for better performance
        if (stack_optimization_enabled) {
            stack.reserve(STACK_RESERVE_SIZE);
        }
    }

    // Optimization helpers implementation
    void LiVM::cache_instruction(size_t ip, const Instruction& instr) {
        if (!instruction_caching_enabled) return;
        
        // Cache frequently executed instructions
        instruction_cache[ip] = [this, instr]() {
            // Execute the instruction directly without lookup
            switch (instr.opcode) {
                case OpCode::PUSH_INT:
                    push(Value(static_cast<int32_t>(std::get<int64_t>(instr.operand))));
                    break;
                case OpCode::PUSH_FLOAT:
                    push(Value(static_cast<float>(std::get<double>(instr.operand))));
                    break;
                case OpCode::PUSH_BOOL:
                    push(Value(std::get<bool>(instr.operand)));
                    break;
                case OpCode::PUSH_STR:
                    push(Value(std::get<std::string>(instr.operand)));
                    break;
                case OpCode::POP:
                    pop();
                    break;
                case OpCode::ADD:
                case OpCode::SUB:
                case OpCode::MUL:
                case OpCode::DIV:
                case OpCode::MOD:
                case OpCode::HASH:
                case OpCode::AMP:
                case OpCode::PIPE:
                case OpCode::CARET:
                case OpCode::LT_LT:
                case OpCode::GT_GT:
                    math_binary_op(*this, instr);
                    break;
                default:
                    // For complex instructions, don't cache
                    break;
            }
        };
    }
    
    void LiVM::optimize_stack() {
        if (!stack_optimization_enabled) return;
        
        // Shrink stack if it's too large
        if (stack.size() < STACK_SHRINK_THRESHOLD && stack.capacity() > STACK_RESERVE_SIZE) {
            stack.shrink_to_fit();
        }
        
        // Ensure minimum capacity
        if (stack.capacity() < STACK_RESERVE_SIZE) {
            stack.reserve(STACK_RESERVE_SIZE);
        }
    }
    
    void LiVM::track_hot_path(size_t ip) {
        if (!hot_path_optimization_enabled) return;
        
        hot_paths.insert(ip);
        
        // If this is a hot path, consider caching the instruction
        if (hot_paths.count(ip) > 3) { // Execute 3+ times to be considered hot
            // Could implement more sophisticated hot path detection here
        }
    }
    
    void LiVM::execute_cached_instruction(size_t ip) {
        auto it = instruction_cache.find(ip);
        if (it != instruction_cache.end()) {
            cache_hits++;
            it->second();
        } else {
            cache_misses++;
        }
    }

    void LiVM::push(const Value &val)
    {
        stack.push_back(val);
    }

    Value LiVM::pop()
    {
        if (stack.empty())
        {
            std::cerr << "ERROR [Line: 0, Col: 0] RuntimeError : VM stack underflow" << std::endl;
            return Value{}; // trả về sol
        }
        auto val = stack.back();
        stack.pop_back();
        return val;
    }

    Value LiVM::peek()
    {
        if (stack.empty())
            throw std::runtime_error("VM stack empty");
        return stack.back();
    }

    // Add this helper function at the top (after includes)
    static const char *opcode_name(Linh::OpCode opcode)
    {
        using Linh::OpCode;
        switch (opcode)
        {
        case OpCode::NOP:
            return "NOP";
        case OpCode::PUSH_INT:
            return "PUSH_INT";
        case OpCode::PUSH_UINT:
            return "PUSH_UINT";
        case OpCode::PUSH_FLOAT:
            return "PUSH_FLOAT";
        case OpCode::PUSH_STR:
            return "PUSH_STR";
        case OpCode::PUSH_BOOL:
            return "PUSH_BOOL";
        case OpCode::POP:
            return "POP";
        case OpCode::SWAP:
            return "SWAP";
        case OpCode::DUP:
            return "DUP";
        case OpCode::ADD:
            return "ADD";
        case OpCode::SUB:
            return "SUB";
        case OpCode::MUL:
            return "MUL";
        case OpCode::DIV:
            return "DIV";
        case OpCode::MOD:
            return "MOD";
        case OpCode::HASH:
            return "HASH";
        case OpCode::AMP:
            return "AMP";
        case OpCode::PIPE:
            return "PIPE";
        case OpCode::CARET:
            return "CARET";
        case OpCode::LT_LT:
            return "LT_LT";
        case OpCode::GT_GT:
            return "GT_GT";
        case OpCode::AND:
            return "AND";
        case OpCode::OR:
            return "OR";
        case OpCode::NOT:
            return "NOT";
        case OpCode::EQ:
            return "EQ";
        case OpCode::NEQ:
            return "NEQ";
        case OpCode::LT:
            return "LT";
        case OpCode::GT:
            return "GT";
        case OpCode::LTE:
            return "LTE";
        case OpCode::GTE:
            return "GTE";
        case OpCode::LOAD_VAR:
            return "LOAD_VAR";
        case OpCode::STORE_VAR:
            return "STORE_VAR";
        case OpCode::JMP:
            return "JMP";
        case OpCode::JMP_IF_FALSE:
            return "JMP_IF_FALSE";
        case OpCode::JMP_IF_TRUE:
            return "JMP_IF_TRUE";
        case OpCode::CALL:
            return "CALL";
        case OpCode::RET:
            return "RET";
        case OpCode::PUSH_FUNCTION:
            return "PUSH_FUNCTION";
        case OpCode::PRINT:
            return "PRINT";
        case OpCode::INPUT:
            return "INPUT";
        case OpCode::TYPEOF:
            return "TYPEOF";
        case OpCode::HALT:
            return "HALT";
        case OpCode::TRY:
            return "TRY";
        case OpCode::END_TRY:
            return "END_TRY";
        case OpCode::ID:
            return "ID";
        case OpCode::LOAD_PACKAGE_CONST:
            return "LOAD_PACKAGE_CONST";
        case OpCode::CALL_PACKAGE_FUNCTION:
            return "CALL_PACKAGE_FUNCTION";
        case OpCode::PUSH_ARRAY:
            return "PUSH_ARRAY";
        case OpCode::PUSH_MAP:
            return "PUSH_MAP";
        case OpCode::ARRAY_GET:
            return "ARRAY_GET";
        case OpCode::ARRAY_SET:
            return "ARRAY_SET";
        case OpCode::MAP_GET:
            return "MAP_GET";
        case OpCode::MAP_SET:
            return "MAP_SET";
        case OpCode::ARRAY_LEN:
            return "ARRAY_LEN";
        case OpCode::ARRAY_APPEND:
            return "ARRAY_APPEND";
        case OpCode::ARRAY_REMOVE:
            return "ARRAY_REMOVE";
        case OpCode::ARRAY_CLEAR:
            return "ARRAY_CLEAR";
        case OpCode::ARRAY_CLONE:
            return "ARRAY_CLONE";
        case OpCode::ARRAY_POP:
            return "ARRAY_POP";
        case OpCode::MAP_KEYS:
            return "MAP_KEYS";
        case OpCode::MAP_VALUES:
            return "MAP_VALUES";
        case OpCode::MAP_DELETE:
            return "MAP_DELETE";
        case OpCode::MAP_CLEAR:
            return "MAP_CLEAR";
        case OpCode::PRINT_MULTIPLE:
            return "PRINT_MULTIPLE";
        case OpCode::PRINTF:
            return "PRINTF";
        case OpCode::CREATE_CLOSURE:
            return "CREATE_CLOSURE";
        case OpCode::CAPTURE_VAR:
            return "CAPTURE_VAR";
        case OpCode::LOAD_CLOSURE_VAR:
            return "LOAD_CLOSURE_VAR";
        default:
            return "UNKNOWN";
        }
    }

    // Forward declarations for opcode handlers
    using OpHandler = void(*)(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);

    // Handler functions for basic opcodes
    static void handle_PUSH_INT(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        vm.push(Value(static_cast<int32_t>(std::get<int64_t>(instr.operand))));
    }
    static void handle_PUSH_FLOAT(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        vm.push(Value(static_cast<float>(std::get<double>(instr.operand))));
    }
    static void handle_ADD(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        Linh::math_binary_op(vm, instr);
    }
    static void handle_SUB(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        Linh::math_binary_op(vm, instr);
    }
    static void handle_MUL(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        Linh::math_binary_op(vm, instr);
    }
    static void handle_DIV(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        Linh::math_binary_op(vm, instr);
    }
    static void handle_MOD(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        Linh::math_binary_op(vm, instr);
    }
    static void handle_HASH(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        Linh::math_binary_op(vm, instr);
    }
    static void handle_AMP(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        Linh::math_binary_op(vm, instr);
    }
    static void handle_PIPE(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        Linh::math_binary_op(vm, instr);
    }
    static void handle_CARET(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        Linh::math_binary_op(vm, instr);
    }
    static void handle_LT_LT(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        Linh::math_binary_op(vm, instr);
    }
    static void handle_GT_GT(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        Linh::math_binary_op(vm, instr);
    }
    static void handle_JMP(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t& ip) {
        ip = static_cast<size_t>(std::get<int64_t>(instr.operand));
    }
    static void handle_JMP_IF_FALSE(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t& ip) {
        auto cond = vm.pop();
        bool cond_val = eval_condition(cond);
        if (!cond_val)
            ip = static_cast<size_t>(std::get<int64_t>(instr.operand));
        else
            ++ip;
    }
    static void handle_PRINT(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        if (vm.stack.empty()) vm.stack.push_back(std::monostate{});
        auto val = vm.pop();
        LinhIO::linh_print(val);
    }
    static void handle_NOP(LiVM&, const Instruction&, const BytecodeChunk&, size_t&) {}

    static void handle_PRINT_MULTIPLE(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        int32_t count = static_cast<int32_t>(std::get<int64_t>(instr.operand));
        if (vm.stack.size() < static_cast<size_t>(count)) {
            std::cerr << "ERROR [Line " << instr.line << ", Col " << instr.col << "] RuntimeError : VM stack underflow for PRINT_MULTIPLE" << std::endl;
            return;
        }
        std::vector<std::string> strings;
        for (int i = 0; i < count; i++) {
            auto val = vm.pop();
            strings.push_back(Linh::to_str(val));
        }
        std::reverse(strings.begin(), strings.end());
        std::string result;
        for (size_t i = 0; i < strings.size(); i++) {
            if (i > 0) result += " ";
            result += strings[i];
        }
        LinhIO::linh_print(Value(result));
    }
    static void handle_PRINTF(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        if (vm.stack.empty()) {
            std::cerr << "ERROR [Line " << instr.line << ", Col " << instr.col << "] RuntimeError : VM stack underflow" << std::endl;
            return;
        }
        auto val = vm.pop();
        LinhIO::linh_printf(val);
    }

    static void handle_POP(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        vm.pop();
    }
    static void handle_SWAP(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        if (vm.stack.size() < 2) {
            std::cerr << "VM stack underflow for SWAP" << std::endl;
            return;
        }
        std::swap(vm.stack[vm.stack.size() - 1], vm.stack[vm.stack.size() - 2]);
    }
    static void handle_DUP(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        if (vm.stack.empty()) {
            std::cerr << "VM stack underflow for DUP" << std::endl;
            return;
        }
        vm.stack.push_back(vm.stack.back());
    }
    static void handle_PUSH_UINT(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        vm.push(Value(static_cast<uint32_t>(std::get<uint64_t>(instr.operand))));
    }
    static void handle_PUSH_STR(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        vm.push(Value(std::get<std::string>(instr.operand)));
    }
    static void handle_PUSH_BOOL(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        vm.push(Value(std::get<bool>(instr.operand)));
    }
    static void handle_PUSH_FUNCTION(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        try {
#ifdef _DEBUG
            std::cerr << "[DEBUG] PUSH_FUNCTION: operand index = " << instr.operand.index() << std::endl;
#endif
            if (instr.operand.index() == 6) { // FunctionPtr is at index 6 in BytecodeValue
                auto fn = std::get<6>(instr.operand);
                Value function_value(fn);
#ifdef _DEBUG
                std::cerr << "[DEBUG] PUSH_FUNCTION: function_value index = " << function_value.index() << std::endl;
#endif
                vm.push(function_value);
#ifdef _DEBUG
                std::cerr << "[DEBUG] PUSH_FUNCTION: pushed function object, stack size = " << vm.stack.size() << std::endl;
#endif
            } else {
                std::cerr << "[ERROR] PUSH_FUNCTION: wrong operand type, expected FunctionPtr, got index " << instr.operand.index() << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] PUSH_FUNCTION: exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] PUSH_FUNCTION: unknown exception" << std::endl;
        }
    }

    static void handle_LOAD_PACKAGE_CONST(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        // Operand là tên hằng số package (string) dạng "package.constant"
        std::string full_name = std::get<std::string>(instr.operand);
        
        // Tách package name và constant name
        size_t dot_pos = full_name.find('.');
        if (dot_pos == std::string::npos) {
            std::cerr << "ERROR: Invalid package constant format: " << full_name << std::endl;
            vm.push(Value{}); // Push sol
            return;
        }
        
        std::string package_name = full_name.substr(0, dot_pos);
        std::string constant_name = full_name.substr(dot_pos + 1);
        
        // Sử dụng Std để lấy giá trị hằng số
        Value constant_value = Linh::Std::get_constant(package_name, constant_name);
        vm.push(constant_value);
    }

    static void handle_CALL_PACKAGE_FUNCTION(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        // Operand là tên function package (string) dạng "package.function"
        std::string full_name = std::get<std::string>(instr.operand);
        
        // Tách package name và function name
        size_t dot_pos = full_name.find('.');
        if (dot_pos == std::string::npos) {
            std::cerr << "VM: Invalid package function format: " << full_name << std::endl;
            vm.push(Value{});
            return;
        }
        
        std::string package_name = full_name.substr(0, dot_pos);
        std::string function_name = full_name.substr(dot_pos + 1);
        
        // Lấy function từ package và gọi nó
        if (package_name == "fs") {
            auto fs_func = Linh::Std::get_fs_function(function_name);
            if (fs_func) {
                // Special handling for fs functions that need 2 arguments
                if (function_name == "rename" || function_name == "copy" || function_name == "move") {
                    if (vm.stack.size() >= 2) {
                        Value dst = vm.pop();  // Second argument (destination)
                        Value src = vm.pop();  // First argument (source)
                        
                        // Create array with both arguments
                        Array args = make_array();
                        args->push_back(src);
                        args->push_back(dst);
                        Value result = fs_func(Value(args));
                        vm.push(result);
                        return;
                    } else {
                        vm.push(Value(false));
                        return;
                    }
                } else {
                    Value arg = vm.stack.empty() ? Value{} : vm.pop();
                    Value result = fs_func(arg);
                    vm.push(result);
                    return;
                }
            }
        } else if (package_name == "json") {
            auto json_func = Linh::Std::get_json_function(function_name);
            if (json_func) {
                Value arg = vm.stack.empty() ? Value{} : vm.pop();
                Value result = json_func(arg);
                vm.push(result);
                return;
            }
        } else if (package_name == "os") {
            auto os_func = Linh::Std::get_os_function(function_name);
            if (os_func) {
                Value arg = vm.stack.empty() ? Value{} : vm.pop();
                Value result = os_func(arg);
                vm.push(result);
                return;
            }
        }
        std::cerr << "VM: Package " << package_name << " not supported or function not found: " << full_name << std::endl;
        vm.push(Value{});
    }

    // Closure support handlers
    static void handle_CREATE_CLOSURE(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        // Expect a function object on top of stack
        if (vm.stack.empty()) {
            std::cerr << "Error: CREATE_CLOSURE requires a function on stack" << std::endl;
            return;
        }

        auto function_value = vm.stack.back();
        vm.stack.pop_back();

        if (function_value.index() != 15) { // FunctionPtr index
            std::cerr << "Error: CREATE_CLOSURE expected function object" << std::endl;
            return;
        }

        auto fn = std::get<15>(function_value);
        
        // Create closure with current environment
        auto closure = Linh::create_closure(fn->name, fn->params, fn->body, ClosureEnvironment{});
        
        // Push closure onto stack
        vm.push(Value(closure));
    }

    static void handle_CAPTURE_VAR(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        // Capture a variable from current scope into environment
        std::string var_name = std::get<std::string>(instr.operand);
        
        // Look for variable in current scope (variables map)
        // This is a simplified approach - in a real implementation you'd have proper scoping
        bool found = false;
        for (const auto& [var_index, var_value] : vm.variables) {
            // For now, we'll assume the variable name matches the index pattern
            // In a more sophisticated implementation, you'd have proper variable name tracking
            std::string expected_name = "var_" + std::to_string(var_index);
            if (var_name == expected_name || var_name == "count") {
                vm.current_environment[var_name] = var_value;
                found = true;
                break;
            }
        }
        
        if (!found) {
            std::cerr << "Warning: Variable '" << var_name << "' not found for capture" << std::endl;
        }
    }

    static void handle_LOAD_CLOSURE_VAR(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        // Load a variable from closure environment
        std::string var_name = std::get<std::string>(instr.operand);
        
        auto it = vm.current_environment.find(var_name);
        if (it != vm.current_environment.end()) {
            vm.push(it->second);
        } else {
            std::cerr << "Error: Closure variable '" << var_name << "' not found" << std::endl;
            vm.push(Value()); // Push default value
        }
    }

    // Jump table for opcodes (partial, expand as needed)
    static const std::array<OpHandler, 256> opcode_jump_table = []{
        std::array<OpHandler, 256> table{};
        table[static_cast<size_t>(OpCode::NOP)] = handle_NOP;
        table[static_cast<size_t>(OpCode::PUSH_INT)] = handle_PUSH_INT;
        table[static_cast<size_t>(OpCode::PUSH_FLOAT)] = handle_PUSH_FLOAT;
        table[static_cast<size_t>(OpCode::ADD)] = handle_ADD;
        table[static_cast<size_t>(OpCode::SUB)] = handle_SUB;
        table[static_cast<size_t>(OpCode::MUL)] = handle_MUL;
        table[static_cast<size_t>(OpCode::DIV)] = handle_DIV;
        table[static_cast<size_t>(OpCode::MOD)] = handle_MOD;
        table[static_cast<size_t>(OpCode::HASH)] = handle_HASH;
        table[static_cast<size_t>(OpCode::AMP)] = handle_AMP;
        table[static_cast<size_t>(OpCode::PIPE)] = handle_PIPE;
        table[static_cast<size_t>(OpCode::CARET)] = handle_CARET;
        table[static_cast<size_t>(OpCode::LT_LT)] = handle_LT_LT;
        table[static_cast<size_t>(OpCode::GT_GT)] = handle_GT_GT;
        table[static_cast<size_t>(OpCode::EQ)] = handle_EQ;
        table[static_cast<size_t>(OpCode::NEQ)] = handle_NEQ;
        table[static_cast<size_t>(OpCode::LT)] = handle_LT;
        table[static_cast<size_t>(OpCode::GT)] = handle_GT;
        table[static_cast<size_t>(OpCode::LTE)] = handle_LTE;
        table[static_cast<size_t>(OpCode::GTE)] = handle_GTE;
        table[static_cast<size_t>(OpCode::JMP)] = handle_JMP;
        table[static_cast<size_t>(OpCode::JMP_IF_FALSE)] = handle_JMP_IF_FALSE;
        table[static_cast<size_t>(OpCode::JMP_IF_TRUE)] = handle_JMP_IF_TRUE;
        table[static_cast<size_t>(OpCode::CALL)] = handle_CALL;
        table[static_cast<size_t>(OpCode::RET)] = handle_RET;
        table[static_cast<size_t>(OpCode::HALT)] = handle_HALT;
        table[static_cast<size_t>(OpCode::LOAD_VAR)] = handle_LOAD_VAR;
        table[static_cast<size_t>(OpCode::STORE_VAR)] = handle_STORE_VAR;
        table[static_cast<size_t>(OpCode::AND)] = handle_AND;
        table[static_cast<size_t>(OpCode::OR)] = handle_OR;
        table[static_cast<size_t>(OpCode::NOT)] = handle_NOT;
        table[static_cast<size_t>(OpCode::INPUT)] = handle_INPUT;
        table[static_cast<size_t>(OpCode::TYPEOF)] = handle_TYPEOF;
        table[static_cast<size_t>(OpCode::PRINT)] = handle_PRINT;
        table[static_cast<size_t>(OpCode::PUSH_ARRAY)] = handle_PUSH_ARRAY;
        table[static_cast<size_t>(OpCode::ARRAY_GET)] = handle_ARRAY_GET;
        table[static_cast<size_t>(OpCode::ARRAY_SET)] = handle_ARRAY_SET;
        table[static_cast<size_t>(OpCode::ARRAY_LEN)] = handle_ARRAY_LEN;
        table[static_cast<size_t>(OpCode::ARRAY_APPEND)] = handle_ARRAY_APPEND;
        table[static_cast<size_t>(OpCode::ARRAY_REMOVE)] = handle_ARRAY_REMOVE;
        table[static_cast<size_t>(OpCode::ARRAY_CLEAR)] = handle_ARRAY_CLEAR;
        table[static_cast<size_t>(OpCode::ARRAY_CLONE)] = handle_ARRAY_CLONE;
        table[static_cast<size_t>(OpCode::ARRAY_POP)] = handle_ARRAY_POP;
        table[static_cast<size_t>(OpCode::PUSH_MAP)] = handle_PUSH_MAP;
        table[static_cast<size_t>(OpCode::MAP_GET)] = handle_MAP_GET;
        table[static_cast<size_t>(OpCode::MAP_SET)] = handle_MAP_SET;
        table[static_cast<size_t>(OpCode::MAP_KEYS)] = handle_MAP_KEYS;
        table[static_cast<size_t>(OpCode::MAP_VALUES)] = handle_MAP_VALUES;
        table[static_cast<size_t>(OpCode::MAP_DELETE)] = handle_MAP_DELETE;
        table[static_cast<size_t>(OpCode::MAP_CLEAR)] = handle_MAP_CLEAR;
        table[static_cast<size_t>(OpCode::TRY)] = handle_TRY;
        table[static_cast<size_t>(OpCode::END_TRY)] = handle_END_TRY;
        table[static_cast<size_t>(OpCode::PRINT_MULTIPLE)] = handle_PRINT_MULTIPLE;
        table[static_cast<size_t>(OpCode::PRINTF)] = handle_PRINTF;
        table[static_cast<size_t>(OpCode::POP)] = handle_POP;
        table[static_cast<size_t>(OpCode::SWAP)] = handle_SWAP;
        table[static_cast<size_t>(OpCode::DUP)] = handle_DUP;
        table[static_cast<size_t>(OpCode::PUSH_UINT)] = handle_PUSH_UINT;
        table[static_cast<size_t>(OpCode::PUSH_STR)] = handle_PUSH_STR;
        table[static_cast<size_t>(OpCode::PUSH_BOOL)] = handle_PUSH_BOOL;
        table[static_cast<size_t>(OpCode::PUSH_FUNCTION)] = handle_PUSH_FUNCTION;
        table[static_cast<size_t>(OpCode::LOAD_PACKAGE_CONST)] = handle_LOAD_PACKAGE_CONST;
        table[static_cast<size_t>(OpCode::CALL_PACKAGE_FUNCTION)] = handle_CALL_PACKAGE_FUNCTION;
        table[static_cast<size_t>(OpCode::CREATE_CLOSURE)] = handle_CREATE_CLOSURE;
        table[static_cast<size_t>(OpCode::CAPTURE_VAR)] = handle_CAPTURE_VAR;
        table[static_cast<size_t>(OpCode::LOAD_CLOSURE_VAR)] = handle_LOAD_CLOSURE_VAR;
        return table;
    }();

    void LiVM::run(const BytecodeChunk &chunk)
    {

#ifdef _DEBUG
        std::cerr << "[DEBUG] VM::run() started with " << chunk.size() << " instructions" << std::endl;
#endif
        auto start_time = std::chrono::high_resolution_clock::now();
        
        ip = 0;
        std::vector<TryFrame> try_stack;
        
        // Reset performance counters
        instruction_count = 0;
        cache_hits = 0;
        cache_misses = 0;
        
        // Pre-optimize stack
        if (stack_optimization_enabled) {
            stack.reserve(STACK_RESERVE_SIZE);
        }

#ifdef _DEBUG
        // --- Print full instruction list for debugging ---
        std::cerr << "=== VM Bytecode Dump ===" << std::endl;
        for (size_t i = 0; i < chunk.size(); ++i)
        {
            const auto &instr = chunk[i];
            std::cerr << "[" << i << "] OpCode: " << static_cast<int>(instr.opcode)
                      << " (" << opcode_name(instr.opcode) << "), Operand: ";
            if (std::holds_alternative<int64_t>(instr.operand))
                std::cerr << std::get<int64_t>(instr.operand);
            else if (std::holds_alternative<double>(instr.operand))
                std::cerr << std::get<double>(instr.operand);
            else if (std::holds_alternative<std::string>(instr.operand))
                std::cerr << std::get<std::string>(instr.operand);
            else if (std::holds_alternative<bool>(instr.operand))
                std::cerr << (std::get<bool>(instr.operand) ? "true" : "false");
            else
                std::cerr << "(tuple/other)";
            std::cerr << std::endl;
        }
        std::cerr << "=== End Bytecode Dump ===" << std::endl;
#endif

        while (ip < chunk.size())
        {
            const auto &instr = chunk[ip];
            
#ifdef _DEBUG
            std::cerr << "[DEBUG] VM executing instruction " << ip << ": " << opcode_name(instr.opcode) << std::endl;
#endif
            
            // Performance tracking
            instruction_count++;
            track_hot_path(ip);
            
            // Try cached instruction first
            if (instruction_caching_enabled) {
#ifdef _DEBUG
                std::cerr << "[DEBUG] Trying cached instruction for ip=" << ip << std::endl;
#endif
                execute_cached_instruction(ip);
                ip++;
                continue;
            }
            
            try
            {
#ifdef _DEBUG
                // Debug: In thông tin opcode, operand, stack size
                std::cerr << "[DEBUG][ip=" << ip << "] OpCode: " << static_cast<int>(instr.opcode)
                          << " (" << opcode_name(instr.opcode) << ")"
                          << ", Operand index: ";
                if (std::holds_alternative<int64_t>(instr.operand))
                    std::cerr << std::get<int64_t>(instr.operand);
                else if (std::holds_alternative<double>(instr.operand))
                    std::cerr << std::get<double>(instr.operand);
                else if (std::holds_alternative<std::string>(instr.operand))
                    std::cerr << std::get<std::string>(instr.operand);
                else if (std::holds_alternative<bool>(instr.operand))
                    std::cerr << (std::get<bool>(instr.operand) ? "true" : "false");
                else
                    std::cerr << "(tuple/other)";
                std::cerr << ", Stack size: " << stack.size() << std::endl;
#endif
                switch (instr.opcode)
                {
#ifdef _DEBUG
                std::cerr << "[DEBUG] Entering switch case for opcode: " << opcode_name(instr.opcode) << std::endl;
#endif
                case OpCode::PUSH_INT:
                    push(Value(static_cast<int32_t>(std::get<int64_t>(instr.operand))));
#ifdef _DEBUG
                    std::cerr << "[DEBUG] PUSH_INT: stack size = " << stack.size() << ", top index = " << stack.back().index() << std::endl;
#endif
                    break;
                case OpCode::PUSH_UINT:
                    // Hỗ trợ uint32_t
                    push(Value(static_cast<uint32_t>(std::get<uint64_t>(instr.operand))));
                    break;
                case OpCode::PUSH_FLOAT:
                    // If you want to support float128, check here
                    // For now, always push float
                    push(Value(static_cast<float>(std::get<double>(instr.operand))));
                    break;
                case OpCode::PUSH_STR:
                    push(Value(std::get<std::string>(instr.operand)));
                    break;
                case OpCode::PUSH_BOOL:
                    push(Value(std::get<bool>(instr.operand)));
                    break;
                case OpCode::PUSH_FUNCTION:
#ifdef _DEBUG
                    std::cerr << "[DEBUG] PUSH_FUNCTION case reached" << std::endl;
#endif
                    handle_PUSH_FUNCTION(*this, instr, chunk, ip);
                    break;
                case OpCode::POP:
                    pop();
                    break;
                case OpCode::SWAP:
                    if (stack.size() < 2)
                    {
                        std::cerr << "VM stack underflow for SWAP" << std::endl;
                        break;
                    }
                    std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
                    break;
                case OpCode::ADD:
                case OpCode::SUB:
                case OpCode::MUL:
                case OpCode::DIV:
                case OpCode::MOD:
                case OpCode::HASH:
                case OpCode::AMP:
                case OpCode::PIPE:
                case OpCode::CARET:
                case OpCode::LT_LT:
                case OpCode::GT_GT:
                    Linh::math_binary_op(*this, instr);
                    break;
                case OpCode::AND:
                case OpCode::OR:
                {
                    auto b = pop();
                    auto a = pop();
                    if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b))
                    {
                        bool av = std::get<bool>(a);
                        bool bv = std::get<bool>(b);
                        if (instr.opcode == OpCode::AND)
                            push(av && bv);
                        else
                            push(av || bv);
                    }
                    else
                    {
                        std::cerr << "ERROR [Line: 0, Col: 0] RuntimeError: AND/OR only supports bool" << std::endl;
                        push(false);
                    }
                    break;
                }
                case OpCode::NOT:
                {
                    auto a = pop();
                    if (std::holds_alternative<bool>(a))
                        push(!std::get<bool>(a));
                    else if (std::holds_alternative<int32_t>(a))
                        push(~std::get<int32_t>(a)); // bitwise NOT
                    else {
                        std::cerr << "VM: NOT only supports bool or int" << std::endl;
                        push(false);
                    }
                    break;
                }
                case OpCode::EQ:
                case OpCode::NEQ:
                case OpCode::LT:
                case OpCode::GT:
                case OpCode::LTE:
                case OpCode::GTE:
                {
#ifdef _DEBUG
                    std::cerr << "[DEBUG] Before comparison: ";
                    debug_print_stack(stack);
#endif
                    auto b = pop();
                    auto a = pop();
#ifdef _DEBUG
                    std::cerr << "[DEBUG] Compare a="; debug_print_value(a); std::cerr << ", b="; debug_print_value(b); std::cerr << std::endl;
#endif
                    bool result = false;
                    // If either is string, compare as string
                                                                    if (std::holds_alternative<std::string>(a) || std::holds_alternative<std::string>(b)) {
                            std::string sa = std::holds_alternative<std::string>(a) ? std::get<std::string>(a) : Linh::to_str(a);
                            std::string sb = std::holds_alternative<std::string>(b) ? std::get<std::string>(b) : Linh::to_str(b);
                        switch (instr.opcode) {
                            case OpCode::EQ: result = (sa == sb); break;
                            case OpCode::NEQ: result = (sa != sb); break;
                            case OpCode::LT: result = (sa < sb); break;
                            case OpCode::GT: result = (sa > sb); break;
                            case OpCode::LTE: result = (sa <= sb); break;
                            case OpCode::GTE: result = (sa >= sb); break;
                            default: result = false; break;
                        }
                        push(result);
                        break;
                    }
                    // If both are bool
                    if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b)) {
                        bool av = std::get<bool>(a);
                        bool bv = std::get<bool>(b);
                        switch (instr.opcode) {
                            case OpCode::EQ: result = (av == bv); break;
                            case OpCode::NEQ: result = (av != bv); break;
                            case OpCode::LT: result = (!av && bv); break;
                            case OpCode::GT: result = (av && !bv); break;
                            case OpCode::LTE: result = (!av || bv); break;
                            case OpCode::GTE: result = (av || !bv); break;
                            default: result = false; break;
                        }
                        push(result);
                        break;
                    }
                    // If both are numbers (int/float/uint)
                    if ((std::holds_alternative<int32_t>(a) || std::holds_alternative<int64_t>(a) || std::holds_alternative<float>(a) || std::holds_alternative<uint32_t>(a) || std::holds_alternative<uint64_t>(a)) &&
                        (std::holds_alternative<int32_t>(b) || std::holds_alternative<int64_t>(b) || std::holds_alternative<float>(b) || std::holds_alternative<uint32_t>(b) || std::holds_alternative<uint64_t>(b))) {
                        long double av = 0.0;
                        long double bv = 0.0;
                        if (std::holds_alternative<int32_t>(a))        av = static_cast<long double>(std::get<int32_t>(a));
                        else if (std::holds_alternative<int64_t>(a))   av = static_cast<long double>(std::get<int64_t>(a));
                        else if (std::holds_alternative<uint32_t>(a))  av = static_cast<long double>(std::get<uint32_t>(a));
                        else if (std::holds_alternative<uint64_t>(a))  av = static_cast<long double>(std::get<uint64_t>(a));
                        else if (std::holds_alternative<float>(a))     av = static_cast<long double>(std::get<float>(a));
                        if (std::holds_alternative<int32_t>(b))        bv = static_cast<long double>(std::get<int32_t>(b));
                        else if (std::holds_alternative<int64_t>(b))   bv = static_cast<long double>(std::get<int64_t>(b));
                        else if (std::holds_alternative<uint32_t>(b))  bv = static_cast<long double>(std::get<uint32_t>(b));
                        else if (std::holds_alternative<uint64_t>(b))  bv = static_cast<long double>(std::get<uint64_t>(b));
                        else if (std::holds_alternative<float>(b))     bv = static_cast<long double>(std::get<float>(b));
                        switch (instr.opcode) {
                            case OpCode::EQ: result = (av == bv); break;
                            case OpCode::NEQ: result = (av != bv); break;
                            case OpCode::LT: result = (av < bv); break;
                            case OpCode::GT: result = (av > bv); break;
                            case OpCode::LTE: result = (av <= bv); break;
                            case OpCode::GTE: result = (av >= bv); break;
                            default: result = false; break;
                        }
                        push(result);
                        break;
                    }
                    // Fallback: compare as string
                    std::string sa = Linh::to_str(a);
                    std::string sb = Linh::to_str(b);
                    switch (instr.opcode) {
                        case OpCode::EQ: result = (sa == sb); break;
                        case OpCode::NEQ: result = (sa != sb); break;
                        case OpCode::LT: result = (sa < sb); break;
                        case OpCode::GT: result = (sa > sb); break;
                        case OpCode::LTE: result = (sa <= sb); break;
                        case OpCode::GTE: result = (sa >= sb); break;
                        default: result = false; break;
                    }
                    push(result);
                    break;
                }
                case OpCode::LOAD_VAR:
                {
                    int idx = static_cast<int>(std::get<int64_t>(instr.operand));
#ifdef _DEBUG
                    std::cerr << "[DEBUG] LOAD_VAR: loading variable index " << idx << std::endl;
                    std::cerr << "[DEBUG] LOAD_VAR: variables.size() = " << variables.size() << std::endl;
#endif
                    if (variables.count(idx)) {
                        auto value = variables[idx];
#ifdef _DEBUG
                        std::cerr << "[DEBUG] LOAD_VAR: loaded value index = " << value.index() << std::endl;
#endif
                        push(value);
#ifdef _DEBUG
                        std::cerr << "[DEBUG] LOAD_VAR: after push, stack size = " << stack.size() << ", top index = " << stack.back().index() << std::endl;
#endif
                    } else {
                        // Nếu tên biến là error.message và error tồn tại, trả về error
                        if (idx == 3 && variables.count(2)) {
                            push(variables[2]);
                        } else {
                            std::cerr << "VM: LOAD_VAR unknown variable index " << idx << std::endl;
                            push(int32_t(0));
                        }
                    }
                    break;
                }
                case OpCode::STORE_VAR:
                {
                    int idx = static_cast<int>(std::get<int64_t>(instr.operand));
                    if (stack.empty())
                    {
                        stack.push_back(std::monostate{});
                    }
                    Value value = pop();
                    variables[idx] = value;
                    
                    // Track variable in current environment for closure capture
                    // This is a simplified approach - in practice you'd have proper variable name tracking
                    // For now, we'll use the index as a simple identifier
                    std::string var_name = "var_" + std::to_string(idx);
                    current_environment[var_name] = value;
                    break;
                }
                case OpCode::PRINT:
                {
#ifdef _DEBUG
                    std::cerr << "[DEBUG] PRINT opcode executed" << std::endl;
#endif
                    if (stack.empty())
                    {
                        // Nếu stack rỗng, tự động push sol để không lỗi underflow
                        stack.push_back(std::monostate{});
                    }
                    auto val = pop();
#ifdef _DEBUG
                    std::cerr << "[DEBUG] PRINT: about to print: " << Linh::to_str(val) << std::endl;
                    std::cerr << "----- START PRINT -----" << std::endl;
#endif
                    LinhIO::linh_print(val);
#ifdef _DEBUG
                    std::cerr << "----- END PRINT -----" << std::endl;
#endif
                    break;
                }
                case OpCode::PRINT_MULTIPLE:
                {
#ifdef _DEBUG
                    std::cerr << "[DEBUG] PRINT_MULTIPLE in second switch case" << std::endl;
#endif
                    int32_t count = static_cast<int32_t>(std::get<int64_t>(instr.operand));
#ifdef _DEBUG
                    std::cerr << "[DEBUG] PRINT_MULTIPLE: count=" << count << ", stack_size=" << stack.size() << std::endl;
#endif
                    if (stack.size() < static_cast<size_t>(count))
                    {
                        std::cerr << "ERROR [Line " << instr.line << ", Col " << instr.col << "] RuntimeError : VM stack underflow for PRINT_MULTIPLE" << std::endl;
                        break;
                    }
                    // Pop all values and convert them to strings
                    std::vector<std::string> strings;
                    for (int i = 0; i < count; i++) {
                        auto val = pop();
#ifdef _DEBUG
                        std::cerr << "[DEBUG] PRINT_MULTIPLE: popped value " << i << ": " << Linh::to_str(val) << std::endl;
#endif
                        strings.push_back(Linh::to_str(val));
                    }
                    // Reverse the strings to get correct order
                    std::reverse(strings.begin(), strings.end());
                    // Join strings with space separator (like Python's print)
                    std::string result;
                    for (size_t i = 0; i < strings.size(); i++) {
                        if (i > 0) result += " ";
                        result += strings[i];
                    }
#ifdef _DEBUG
                    std::cerr << "[DEBUG] PRINT_MULTIPLE: final result: '" << result << "'" << std::endl;
                    std::cerr << "----- START PRINT -----" << std::endl;
#endif
                    LinhIO::linh_print(Value(result));
#ifdef _DEBUG
                    std::cerr << "----- END PRINT -----" << std::endl;
#endif
                    break;
                }
                case OpCode::PRINTF:
                {
                    if (stack.empty())
                    {
                        std::cerr << "ERROR [Line " << instr.line << ", Col " << instr.col << "] RuntimeError : VM stack underflow" << std::endl;
                        break;
                    }
                    auto val = pop();
#ifdef _DEBUG
                    std::cerr << "[DEBUG] PRINTF: about to print: " << Linh::to_str(val) << std::endl;
                    std::cerr << "----- START PRINTF -----" << std::endl;
#endif
                    LinhIO::linh_printf(val);
#ifdef _DEBUG
                    std::cerr << "----- END PRINTF -----" << std::endl;
#endif
                    break;
                }
                case OpCode::INPUT:
                {
                    auto prompt = pop();
                    std::string prompt_str;
                    if (std::holds_alternative<std::string>(prompt))
                        prompt_str = std::get<std::string>(prompt);
                    else
                        prompt_str = "";
#ifdef _DEBUG
                    std::cerr << "[DEBUG] INPUT: about to input: " << prompt_str << std::endl;
                    std::cerr << "----- START INPUT -----" << std::endl;
#endif
                        auto input_val = LinhIO::linh_input(prompt_str);
#ifdef _DEBUG
                    std::cerr << "----- END INPUT -----" << std::endl;
#endif
                    push(input_val);
                    break;
                }
                case OpCode::TYPEOF:
                {
                    if (stack.empty())
                    {
                        std::cerr << "ERROR [Line " << instr.line << ", Col " << instr.col << "] RuntimeError : VM stack underflow" << std::endl;
                        break;
                    }
                    auto val = pop();
                    std::string type_str = type_of(val);
                    push(type_str); // Đẩy lại kết quả lên stack để PRINT lấy ra
                    break;
                }
                case OpCode::HALT:
                    return;
                case OpCode::JMP:
                case OpCode::JMP_IF_FALSE:
                case OpCode::JMP_IF_TRUE:
                    handle_loop_opcode(*this, instr, chunk, ip);
                    continue;
                case OpCode::CALL:
                {
                    // Get function name
                    std::string fname = std::get<std::string>(instr.operand);
                    // --- Thêm hỗ trợ hàm pow ---
                    if (fname == "pow")
                    {
                        auto b = pop();
                        auto a = pop();
                        double av = 0, bv = 0;
                        if (std::holds_alternative<int64_t>(a))
                            av = static_cast<double>(std::get<int64_t>(a));
                        else if (std::holds_alternative<double>(a))
                            av = std::get<double>(a);
                        else
                            av = 0;
                        if (std::holds_alternative<int64_t>(b))
                            bv = static_cast<double>(std::get<int64_t>(b));
                        else if (std::holds_alternative<double>(b))
                            bv = std::get<double>(b);
                        else
                            bv = 0;
                        push(std::pow(av, bv));
                        break;
                    }
                    // --- Built-in conversion functions ---
                    if (fname == "sol")
                    {
                        if (!stack.empty()) pop();
                        push(std::monostate{});
                        break;
                    }
                    if (fname == "str")
                    {
                        auto val = pop();
                        push(Linh::to_str(val));
                        break;
                    }
                    if (fname == "bin")
                    {
                        auto val = pop();
                        // Convert numeric types to binary string with 0b prefix
                        auto to_bin = [](uint64_t v) {
                            if (v == 0) return std::string("0");
                            std::string s;
                            while (v > 0) {
                                s.push_back((v & 1ull) ? '1' : '0');
                                v >>= 1ull;
                            }
                            std::reverse(s.begin(), s.end());
                            return s;
                        };
                        bool is_neg = false;
                        std::string bits;
                        if (std::holds_alternative<uint64_t>(val)) {
                            bits = to_bin(std::get<uint64_t>(val));
                        } else if (std::holds_alternative<int64_t>(val)) {
                            int64_t iv = std::get<int64_t>(val);
                            if (iv < 0) {
                                is_neg = true;
                                bits = to_bin(static_cast<uint64_t>(-iv));
                            } else {
                                bits = to_bin(static_cast<uint64_t>(iv));
                            }
                        } else if (std::holds_alternative<double>(val)) {
                            // For float, take integer part
                            double dv = std::get<double>(val);
                            if (dv < 0) {
                                is_neg = true;
                                bits = to_bin(static_cast<uint64_t>(-static_cast<int64_t>(dv)));
                            } else {
                                bits = to_bin(static_cast<uint64_t>(dv));
                            }
                        } else if (std::holds_alternative<bool>(val)) {
                            bits = std::get<bool>(val) ? "1" : "0";
                        } else if (std::holds_alternative<std::string>(val)) {
                            // If input is string of decimal digits (optionally with leading -), parse; otherwise return empty
                            try {
                                std::string s = std::get<std::string>(val);
                                if (!s.empty() && s[0] == '-') { is_neg = true; s = s.substr(1); }
                                uint64_t uv = std::stoull(s);
                                bits = to_bin(uv);
                            } catch (...) {
                                bits = ""; // invalid input
                            }
                        } else if (std::holds_alternative<Byte>(val)) {
                            bits = to_bin(static_cast<uint64_t>(std::get<Byte>(val)));
                        }
                        std::string out = (is_neg ? "-0b" : "0b") + bits;
                        push(out);
                        break;
                    }
                    if (fname == "uint")
                    {
                        auto val = pop();
                        push(static_cast<uint64_t>(Linh::to_uint(val)));
                        break;
                    }
                    if (fname == "float")
                    {
                        auto val = pop();
                        push(Linh::to_float(val));
                        break;
                    }
                    if (fname == "int")
                    {
                        auto val = pop();
                        push(Linh::to_int(val));
                        break;
                    }
                    if (fname == "bool")
                    {
                        auto val = pop();
                        push(Linh::to_bool(val));
                        break;
                    }
                    if (fname == "len")
                    {
                        auto val = pop();
                        int64_t result = Linh::len(val);
                        push(result);
                        break;
                    }
                    // --- explicit typed conversions for declared types ---
                    if (fname == "to_int8")
                    {
                        auto val = pop();
                        push(static_cast<int8_t>(Linh::to_int8(val)));
                        break;
                    }
                    if (fname == "to_int16")
                    {
                        auto val = pop();
                        push(static_cast<int16_t>(Linh::to_int16(val)));
                        break;
                    }
                    if (fname == "to_int32")
                    {
                        auto val = pop();
                        push(static_cast<int32_t>(Linh::to_int32(val)));
                        break;
                    }
                    if (fname == "to_int64")
                    {
                        auto val = pop();
                        push(static_cast<int64_t>(Linh::to_int64(val)));
                        break;
                    }
                    if (fname == "to_uint8")
                    {
                        auto val = pop();
                        push(static_cast<uint8_t>(Linh::to_uint8(val)));
                        break;
                    }
                    if (fname == "to_uint16")
                    {
                        auto val = pop();
                        push(static_cast<uint16_t>(Linh::to_uint16(val)));
                        break;
                    }
                    if (fname == "to_uint32")
                    {
                        auto val = pop();
                        push(static_cast<uint32_t>(Linh::to_uint32(val)));
                        break;
                    }
                    if (fname == "to_uint64")
                    {
                        auto val = pop();
                        push(static_cast<uint64_t>(Linh::to_uint64(val)));
                        break;
                    }
                    if (fname == "to_float32")
                    {
                        auto val = pop();
                        push(Linh::to_float32(val));
                        break;
                    }
                    if (fname == "to_float64")
                    {
                        auto val = pop();
                        push(Linh::to_float64(val));
                        break;
                    }
                    if (fname == "to_bool")
                    {
                        auto val = pop();
                        push(Linh::to_bool(val));
                        break;
                    }
                    if (fname == "to_string")
                    {
                        auto val = pop();
                        // Use to_str, since to_string helper isn't declared in the header
                        push(Linh::to_str(val));
                        break;
                    }
                    // --- bytes([...]) ---
                    if (fname == "bytes")
                    {
                        if (stack.empty())
                        {
                            std::cerr << "VM: bytes() requires 1 argument" << std::endl;
                            push(Value{});
                            break;
                        }
                        auto arg = pop();
                        try {
                            if (std::holds_alternative<Array>(arg)) {
                                auto in_arr = std::get<Array>(arg);
                                auto bArr = make_array();
                                bArr->reserve(in_arr->size());
                                for (const auto &v : *in_arr) {
                                    uint64_t iv = 0;
                                    if (std::holds_alternative<int64_t>(v)) iv = static_cast<uint64_t>(std::get<int64_t>(v));
                                    else if (std::holds_alternative<uint64_t>(v)) iv = std::get<uint64_t>(v);
                                    else if (std::holds_alternative<double>(v)) iv = static_cast<uint64_t>(std::get<double>(v));
                                    else if (std::holds_alternative<Byte>(v)) iv = std::get<Byte>(v);
                                    else if (std::holds_alternative<bool>(v)) iv = std::get<bool>(v) ? 1u : 0u;
                                    // Clamp to 0..255
                                    if (iv > 255) iv = 255;
                                    bArr->push_back(Value(static_cast<Byte>(iv & 0xFF)));
                                }
                                push(Value(bArr));
                            } else if (std::holds_alternative<std::string>(arg)) {
                                auto b = Linh::string_bytes(arg, "utf-8");
                                push(Value(b));
                            } else {
                                std::cerr << "VM: bytes() expects array or string" << std::endl;
                                push(Value{});
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "VM: bytes() error: " << e.what() << std::endl;
                            push(Value{});
                        }
                        break;
                    }
                    // --- string.bytes(encoding) ---
                    if (fname == "str.bytes" || fname == "string.bytes")
                    {
                        // Nếu có 2 arg: string, encoding; 1 arg: string (utf-8)
                        std::string encoding = "utf-8";
                        Value sval;
                        if (stack.size() >= 2) {
                            auto enc_val = pop();
                            sval = pop();
                            if (std::holds_alternative<std::string>(enc_val))
                                encoding = std::get<std::string>(enc_val);
                        } else if (!stack.empty()) {
                            sval = pop();
                        } else {
                            push(Value{}); // sol
                            break;
                        }
                        try {
                            auto barr = Linh::string_bytes(sval, encoding);
                            push(Value(barr));
                        } catch (const std::exception& e) {
                            std::cerr << "VM: string.bytes error: " << e.what() << std::endl;
                            push(Value{}); // sol
                        }
                        break;
                    }
                    // --- Math functions support ---
                    if (fname == "pow")
                    {
                        // pow requires 2 arguments
                        if (stack.size() < 2)
                        {
                            std::cerr << "VM: Math function 'pow' requires 2 arguments\n";
                            push(Value{}); // Return sol
                            break;
                        }
                        auto exponent = pop();
                        auto base = pop();
                        double basev = 0, exponentv = 0;
                        
                        if (std::holds_alternative<int64_t>(base))
                            basev = static_cast<double>(std::get<int64_t>(base));
                        else if (std::holds_alternative<double>(base))
                            basev = std::get<double>(base);
                        else
                            basev = 0;
                            
                        if (std::holds_alternative<int64_t>(exponent))
                            exponentv = static_cast<double>(std::get<int64_t>(exponent));
                        else if (std::holds_alternative<double>(exponent))
                            exponentv = std::get<double>(exponent);
                        else
                            exponentv = 0;
                            
                        push(std::pow(basev, exponentv));
                        break;
                    }
                    else if (fname == "atan2")
                    {
                        // atan2 requires 2 arguments
                        if (stack.size() < 2)
                        {
                            std::cerr << "VM: Math function 'atan2' requires 2 arguments\n";
                            push(Value{}); // Return sol
                            break;
                        }
                        auto y = pop();
                        auto x = pop();
                        double xv = 0, yv = 0;
                        
                        if (std::holds_alternative<int64_t>(x))
                            xv = static_cast<double>(std::get<int64_t>(x));
                        else if (std::holds_alternative<double>(x))
                            xv = std::get<double>(x);
                        else
                            xv = 0;
                            
                        if (std::holds_alternative<int64_t>(y))
                            yv = static_cast<double>(std::get<int64_t>(y));
                        else if (std::holds_alternative<double>(y))
                            yv = std::get<double>(y);
                        else
                            yv = 0;
                            
                        push(std::atan2(yv, xv));
                        break;
                    }
                    
                    auto math_func = Linh::Std::get_math_function(fname);
                    if (math_func)
                    {
                        if (stack.empty())
                        {
                            std::cerr << "VM: Math function '" << fname << "' requires an argument\n";
                            push(Value{}); // Return sol
                            break;
                        }
                        auto val = pop();
                        auto result = math_func(val);
                        push(result);
                        break;
                    }
                    
                    auto time_func = Linh::Std::get_time_function(fname);
                    if (time_func)
                    {
                        if (stack.empty())
                        {
                            std::cerr << "VM: Time function '" << fname << "' requires an argument\n";
                            push(Value{}); // Return sol
                            break;
                        }
                        auto val = pop();
                        auto result = time_func(val);
                        push(result);
                        break;
                    }
                    
                    // Package function calls (e.g., time.sleep, math.abs)
                    size_t dot_pos = fname.find('.');
                    if (dot_pos != std::string::npos) {
                        std::string package_name = fname.substr(0, dot_pos);
                        std::string function_name = fname.substr(dot_pos + 1);
                        
                        if (package_name == "time") {
                            auto time_func = Linh::Std::get_time_function(function_name);
                            if (time_func) {
                                Value val;
                                if (!stack.empty()) {
                                    val = pop();
                                } else {
                                    // Cho phép các hàm không có argument (như process_time)
                                    val = Value{};
                                }
                                auto result = time_func(val);
                                push(result);
                                break;
                            }
                        } else if (package_name == "math") {
                            auto math_func = Linh::Std::get_math_function(function_name);
                            if (math_func) {
                                Value val;
                                if (!stack.empty()) {
                                    val = pop();
                                } else {
                                    // Cho phép các hàm không có argument
                                    val = Value{};
                                }
                                auto result = math_func(val);
                                push(result);
                                break;
                            }
                        }
                    }
                    // Kiểm tra xem có function object trên stack không
#ifdef _DEBUG
                    std::cerr << "[DEBUG] CALL: checking for function object on stack, size=" << stack.size() << std::endl;
                    if (!stack.empty()) {
                        std::cerr << "[DEBUG] CALL: top of stack index = " << stack.back().index() << std::endl;
                    }
#endif
                    if (!stack.empty() && stack.back().index() == 15) { // FunctionPtr is at index 15 in Value
#ifdef _DEBUG
                        std::cerr << "[DEBUG] CALL: found function object, calling it" << std::endl;
#endif
                        // Gọi function object
                        auto fn = std::get<15>(stack.back());
                        pop(); // Pop function object
                        
                        // Thu thập arguments từ stack (arguments được push theo thứ tự ngược)
                        std::vector<Value> args;
                        size_t expected_args = fn->params.size();
                        
                        // Arguments được push theo thứ tự từ trái sang phải, nên trên stack sẽ ngược lại
                        for (size_t i = 0; i < expected_args; ++i) {
                            if (stack.empty()) {
                                std::cerr << "Error: Not enough arguments for function " << fn->name << std::endl;
                                push(Value()); // Push default value
                                break;
                            }
                            args.insert(args.begin(), pop()); // Insert at beginning để giữ thứ tự đúng
                        }
                        
#ifdef _DEBUG
                        std::cerr << "[DEBUG] CALL: calling function " << fn->name << " with " << args.size() << " arguments" << std::endl;
#endif
                        
                        // Gọi function object
                        auto result = call_function(fn, args, *this);
                        push(result);
#ifdef _DEBUG
                        std::cerr << "[DEBUG] CALL: function executed, result pushed to stack, ip=" << ip << std::endl;
#endif
                        break;
                    }
                    
                    if (!functions.count(fname))
                    {
                        std::cerr << "VM: Unknown function '" << fname << "'\n";
                        break;
                    }
                    const auto &fn = functions[fname];
                    // Pop arguments in reverse order
                    std::vector<Value> args;
                    for (size_t i = 0; i < fn.param_names.size(); ++i)
                        args.push_back(pop());
                    std::reverse(args.begin(), args.end());
                    // Save current frame
                    CallFrame frame;
                    frame.return_ip = ip + 1;
                    frame.locals = variables;
                    call_stack.push_back(frame);
                    // Set up new variables for function
                    variables.clear();
                    for (size_t i = 0; i < fn.param_names.size(); ++i)
                        variables[i] = args[i];
                    // Run function code
                    size_t saved_ip = ip;
                    size_t fn_ip = 0;
                    while (fn_ip < fn.code.size())
                    {
                        const auto &finstr = fn.code[fn_ip];
                        switch (finstr.opcode)
                        {
                        case OpCode::PUSH_INT:
                            push(Value(std::get<int64_t>(finstr.operand)));
                            break;
                        case OpCode::PUSH_UINT:
                            push(Value(std::get<uint64_t>(finstr.operand)));
                            break;
                        case OpCode::PUSH_FLOAT:
                            push(Value(std::get<double>(finstr.operand)));
                            break;
                        case OpCode::PUSH_STR:
                            push(Value(std::get<std::string>(finstr.operand)));
                            break;
                        case OpCode::PUSH_BOOL:
                            push(Value(std::get<bool>(finstr.operand)));
                            break;
                        case OpCode::POP:
                            pop();
                            break;
                        case OpCode::SWAP:
                            if (stack.size() < 2)
                            {
                                std::cerr << "VM stack underflow for SWAP" << std::endl;
                                break;
                            }
                            std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
                            break;
                        case OpCode::ADD:
                        case OpCode::SUB:
                        case OpCode::MUL:
                        case OpCode::DIV:
                        case OpCode::MOD:
                        case OpCode::HASH:  // # (floor division)
                        case OpCode::AMP:   // & (bitwise and)
                        case OpCode::PIPE:  // | (bitwise or)
                        case OpCode::CARET: // ^ (bitwise xor)
                        case OpCode::LT_LT: // << (bitwise shift left)
                        case OpCode::GT_GT: // >> (bitwise shift right)
                        {
#ifdef _DEBUG
                            // Debug: In stack trước khi pop
                            std::cerr << "[DEBUG] Stack before pop (size=" << stack.size() << "): ";
                            for (const auto &v : stack)
                            {
                                if (std::holds_alternative<int64_t>(v))
                                    std::cerr << std::get<int64_t>(v) << " ";
                                else if (std::holds_alternative<double>(v))
                                    std::cerr << std::get<double>(v) << " ";
                                else if (std::holds_alternative<std::string>(v))
                                    std::cerr << "\"" << std::get<std::string>(v) << "\" ";
                                else if (std::holds_alternative<bool>(v))
                                    std::cerr << (std::get<bool>(v) ? "true" : "false") << " ";
                                else
                                    std::cerr << "(?) ";
                            }
                            std::cerr << std::endl;
#endif
                            auto b = pop();
                            auto a = pop();
#ifdef _DEBUG
                            // Debug: In giá trị a, b
                            std::cerr << "[DEBUG] a=";
                            if (std::holds_alternative<int64_t>(a))
                                std::cerr << std::get<int64_t>(a);
                            else if (std::holds_alternative<double>(a))
                                std::cerr << std::get<double>(a);
                            else if (std::holds_alternative<std::string>(a))
                                std::cerr << "\"" << std::get<std::string>(a) << "\"";
                            else if (std::holds_alternative<bool>(a))
                                std::cerr << (std::get<bool>(a) ? "true" : "false");
                            else
                                std::cerr << "(?)";
                            std::cerr << ", b=";
                            if (std::holds_alternative<int64_t>(b))
                                std::cerr << std::get<int64_t>(b);
                            else if (std::holds_alternative<double>(b))
                                std::cerr << std::get<double>(b);
                            else if (std::holds_alternative<std::string>(b))
                                std::cerr << "\"" << std::get<std::string>(b) << "\"";
                            else if (std::holds_alternative<bool>(b))
                                std::cerr << (std::get<bool>(b) ? "true" : "false");
                            else
                                std::cerr << "(?)";
                            std::cerr << std::endl;
#endif
                            // --- HỖ TRỢ NỐI CHUỖI ---
                            if (finstr.opcode == OpCode::ADD &&
                                (std::holds_alternative<std::string>(a) || std::holds_alternative<std::string>(b)))
                            {
                                std::string sa, sb;
                                // Chuyển a về string
                                if (std::holds_alternative<std::string>(a))
                                    sa = std::get<std::string>(a);
                                else if (std::holds_alternative<int64_t>(a))
                                    sa = std::to_string(std::get<int64_t>(a));
                                else if (std::holds_alternative<double>(a))
                                    sa = std::to_string(std::get<double>(a));
                                else if (std::holds_alternative<bool>(a))
                                    sa = std::get<bool>(a) ? "true" : "false";
                                // Chuyển b về string
                                if (std::holds_alternative<std::string>(b))
                                    sb = std::get<std::string>(b);
                                else if (std::holds_alternative<int64_t>(b))
                                    sb = std::to_string(std::get<int64_t>(b));
                                else if (std::holds_alternative<double>(b))
                                    sb = std::to_string(std::get<double>(b));
                                else if (std::holds_alternative<bool>(b))
                                    sb = std::get<bool>(b) ? "true" : "false";
                                push(sa + sb);
                                break;
                            }
                            // --- KẾT THÚC HỖ TRỢ NỐI CHUỖI ---
                            if (std::holds_alternative<int64_t>(a) && std::holds_alternative<int64_t>(b))
                            {
                                int64_t av = std::get<int64_t>(a);
                                int64_t bv = std::get<int64_t>(b);
                                switch (finstr.opcode)
                                {
                                case OpCode::ADD:
                                    push(av + bv);
                                    break;
                                case OpCode::SUB:
                                    push(av - bv);
                                    break;
                                case OpCode::MUL:
                                    push(av * bv);
                                    break;
                                case OpCode::DIV:
                                case OpCode::MOD:
                                    if ((finstr.opcode == OpCode::DIV || finstr.opcode == OpCode::MOD) && bv == 0)
                                    {
                                        std::string err_msg = "Division by zero (int)";
#ifdef _DEBUG
                                        std::cerr << "[DEBUG] Division by zero detected. try_stack size: " << try_stack.size() << std::endl;
                                        if (!try_stack.empty()) {
                                            std::cerr << "[DEBUG] catch_ip: " << try_stack.back().catch_ip << ", current ip: " << ip << std::endl;
                                        }
#endif
                                        if (!try_stack.empty())
                                        {
                                            // Get the error variable index from the try frame
                                            int error_var_index = 1; // Default to 1 for "error"
                                            if (!try_stack.empty() && try_stack.back().error_var == "error")
                                            {
                                                error_var_index = 1; // "error" variable is at index 1
                                            }
                                            variables[error_var_index] = err_msg;
                                            ip = try_stack.back().catch_ip;
#ifdef _DEBUG
                                            std::cerr << "[DEBUG] Jumping to catch block at ip: " << ip << std::endl;
#endif
                                            continue;
                                        }
                                        else
                                        {
                                            std::cerr << "ERROR [Line: " << instr.line << ", Col: " << instr.col << "] VM: " << err_msg << std::endl;
                                            return;
                                        }
                                    }
                                    if (finstr.opcode == OpCode::DIV)
                                        push(av / bv);
                                    else
                                        push(av % bv);
                                    break;
                                case OpCode::HASH:
                                    if (bv == 0)
                                    {
                                        std::string err_msg = "Floor division by zero (int)";
                                        if (!try_stack.empty())
                                        {
                                            // Get the error variable index from the try frame
                                            int error_var_index = 1; // Default to 1 for "error"
                                            if (!try_stack.empty() && try_stack.back().error_var == "error")
                                            {
                                                error_var_index = 1; // "error" variable is at index 1
                                            }
                                            variables[error_var_index] = err_msg;
                                            ip = try_stack.back().catch_ip;
                                            continue;
                                        }
                                        else
                                        {
                                            std::cerr << "ERROR [Line: " << instr.line << ", Col: " << instr.col << "] VM: " << err_msg << std::endl;
                                            return;
                                        }
                                    }
                                    // Python-like floor division for int
                                    if ((av < 0) != (bv < 0) && av % bv != 0)
                                        push((av / bv) - 1);
                                    else
                                        push(av / bv);
                                    break;
                                case OpCode::AMP:
                                    push(av & bv);
                                    break;
                                case OpCode::PIPE:
                                    push(av | bv);
                                    break;
                                case OpCode::CARET:
                                    push(av ^ bv);
                                    break;
                                case OpCode::LT_LT:
                                    push(av << bv);
                                    break;
                                case OpCode::GT_GT:
                                    push(av >> bv);
                                    break;
                                default:
                                    break;
                                }
                            }
                            else if ((std::holds_alternative<int64_t>(a) || std::holds_alternative<double>(a)) &&
                                     (std::holds_alternative<int64_t>(b) || std::holds_alternative<double>(b)))
                            {
                                double av = std::holds_alternative<int64_t>(a) ? static_cast<double>(std::get<int64_t>(a)) : std::get<double>(a);
                                double bv = std::holds_alternative<int64_t>(b) ? static_cast<double>(std::get<int64_t>(b)) : std::get<double>(b);
                                switch (finstr.opcode)
                                {
                                case OpCode::ADD:
                                    push(av + bv);
                                    break;
                                case OpCode::SUB:
                                    push(av - bv);
                                    break;
                                case OpCode::MUL:
                                    push(av * bv);
                                    break;
                                case OpCode::DIV:
                                    if (bv == 0.0)
                                    {
                                        std::string err_msg = "Division by zero (float)";
                                        if (!try_stack.empty())
                                        {
                                            // Get the error variable index from the try frame
                                            int error_var_index = 1; // Default to 1 for "error"
                                            if (!try_stack.empty() && try_stack.back().error_var == "error")
                                            {
                                                error_var_index = 1; // "error" variable is at index 1
                                            }
                                            variables[error_var_index] = err_msg;
                                            ip = try_stack.back().catch_ip;
                                            continue;
                                        }
                                        else
                                        {
                                            std::cerr << "ERROR [Line: " << instr.line << ", Col: " << instr.col << "] VM: " << err_msg << std::endl;
                                            return;
                                        }
                                    }
                                    else
                                    {
                                        push(av / bv);
                                    }
                                    break;
                                case OpCode::MOD:
                                    if (bv == 0.0)
                                    {
                                        std::string err_msg = "Modulo by zero (float)";
                                        if (!try_stack.empty())
                                        {
                                            // Get the error variable index from the try frame
                                            int error_var_index = 1; // Default to 1 for "error"
                                            if (!try_stack.empty() && try_stack.back().error_var == "error")
                                            {
                                                error_var_index = 1; // "error" variable is at index 1
                                            }
                                            variables[error_var_index] = err_msg;
                                            ip = try_stack.back().catch_ip;
                                            continue;
                                        }
                                        else
                                        {
                                            std::cerr << "ERROR [Line: " << instr.line << ", Col: " << instr.col << "] VM: " << err_msg << std::endl;
                                            return;
                                        }
                                    }
                                    else
                                    {
                                        push(std::fmod(av, bv));
                                    }
                                    break;
                                case OpCode::HASH:
                                    if (bv == 0.0)
                                    {
                                        std::string err_msg = "Floor division by zero (float)";
                                        if (!try_stack.empty())
                                        {
                                            // Get the error variable index from the try frame
                                            int error_var_index = 1; // Default to 1 for "error"
                                            if (!try_stack.empty() && try_stack.back().error_var == "error")
                                            {
                                                error_var_index = 1; // "error" variable is at index 1
                                            }
                                            variables[error_var_index] = err_msg;
                                            ip = try_stack.back().catch_ip;
                                            continue;
                                        }
                                        else
                                        {
                                            std::cerr << "ERROR [Line: " << instr.line << ", Col: " << instr.col << "] VM: " << err_msg << std::endl;
                                            return;
                                        }
                                    }
                                    else
                                    {
                                        push(std::floor(av / bv));
                                    }
                                    break;
                                default:
                                    break;
                                }
                            }
                            else
                            {
                                std::cerr << "VM: ADD/SUB/MUL/DIV/MOD only supports int/float" << std::endl;
                            }
                            break;
                        }
                        case OpCode::AND:
                        case OpCode::OR:
                        {
                            auto b = pop();
                            auto a = pop();
                            if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b))
                            {
                                bool av = std::get<bool>(a);
                                bool bv = std::get<bool>(b);
                                if (finstr.opcode == OpCode::AND)
                                    push(av && bv);
                                else
                                    push(av || bv);
                            }
                            else
                            {
                                std::cerr << "VM: AND/OR only supports bool" << std::endl;
                                push(false);
                            }
                            break;
                        }
                        case OpCode::NOT:
                        {
                            auto a = pop();
                            if (std::holds_alternative<bool>(a))
                                push(!std::get<bool>(a));
                            else if (std::holds_alternative<int64_t>(a))
                                push(~std::get<int64_t>(a)); // bitwise NOT
                            else {
                                std::cerr << "VM: NOT only supports bool or int" << std::endl;
                                push(false);
                            }
                            break;
                        }
                        case OpCode::EQ:
                        case OpCode::NEQ:
                        case OpCode::LT:
                        case OpCode::GT:
                        case OpCode::LTE:
                        case OpCode::GTE:
                        {
#ifdef _DEBUG
                            // Debug: In stack trước khi pop
                            std::cerr << "[DEBUG] Stack before pop (size=" << stack.size() << "): ";
                            for (const auto &v : stack)
                            {
                                if (std::holds_alternative<int64_t>(v))
                                    std::cerr << std::get<int64_t>(v) << " ";
                                else if (std::holds_alternative<double>(v))
                                    std::cerr << std::get<double>(v) << " ";
                                else if (std::holds_alternative<std::string>(v))
                                    std::cerr << "\"" << std::get<std::string>(v) << "\" ";
                                else if (std::holds_alternative<bool>(v))
                                    std::cerr << (std::get<bool>(v) ? "true" : "false") << " ";
                                else
                                    std::cerr << "(?) ";
                            }
                            std::cerr << std::endl;
#endif
                            auto b = pop();
                            auto a = pop();
#ifdef _DEBUG
                            // Debug: In giá trị a, b
                            std::cerr << "[DEBUG] a=";
                            if (std::holds_alternative<int64_t>(a))
                                std::cerr << std::get<int64_t>(a);
                            else if (std::holds_alternative<double>(a))
                                std::cerr << std::get<double>(a);
                            else if (std::holds_alternative<std::string>(a))
                                std::cerr << "\"" << std::get<std::string>(a) << "\"";
                            else if (std::holds_alternative<bool>(a))
                                std::cerr << (std::get<bool>(a) ? "true" : "false");
                            else
                                std::cerr << "(?)";
                            std::cerr << ", b=";
                            if (std::holds_alternative<int64_t>(b))
                                std::cerr << std::get<int64_t>(b);
                            else if (std::holds_alternative<double>(b))
                                std::cerr << std::get<double>(b);
                            else if (std::holds_alternative<std::string>(b))
                                std::cerr << "\"" << std::get<std::string>(b) << "\"";
                            else if (std::holds_alternative<bool>(b))
                                std::cerr << (std::get<bool>(b) ? "true" : "false");
                            else
                                std::cerr << "(?)";
                            std::cerr << std::endl;
#endif

                            bool result = false;
                            // So sánh chuỗi nếu một trong hai là string
                            if (std::holds_alternative<std::string>(a) || std::holds_alternative<std::string>(b))
                            {
                                std::string sa, sb;
                                if (std::holds_alternative<std::string>(a))
                                    sa = std::get<std::string>(a);
                                else if (std::holds_alternative<int64_t>(a))
                                    sa = std::to_string(std::get<int64_t>(a));
                                else if (std::holds_alternative<double>(a))
                                    sa = std::to_string(std::get<double>(a));
                                else if (std::holds_alternative<bool>(a))
                                    sa = std::get<bool>(a) ? "true" : "false";
                                if (std::holds_alternative<std::string>(b))
                                    sb = std::get<std::string>(b);
                                else if (std::holds_alternative<int64_t>(b))
                                    sb = std::to_string(std::get<int64_t>(b));
                                else if (std::holds_alternative<double>(b))
                                    sb = std::to_string(std::get<double>(b));
                                else if (std::holds_alternative<bool>(b))
                                    sb = std::get<bool>(b) ? "true" : "false";
                                // Do NOT push(sa + sb) for comparison ops!
                                // Instead, compare as strings:
                                switch (instr.opcode)
                                {
                                case OpCode::EQ:
                                    result = (sa == sb);
                                    break;
                                case OpCode::NEQ:
                                    result = (sa != sb);
                                    break;
                                case OpCode::LT:
                                    result = (sa < sb);
                                    break;
                                case OpCode::GT:
                                    result = (sa > sb);
                                    break;
                                case OpCode::LTE:
                                    result = (sa <= sb);
                                    break;
                                case OpCode::GTE:
                                    result = (sa >= sb);
                                    break;
                                default:
                                    result = false;
                                    break;
                                }
                                push(result);
                                break;
                            }
                            // So sánh bool nếu cả hai là bool
                            if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b))
                            {
                                bool av = std::get<bool>(a);
                                bool bv = std::get<bool>(b);
                                switch (instr.opcode)
                                {
                                case OpCode::EQ:
                                    result = (av == bv);
                                    break;
                                case OpCode::NEQ:
                                    result = (av != bv);
                                    break;
                                case OpCode::LT:
                                    result = (!av && bv);
                                    break;
                                case OpCode::GT:
                                    result = (av && !bv);
                                    break;
                                case OpCode::LTE:
                                    result = (!av || bv);
                                    break;
                                case OpCode::GTE:
                                    result = (av || !bv);
                                    break;
                                default:
                                    result = false;
                                    break;
                                }
                                push(result);
                                break;
                            }
                            // So sánh số nếu cả hai là số
                            if ((std::holds_alternative<int64_t>(a) || std::holds_alternative<double>(a)) &&
                                (std::holds_alternative<int64_t>(b) || std::holds_alternative<double>(b)))
                            {
                                double av = std::holds_alternative<int64_t>(a) ? static_cast<double>(std::get<int64_t>(a)) : std::get<double>(a);
                                double bv = std::holds_alternative<int64_t>(b) ? static_cast<double>(std::get<int64_t>(b)) : std::get<double>(b);
                                switch (instr.opcode)
                                {
                                case OpCode::EQ:
                                    result = (av == bv);
                                    break;
                                case OpCode::NEQ:
                                    result = (av != bv);
                                    break;
                                case OpCode::LT:
                                    result = (av < bv);
                                    break;
                                case OpCode::GT:
                                    result = (av > bv);
                                    break;
                                case OpCode::LTE:
                                    result = (av <= bv);
                                    break;
                                case OpCode::GTE:
                                    result = (av >= bv);
                                    break;
                                default:
                                    result = false;
                                    break;
                                }
                                push(result);
                                break;
                            }
                            // Nếu kiểu không khớp, chuyển về chuỗi rồi so sánh
                            std::string sa, sb;
                            // a
                            if (std::holds_alternative<int64_t>(a))
                                sa = std::to_string(std::get<int64_t>(a));
                            else if (std::holds_alternative<double>(a))
                                sa = std::to_string(std::get<double>(a));
                            else if (std::holds_alternative<bool>(a))
                                sa = std::get<bool>(a) ? "true" : "false";
                            else if (std::holds_alternative<std::string>(a))
                                sa = std::get<std::string>(a);
                            else
                                sa = "";
                            // b
                            if (std::holds_alternative<int64_t>(b))
                                sb = std::to_string(std::get<int64_t>(b));
                            else if (std::holds_alternative<double>(b))
                                sb = std::to_string(std::get<double>(b));
                            else if (std::holds_alternative<bool>(b))
                                sb = std::get<bool>(b) ? "true" : "false";
                            else if (std::holds_alternative<std::string>(b))
                                sb = std::get<std::string>(b);
                            else
                                sb = "";
                            switch (instr.opcode)
                            {
                            case OpCode::EQ:
                                result = (sa == sb);
                                break;
                            case OpCode::NEQ:
                                result = (sa != sb);
                                break;
                            case OpCode::LT:
                                result = (sa < sb);
                                break;
                            case OpCode::GT:
                                result = (sa > sb);
                                break;
                            case OpCode::LTE:
                                result = (sa <= sb);
                                break;
                            case OpCode::GTE:
                                result = (sa >= sb);
                                break;
                            default:
                                result = false;
                                break;
                            }
                            push(result);
                            break;
                        }
                        case OpCode::LOAD_VAR:
                        {
                            int idx = std::get<int64_t>(finstr.operand);
#ifdef _DEBUG
                            std::cerr << "[DEBUG] LOAD_VAR idx=" << idx << ", variables.size()=" << variables.size() << std::endl;
#endif
                            if (variables.count(idx))
                                push(variables[idx]);
                            else
                            {
                                std::cerr << "VM: LOAD_VAR unknown variable index " << idx << std::endl;
                                push(int64_t(0));
                            }
                            break;
                        }
                        case OpCode::STORE_VAR:
                        {
                            int idx = std::get<int64_t>(finstr.operand);
                            if (stack.empty())
                            {
                                stack.push_back(std::monostate{});
                            }
                            Value value = pop();
                            variables[idx] = value;
                            
                            // Track variable in current environment for closure capture
                            // This is a simplified approach - in practice you'd have proper variable name tracking
                            // For now, we'll use the index as a simple identifier
                            std::string var_name = "var_" + std::to_string(idx);
                            current_environment[var_name] = value;
                            break;
                        }
                        case OpCode::PRINT:
                        {
                            auto val = pop();
                            LinhIO::linh_print(val);
                            break;
                        }
                        case OpCode::PRINT_MULTIPLE:
                        {
                            std::cerr << "[DEBUG] PRINT_MULTIPLE in second switch case" << std::endl;
                            int64_t count = std::get<int64_t>(finstr.operand);
                            std::cerr << "[DEBUG] PRINT_MULTIPLE: count=" << count << ", stack_size=" << stack.size() << std::endl;
                            if (stack.size() < static_cast<size_t>(count))
                            {
                                std::cerr << "ERROR [Line " << finstr.line << ", Col " << finstr.col << "] RuntimeError : VM stack underflow for PRINT_MULTIPLE" << std::endl;
                                break;
                            }
                            
                            // Pop all values and convert them to strings
                            std::vector<std::string> strings;
                            for (int i = 0; i < count; i++) {
                                auto val = pop();
                                std::cerr << "[DEBUG] PRINT_MULTIPLE: popped value " << i << ": " << Linh::to_str(val) << std::endl;
                                strings.push_back(Linh::to_str(val));
                            }
                            
                            // Reverse the strings to get correct order
                            std::reverse(strings.begin(), strings.end());
                            
                            // Join strings with space separator (like Python's print)
                            std::string result;
                            for (size_t i = 0; i < strings.size(); i++) {
                                if (i > 0) result += " ";
                                result += strings[i];
                            }
                            
                            std::cerr << "[DEBUG] PRINT_MULTIPLE: final result: '" << result << "'" << std::endl;
                            LinhIO::linh_print(Value(result));
                            break;
                        }
                        case OpCode::PRINTF:
                        {
                            auto val = pop();
                            LinhIO::linh_printf(val);
                            break;
                        }
                        case OpCode::INPUT:
                        {
                            auto prompt = pop();
                            std::string prompt_str;
                            if (std::holds_alternative<std::string>(prompt))
                                prompt_str = std::get<std::string>(prompt);
                            else
                                prompt_str = "";
                            auto input_val = LinhIO::linh_input(prompt_str);
                            push(input_val);
                            break;
                        }
                        case OpCode::TYPEOF:
                        {
                            auto val = pop();
                            std::cout << type_of(val) << std::endl;
                            break;
                        }
                        case OpCode::RET:
                            // --- Fix: Always push a value to the stack before returning ---
                            if (stack.empty())
                            {
                                stack.push_back(std::monostate{});
                            }
                            // Restore previous frame
                            if (!call_stack.empty())
                            {
                                variables = call_stack.back().locals;
                                ip = call_stack.back().return_ip;
                                call_stack.pop_back();
                            }
                            else
                            {
                                ip = chunk.size(); // End program
                            }
                            goto function_returned;
                        default:
                            break;
                        }
                        ++fn_ip;
                    }
                function_returned:
                    continue;
                }
                case OpCode::RET:
                    // --- Fix: Always push a value to the stack before returning from global code (should not happen, but for safety) ---
                    if (stack.empty())
                    {
                        stack.push_back(std::monostate{});
                    }
                    return;
                // --- Thêm xử lý TRY-CATCH-FINALLY ---
                case OpCode::TRY:
                {
                    // operand = tuple (catch_ip, finally_ip, end_ip, error_var)
                    const auto &info = std::get<std::tuple<int64_t, int64_t, int64_t, std::string>>(instr.operand);
                    try_stack.emplace_back(
                        static_cast<size_t>(std::get<0>(info)),
                        static_cast<size_t>(std::get<1>(info)),
                        static_cast<size_t>(std::get<2>(info)),
                        std::get<3>(info));
                    break;
                }
                case OpCode::END_TRY:
                {
                    if (!try_stack.empty())
                        try_stack.pop_back();
                    break;
                }
                case OpCode::DUP:
#ifdef _DEBUG
                    std::cerr << "[DEBUG] DUP stack size before: " << stack.size() << std::endl;
#endif
                    if (stack.empty())
                    {
                        std::cerr << "VM stack underflow for DUP" << std::endl;
                        break;
                    }
                    stack.push_back(stack.back());
#ifdef _DEBUG
                    std::cerr << "[DEBUG] DUP stack size after: " << stack.size() << std::endl;
#endif
                    break;
                case OpCode::PUSH_ARRAY:
                {
                    int64_t n = std::get<int64_t>(instr.operand);
                    if (n < 0 || (size_t)n > stack.size())
                    {
                        std::cerr << "VM: Invalid array size for PUSH_ARRAY: " << n << std::endl;
                        push(Value{}); // push uninit
                        break;
                    }
                    Array arr = make_array();
                    arr->reserve(n);
                    // Pop n phần tử (theo thứ tự ngược lại)
                    for (int64_t i = 0; i < n; ++i)
                    {
                        arr->push_back(pop());
                    }
                    // Đảo ngược lại để đúng thứ tự literal
                    std::reverse(arr->begin(), arr->end());
                    push(arr);
                    break;
                }
                case OpCode::PUSH_MAP:
                {
                    int64_t n = std::get<int64_t>(instr.operand);
                    if (n < 0 || (size_t)(2 * n) > stack.size())
                    {
                        std::cerr << "VM: Invalid map size for PUSH_MAP: " << n << std::endl;
                        push(Value{}); // push uninit
                        break;
                    }
                    Map map = make_map();
                    // Pop n cặp (value trước, key sau)
                    for (int64_t i = 0; i < n; ++i)
                    {
                        Value value = pop();
                        Value key = pop();
                        std::string key_str;
                        if (std::holds_alternative<std::string>(key))
                            key_str = std::get<std::string>(key);
                        else if (std::holds_alternative<int64_t>(key))
                            key_str = std::to_string(std::get<int64_t>(key));
                        else if (std::holds_alternative<double>(key))
                            key_str = Linh::format_float(std::get<double>(key));
                        else if (std::holds_alternative<bool>(key))
                            key_str = std::get<bool>(key) ? "true" : "false";
                        else
                            key_str = "";
                        (*map)[key_str] = value;
                    }
                    push(map);
                    break;
                }
                case OpCode::ARRAY_GET:
                {
                    if (stack.size() < 2)
                    {
                        std::cerr << "VM: ARRAY_GET stack underflow" << std::endl;
                        push(Value{}); // push sol
                        break;
                    }
                    Value idx = pop();
                    Value obj = pop();
                    // Nếu là array
                    if (std::holds_alternative<Array>(obj))
                    {
                        const auto &arr = std::get<Array>(obj);
                        int64_t i = 0;
                        if (std::holds_alternative<int64_t>(idx))
                            i = std::get<int64_t>(idx);
                        else if (std::holds_alternative<double>(idx))
                            i = static_cast<int64_t>(std::get<double>(idx));
                        else
                        {
                            push(Value{}); // sol
                            break;
                        }
                        if (i < 0 || static_cast<size_t>(i) >= arr->size())
                        {
                            push(Value{}); // sol
                        }
                        else
                        {
                            push((*arr)[i]);
                        }
                    }
                    else if (std::holds_alternative<Map>(obj))
                    {
                        const auto &map = std::get<Map>(obj);
                        std::string key;
                        if (std::holds_alternative<std::string>(idx))
                            key = std::get<std::string>(idx);
                        else if (std::holds_alternative<int64_t>(idx))
                            key = std::to_string(std::get<int64_t>(idx));
                        else if (std::holds_alternative<double>(idx))
                            key = std::to_string(std::get<double>(idx));
                        else if (std::holds_alternative<bool>(idx))
                            key = std::get<bool>(idx) ? "true" : "false";
                        else
                        {
                            push(Value{}); // sol
                            break;
                        }
                        auto it = map->find(key);
                        if (it != map->end())
                        {
                            push(it->second);
                        }
                        else
                        {
                            push(Value{}); // sol
                        }
                    }
                    else
                    {
                        push(Value{}); // sol
                    }
                    break;
                }
                case OpCode::ID:
                {
                    if (stack.empty())
                    {
                        push("0x0");
                    }
                    else
                    {
                        const Value &val = stack.back();
                        std::string addr_str;
                        // For Array/Map: use the address of the underlying object
                        if (std::holds_alternative<Array>(val))
                        {
                            const auto &arr = std::get<Array>(val);
                            addr_str = fmt::format("0x{:x}", reinterpret_cast<uintptr_t>(arr.get()));
                        }
                        else if (std::holds_alternative<Map>(val))
                        {
                            const auto &map = std::get<Map>(val);
                            addr_str = fmt::format("0x{:x}", reinterpret_cast<uintptr_t>(map.get()));
                        }
                        else
                        {
                            // For primitives: use the address of the value in the stack
                            addr_str = fmt::format("0x{:x}", reinterpret_cast<uintptr_t>(&val));
                        }
                        push(addr_str);
                    }
                    break;
                }
                case OpCode::ARRAY_APPEND:
                {
                    if (stack.size() < 2)
                    {
                        std::cerr << "VM: ARRAY_APPEND stack underflow" << std::endl;
                        push(Value{}); // push sol
                        break;
                    }
                    Value val = pop();
                    Value arr_val = pop();
                    if (std::holds_alternative<Array>(arr_val))
                    {
                        auto arr = std::get<Array>(arr_val);
                        arr->push_back(val);
                        push(arr); // push lại array
                    }
                    else
                    {
                        std::cerr << "VM: ARRAY_APPEND target is not array" << std::endl;
                        push(Value{}); // push sol
                    }
                    break;
                }
                case OpCode::ARRAY_REMOVE:
                {
                    if (stack.size() < 2)
                    {
                        std::cerr << "VM: ARRAY_REMOVE stack underflow" << std::endl;
                        push(Value{}); // push sol
                        break;
                    }
                    Value val = pop();
                    Value arr_val = pop();
                    if (std::holds_alternative<Array>(arr_val))
                    {
                        auto arr = std::get<Array>(arr_val);
                        // Tìm và xóa phần tử đầu tiên == val
                        auto it = std::find_if(arr->begin(), arr->end(), [&](const Value &v)
                                               {
                            // So sánh giá trị (chỉ hỗ trợ int, uint, double, string, bool)
                            if (v.index() != val.index()) return false;
                            if (std::holds_alternative<int64_t>(v))
                                return std::get<int64_t>(v) == std::get<int64_t>(val);
                            if (std::holds_alternative<uint64_t>(v))
                                return std::get<uint64_t>(v) == std::get<uint64_t>(val);
                            if (std::holds_alternative<double>(v))
                                return std::get<double>(v) == std::get<double>(val);
                            if (std::holds_alternative<std::string>(v))
                                return std::get<std::string>(v) == std::get<std::string>(val);
                            if (std::holds_alternative<bool>(v))
                                return std::get<bool>(v) == std::get<bool>(val);
                            return false; });
                        if (it != arr->end())
                            arr->erase(it);
                        push(arr); // push lại array
                    }
                    else
                    {
                        std::cerr << "VM: ARRAY_REMOVE target is not array" << std::endl;
                        push(Value{}); // push sol
                    }
                    break;
                }
                case OpCode::ARRAY_CLEAR:
                {
                    if (stack.empty())
                    {
                        std::cerr << "VM: ARRAY_CLEAR stack underflow" << std::endl;
                        push(Value{}); // push sol
                        break;
                    }
                    Value arr_val = pop();
                    if (std::holds_alternative<Array>(arr_val))
                    {
                        auto arr = std::get<Array>(arr_val);
                        arr->clear();
                        push(arr); // push lại array
                    }
                    else
                    {
                        std::cerr << "VM: ARRAY_CLEAR target is not array" << std::endl;
                        push(Value{}); // push sol
                    }
                    break;
                }
                case OpCode::ARRAY_CLONE:
                {
                    if (stack.empty())
                    {
                        std::cerr << "VM: ARRAY_CLONE stack underflow" << std::endl;
                        push(Value{}); // push sol
                        break;
                    }
                    Value arr_val = pop();
                    if (std::holds_alternative<Array>(arr_val))
                    {
                        auto arr = std::get<Array>(arr_val);
                        auto arr_clone = make_array();
                        *arr_clone = *arr;
                        push(arr_clone);
                    }
                    else
                    {
                        std::cerr << "VM: ARRAY_CLONE target is not array" << std::endl;
                        push(Value{}); // push sol
                    }
                    break;
                }
                case OpCode::ARRAY_POP:
                {
                    if (stack.empty())
                    {
                        std::cerr << "VM: ARRAY_POP stack underflow" << std::endl;
                        push(Value{}); // push sol
                        break;
                    }
                    Value maybe_idx_or_arr = pop();
                    // Nếu trên stack tiếp theo là array thì đây là dạng a.pop(index)
                    if (!stack.empty() && std::holds_alternative<Array>(stack.back()))
                    {
                        auto arr = std::get<Array>(pop());
                        // Xác định index
                        int64_t idx = -1;
                        if (std::holds_alternative<int64_t>(maybe_idx_or_arr))
                            idx = std::get<int64_t>(maybe_idx_or_arr);
                        else if (std::holds_alternative<double>(maybe_idx_or_arr))
                            idx = static_cast<int64_t>(std::get<double>(maybe_idx_or_arr));
                        else
                        {
                            push(Value{}); // sol nếu index không hợp lệ
                            break;
                        }
                        if (idx < 0 || static_cast<size_t>(idx) >= arr->size())
                        {
                            push(Value{}); // sol nếu index out of range
                        }
                        else
                        {
                            Value popped = (*arr)[idx];
                            arr->erase(arr->begin() + idx);
                            push(popped);
                        }
                    }
                    else if (std::holds_alternative<Array>(maybe_idx_or_arr))
                    {
                        // Dạng a.pop() không có index
                        auto arr = std::get<Array>(maybe_idx_or_arr);
                        if (!arr->empty())
                        {
                            Value popped = arr->back();
                            arr->pop_back();
                            push(popped);
                        }
                        else
                        {
                            push(Value{}); // sol nếu mảng rỗng
                        }
                    }
                    else
                    {
                        std::cerr << "VM: ARRAY_POP target is not array" << std::endl;
                        push(Value{}); // push sol
                    }
                    break;
                }
                case OpCode::MAP_GET:
                {
                    if (stack.size() < 2)
                    {
                        std::cerr << "VM: MAP_GET stack underflow" << std::endl;
                        push(Value{}); // push sol
                        break;
                    }
                    Value key = pop();
                    Value map = pop();
                    if (std::holds_alternative<Map>(map))
                    {
                        const auto &m = std::get<Map>(map);
                        std::string key_str;
                        if (std::holds_alternative<std::string>(key))
                            key_str = std::get<std::string>(key);
                        else if (std::holds_alternative<int64_t>(key))
                            key_str = std::to_string(std::get<int64_t>(key));
                        else if (std::holds_alternative<double>(key))
                            key_str = std::to_string(std::get<double>(key));
                        else if (std::holds_alternative<bool>(key))
                            key_str = std::get<bool>(key) ? "true" : "false";
                        else
                            key_str = "";
                        auto it = m->find(key_str);
                        if (it != m->end())
                            push(it->second);
                        else
                            push(Value{}); // sol
                    }
                    else
                    {
                        push(Value{}); // sol
                    }
                    break;
                }
                case OpCode::MAP_SET:
                {
                    if (stack.size() < 3)
                    {
                        std::cerr << "VM: MAP_SET stack underflow" << std::endl;
                        push(Value{}); // push sol
                        break;
                    }
                    Value value = pop();
                    Value key = pop();
                    Value map = pop();
                    if (std::holds_alternative<Map>(map))
                    {
                        auto m = std::get<Map>(map);
                        std::string key_str;
                        if (std::holds_alternative<std::string>(key))
                            key_str = std::get<std::string>(key);
                        else if (std::holds_alternative<int64_t>(key))
                            key_str = std::to_string(std::get<int64_t>(key));
                        else if (std::holds_alternative<double>(key))
                            key_str = std::to_string(std::get<double>(key));
                        else if (std::holds_alternative<bool>(key))
                            key_str = std::get<bool>(key) ? "true" : "false";
                        else
                            key_str = "";
                        (*m)[key_str] = value;
                        push(map); // push lại map
                    }
                    else
                    {
                        std::cerr << "VM: MAP_SET target is not map" << std::endl;
                        push(Value{}); // push sol
                    }
                    break;
                }
                case OpCode::MAP_DELETE:
                {
                    if (stack.size() < 2)
                    {
                        std::cerr << "VM: MAP_DELETE stack underflow" << std::endl;
                        push(Value{});
                        break;
                    }
                    Value key_val = pop();
                    Value map_val = pop();
                    if (std::holds_alternative<Map>(map_val))
                    {
                        auto map = std::get<Map>(map_val);
                        std::string key;
                        if (std::holds_alternative<std::string>(key_val))
                            key = std::get<std::string>(key_val);
                        else if (std::holds_alternative<int64_t>(key_val))
                            key = std::to_string(std::get<int64_t>(key_val));
                        else if (std::holds_alternative<double>(key_val))
                            key = std::to_string(std::get<double>(key_val));
                        else if (std::holds_alternative<bool>(key_val))
                            key = std::get<bool>(key_val) ? "true" : "false";
                        else
                            key = "";
                        map->erase(key);
                        push(map);
                    }
                    else
                    {
                        std::cerr << "VM: MAP_DELETE target is not map" << std::endl;
                        push(Value{});
                    }
                    break;
                }
                case OpCode::MAP_CLEAR:
                {
                    if (stack.empty())
                    {
                        std::cerr << "VM: MAP_CLEAR stack underflow" << std::endl;
                        push(Value{});
                        break;
                    }
                    Value map_val = pop();
                    if (std::holds_alternative<Map>(map_val))
                    {
                        auto map = std::get<Map>(map_val);
                        map->clear();
                        push(map);
                    }
                    else
                    {
                        std::cerr << "VM: MAP_CLEAR target is not map" << std::endl;
                        push(Value{});
                    }
                    break;
                }
                case OpCode::MAP_KEYS:
                {
                    if (stack.empty())
                    {
                        std::cerr << "VM: MAP_KEYS stack underflow" << std::endl;
                        push(Value{});
                        break;
                    }
                    Value map_val = pop();
                    if (std::holds_alternative<Map>(map_val))
                    {
                        auto map = std::get<Map>(map_val);
                        Array arr = make_array();
                        for (const auto &kv : *map)
                        {
                            arr->push_back(kv.first);
                        }
                        push(arr);
                    }
                    else
                    {
                        std::cerr << "VM: MAP_KEYS target is not map" << std::endl;
                        push(Value{});
                    }
                    break;
                }
                case OpCode::MAP_VALUES:
                {
                    if (stack.empty())
                    {
                        std::cerr << "VM: MAP_VALUES stack underflow" << std::endl;
                        push(Value{});
                        break;
                    }
                    Value map_val = pop();
                    if (std::holds_alternative<Map>(map_val))
                    {
                        auto map = std::get<Map>(map_val);
                        Array arr = make_array();
                        for (const auto &kv : *map)
                        {
                            arr->push_back(kv.second);
                        }
                        push(arr);
                    }
                    else
                    {
                        std::cerr << "VM: MAP_VALUES target is not map" << std::endl;
                        push(Value{});
                    }
                    break;
                }
                case OpCode::LOAD_PACKAGE_CONST:
                {
                    // Operand is a string: "package.constant"
                    std::string full_name;
                    if (std::holds_alternative<std::string>(instr.operand))
                        full_name = std::get<std::string>(instr.operand);
                    else
                        full_name = "";
                    auto dot_pos = full_name.find('.');
                    if (dot_pos != std::string::npos)
                    {
                        std::string package = full_name.substr(0, dot_pos);
                        std::string constant = full_name.substr(dot_pos + 1);
                        auto val = Linh::Std::get_constant(package, constant);
                        push(val);
                    }
                    else
                    {
                        push(Value{}); // sol
                    }
                    break;
                }
                case OpCode::CALL_PACKAGE_FUNCTION:
                {
                    handle_CALL_PACKAGE_FUNCTION(*this, instr, chunk, ip);
                    break;
                }
                case OpCode::CREATE_CLOSURE:
                    handle_CREATE_CLOSURE(*this, instr, chunk, ip);
                    break;
                case OpCode::CAPTURE_VAR:
                    handle_CAPTURE_VAR(*this, instr, chunk, ip);
                    break;
                case OpCode::LOAD_CLOSURE_VAR:
                    handle_LOAD_CLOSURE_VAR(*this, instr, chunk, ip);
                    break;
                default:
                    ++ip;
                    break;
                }
            }
            catch (const std::exception &ex)
            {
#ifdef _DEBUG
                std::cerr << "[DEBUG][EXCEPTION] " << ex.what() << " at ip=" << ip << std::endl;
#endif
                if (!try_stack.empty())
                {
                    // Get the error variable index from the try frame
                    int error_var_index = 1; // Default to 1 for "error"
                    if (!try_stack.empty() && try_stack.back().error_var == "error")
                    {
                        error_var_index = 1; // "error" variable is at index 1
                    }
                    variables[error_var_index] = std::string(ex.what());
                    ip = try_stack.back().catch_ip;
                    continue;
                }
                else
                {
                    std::cerr << "VM Exception: " << ex.what() << std::endl;
                    return;
                }
            }
            ++ip;
        }
        
        // Performance tracking end
        auto end_time = std::chrono::high_resolution_clock::now();
        execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Optimize stack after execution
        if (stack_optimization_enabled) {
            optimize_stack();
        }
    }

    void LiVM::type()
    {
        if (stack.empty())
        {
            std::cout << "stack_empty" << std::endl;
            return;
        }
        const auto &val = stack.back();
        if (std::holds_alternative<int64_t>(val))
            std::cout << "int" << std::endl;
        else if (std::holds_alternative<double>(val))
            std::cout << "float" << std::endl;
        else if (std::holds_alternative<std::string>(val))
            std::cout << "str" << std::endl;
        else if (std::holds_alternative<bool>(val))
            std::cout << "bool" << std::endl;
        else
            std::cout << "unknown" << std::endl;
    }

    static void handle_GTE(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        auto b = vm.pop();
        auto a = vm.pop();
        bool result = false;
        // If either is string, compare as string
        if (std::holds_alternative<std::string>(a) || std::holds_alternative<std::string>(b)) {
            std::string sa = std::holds_alternative<std::string>(a) ? std::get<std::string>(a) : Linh::to_str(a);
            std::string sb = std::holds_alternative<std::string>(b) ? std::get<std::string>(b) : Linh::to_str(b);
            result = (sa >= sb);
        }
        // If both are bool
        else if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b)) {
            bool av = std::get<bool>(a);
            bool bv = std::get<bool>(b);
            result = (av || !bv);
        }
        // If both are numbers (int/double/uint)
        else if ((std::holds_alternative<int64_t>(a) || std::holds_alternative<double>(a) || std::holds_alternative<uint64_t>(a)) &&
                 (std::holds_alternative<int64_t>(b) || std::holds_alternative<double>(b) || std::holds_alternative<uint64_t>(b))) {
            double av = std::holds_alternative<int64_t>(a) ? static_cast<double>(std::get<int64_t>(a)) :
                        (std::holds_alternative<uint64_t>(a) ? static_cast<double>(std::get<uint64_t>(a)) : std::get<double>(a));
            double bv = std::holds_alternative<int64_t>(b) ? static_cast<double>(std::get<int64_t>(b)) :
                        (std::holds_alternative<uint64_t>(b) ? static_cast<double>(std::get<uint64_t>(b)) : std::get<double>(b));
            result = (av >= bv);
        }
        // Fallback: compare as string
        else {
            std::string sa = Linh::to_str(a);
            std::string sb = Linh::to_str(b);
            result = (sa >= sb);
        }
        vm.push(result);
    }
    static void handle_JMP_IF_TRUE(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t& ip) {
        auto cond = vm.pop();
        bool cond_val = eval_condition(cond);
        if (cond_val)
            ip = std::get<int64_t>(instr.operand);
        else
            ++ip;
    }
    static void handle_CALL(LiVM& vm, const Instruction& instr, const BytecodeChunk& chunk, size_t& ip) {
#ifdef _DEBUG
        std::cerr << "[DEBUG] handle_CALL: entering function" << std::endl;
        std::cerr << "[DEBUG] CALL: stack size = " << vm.stack.size() << std::endl;
        if (!vm.stack.empty()) {
            std::cerr << "[DEBUG] CALL: top of stack index = " << vm.stack.back().index() << std::endl;
        }
#endif
        // Built-in explicit conversions used by declared types inside functions
        if (std::holds_alternative<std::string>(instr.operand)) {
            const std::string& fname = std::get<std::string>(instr.operand);
            auto convert_and_advance = [&](auto conv) {
                if (vm.stack.empty()) { vm.push(std::monostate{}); ++ip; return true; }
                auto v = vm.pop();
                vm.push(conv(v));
                ++ip;
                return true;
            };
            if (fname == "to_int8") { convert_and_advance([](const Value& v){ return Value(static_cast<int8_t>(Linh::to_int8(v))); }); return; }
            if (fname == "to_int16"){ convert_and_advance([](const Value& v){ return Value(static_cast<int16_t>(Linh::to_int16(v))); }); return; }
            if (fname == "to_int32"){ convert_and_advance([](const Value& v){ return Value(static_cast<int32_t>(Linh::to_int32(v))); }); return; }
            if (fname == "to_int64"){ convert_and_advance([](const Value& v){ return Value(static_cast<int64_t>(Linh::to_int64(v))); }); return; }
            if (fname == "to_uint8"){ convert_and_advance([](const Value& v){ return Value(static_cast<uint8_t>(Linh::to_uint8(v))); }); return; }
            if (fname == "to_uint16"){ convert_and_advance([](const Value& v){ return Value(static_cast<uint16_t>(Linh::to_uint16(v))); }); return; }
            if (fname == "to_uint32"){ convert_and_advance([](const Value& v){ return Value(static_cast<uint32_t>(Linh::to_uint32(v))); }); return; }
            if (fname == "to_uint64"){ convert_and_advance([](const Value& v){ return Value(static_cast<uint64_t>(Linh::to_uint64(v))); }); return; }
            if (fname == "to_float32"){ convert_and_advance([](const Value& v){ return Value(Linh::to_float32(v)); }); return; }
            if (fname == "to_float64"){ convert_and_advance([](const Value& v){ return Value(Linh::to_float64(v)); }); return; }
            if (fname == "to_bool"){ convert_and_advance([](const Value& v){ return Value(Linh::to_bool(v)); }); return; }
            if (fname == "to_string"){ convert_and_advance([](const Value& v){ return Value(Linh::to_str(v)); }); return; }
        }
        // Kiểm tra xem có function object trên stack không
        if (!vm.stack.empty()) {
            auto& top_value = vm.stack.back();
#ifdef _DEBUG
            std::cerr << "[DEBUG] CALL: top value index = " << top_value.index() << std::endl;
#endif
            if (top_value.index() == 15) { // FunctionPtr is at index 15 in Value
#ifdef _DEBUG
                std::cerr << "[DEBUG] CALL: found function object on stack" << std::endl;
#endif
                // Gọi function object
                auto fn = std::get<15>(vm.stack.back());
                vm.pop(); // Pop function object
                
                // Thu thập arguments từ stack (arguments được push theo thứ tự ngược)
                std::vector<Value> args;
                size_t expected_args = fn->params.size();
                
                // Arguments được push theo thứ tự từ trái sang phải, nên trên stack sẽ ngược lại
                for (size_t i = 0; i < expected_args; ++i) {
                    if (vm.stack.empty()) {
                        std::cerr << "Error: Not enough arguments for function " << fn->name << std::endl;
                        vm.push(Value()); // Push default value
                        ++ip;
                        return;
                    }
                    args.insert(args.begin(), vm.pop()); // Insert at beginning để giữ thứ tự đúng
                }
                
#ifdef _DEBUG
                std::cerr << "[DEBUG] CALL: calling function " << fn->name << " with " << args.size() << " arguments" << std::endl;
#endif
                
                // Gọi function object
                auto result = call_function(fn, args, vm);
                vm.push(result);
                ++ip;
            } else {
#ifdef _DEBUG
                std::cerr << "[DEBUG] CALL: no function object found, falling back to legacy call" << std::endl;
#endif
                // Gọi function theo tên (legacy behavior)
                std::string func_name = std::get<std::string>(instr.operand);
                auto it = vm.functions.find(func_name);
                if (it != vm.functions.end()) {
                    // Push return address
                    vm.call_stack.push_back({ip + 1, {}});
                    // Set up new frame
                    ip = 0;
                    // Note: This is simplified - in real implementation you'd need to handle parameters
                } else {
                    std::cerr << "VM: Unknown function '" << func_name << "'" << std::endl;
                    ++ip;
                }
            }
        }
    }
    static void handle_RET(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t& ip) {
        if (!vm.call_stack.empty()) {
            auto frame = vm.call_stack.back();
            vm.call_stack.pop_back();
            ip = frame.return_ip;
        } else {
            // No call stack, exit
            ip = SIZE_MAX; // Signal to exit
        }
    }
    static void handle_HALT(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t& ip) {
        ip = SIZE_MAX; // Signal to exit
    }
    static void handle_LOAD_VAR(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        int idx = std::get<int64_t>(instr.operand);
#ifdef _DEBUG
        std::cerr << "[DEBUG] LOAD_VAR: loading variable index " << idx << std::endl;
        std::cerr << "[DEBUG] LOAD_VAR: variables.size() = " << vm.variables.size() << std::endl;
#endif
        if (vm.variables.count(idx)) {
            auto value = vm.variables[idx];
#ifdef _DEBUG
            std::cerr << "[DEBUG] LOAD_VAR: loaded value index = " << value.index() << std::endl;
#endif
            vm.push(value);
            
            // Track variable usage in current environment for closure capture
            std::string var_name = "var_" + std::to_string(idx);
            vm.current_environment[var_name] = value;
        } else {
            // Nếu tên biến là error.message và error tồn tại, trả về error
            if (idx == 3 && vm.variables.count(2)) {
                vm.push(vm.variables[2]);
            } else {
                std::cerr << "VM: LOAD_VAR unknown variable index " << idx << std::endl;
                vm.push(int64_t(0));
            }
        }
    }
    static void handle_STORE_VAR(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        int idx = std::get<int64_t>(instr.operand);
        if (vm.stack.empty()) {
            vm.stack.push_back(std::monostate{});
        }
        Value value = vm.pop();
        vm.variables[idx] = value;
        
        // Track variable in current environment for closure capture
        // This is a simplified approach - in practice you'd have proper variable name tracking
        // For now, we'll use the index as a simple identifier
        std::string var_name = "var_" + std::to_string(idx);
        vm.current_environment[var_name] = value;
    }
    static void handle_AND(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        auto b = vm.pop();
        auto a = vm.pop();
        if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b)) {
            bool av = std::get<bool>(a);
            bool bv = std::get<bool>(b);
            vm.push(av && bv);
        } else {
            std::cerr << "ERROR [Line: 0, Col: 0] RuntimeError: AND only supports bool" << std::endl;
            vm.push(false);
        }
    }
    static void handle_OR(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        auto b = vm.pop();
        auto a = vm.pop();
        if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b)) {
            bool av = std::get<bool>(a);
            bool bv = std::get<bool>(b);
            vm.push(av || bv);
        } else {
            std::cerr << "ERROR [Line: 0, Col: 0] RuntimeError: OR only supports bool" << std::endl;
            vm.push(false);
        }
    }
    static void handle_NOT(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        auto a = vm.pop();
        if (std::holds_alternative<bool>(a))
            vm.push(!std::get<bool>(a));
        else if (std::holds_alternative<int64_t>(a))
            vm.push(~std::get<int64_t>(a)); // bitwise NOT
        else {
            std::cerr << "VM: NOT only supports bool or int" << std::endl;
            vm.push(false);
        }
    }
    static void handle_INPUT(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
        auto prompt = vm.pop();
        std::string prompt_str;
        if (std::holds_alternative<std::string>(prompt))
            prompt_str = std::get<std::string>(prompt);
        else
            prompt_str = "";
        auto input_val = LinhIO::linh_input(prompt_str);
        vm.push(input_val);
    }
    static void handle_TYPEOF(LiVM& vm, const Instruction& instr, const BytecodeChunk&, size_t&) {
#ifdef _DEBUG
        std::cerr << "[DEBUG] TYPEOF: starting" << std::endl;
#endif
        if (vm.stack.empty()) {
            std::cerr << "ERROR [Line " << instr.line << ", Col " << instr.col << "] RuntimeError : VM stack underflow" << std::endl;
        } else {
            auto val = vm.pop();
#ifdef _DEBUG
            std::cerr << "[DEBUG] TYPEOF: value index = " << val.index() << std::endl;
#endif
            std::string type_str = type_of(val);
#ifdef _DEBUG
            std::cerr << "[DEBUG] TYPEOF: returning " << type_str << std::endl;
#endif
            vm.push(type_str);
        }
    }
    static void handle_PUSH_ARRAY(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        vm.push(make_array());
    }
    static void handle_ARRAY_GET(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto idx = vm.pop();
        auto arr_val = vm.pop();
        if (std::holds_alternative<Array>(arr_val)) {
            auto arr = std::get<Array>(arr_val);
            int64_t i = Linh::to_int(idx);
            if (i >= 0 && i < (int64_t)arr->size())
                vm.push((*arr)[i]);
            else
                vm.push(Value{}); // sol
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_ARRAY_SET(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto idx = vm.pop();
        auto arr_val = vm.pop();
        auto value = vm.pop();
        if (std::holds_alternative<Array>(arr_val)) {
            auto arr = std::get<Array>(arr_val);
            int64_t i = Linh::to_int(idx);
            if (i >= 0 && i < (int64_t)arr->size())
                (*arr)[i] = value;
            vm.push(arr);
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_ARRAY_LEN(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto arr_val = vm.pop();
        if (std::holds_alternative<Array>(arr_val)) {
            auto arr = std::get<Array>(arr_val);
            vm.push((int64_t)arr->size());
        } else {
            vm.push(int64_t(0));
        }
    }
    static void handle_ARRAY_APPEND(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto val = vm.pop();
        auto arr_val = vm.pop();
        if (std::holds_alternative<Array>(arr_val)) {
            auto arr = std::get<Array>(arr_val);
            arr->push_back(val);
            vm.push(arr);
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_ARRAY_REMOVE(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto idx = vm.pop();
        auto arr_val = vm.pop();
        if (std::holds_alternative<Array>(arr_val)) {
            auto arr = std::get<Array>(arr_val);
            int64_t i = Linh::to_int(idx);
            if (i >= 0 && i < (int64_t)arr->size())
                arr->erase(arr->begin() + i);
            vm.push(arr);
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_ARRAY_CLEAR(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto arr_val = vm.pop();
        if (std::holds_alternative<Array>(arr_val)) {
            auto arr = std::get<Array>(arr_val);
            arr->clear();
            vm.push(arr);
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_ARRAY_CLONE(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto arr_val = vm.pop();
        if (std::holds_alternative<Array>(arr_val)) {
            auto arr = std::get<Array>(arr_val);
            auto new_arr = make_array();
            *new_arr = *arr;
            vm.push(new_arr);
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_ARRAY_POP(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto arr_val = vm.pop();
        if (std::holds_alternative<Array>(arr_val)) {
            auto arr = std::get<Array>(arr_val);
            if (!arr->empty())
                arr->pop_back();
            vm.push(arr);
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_PUSH_MAP(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        vm.push(make_map());
    }
    static void handle_MAP_GET(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto idx = vm.pop();
        auto map_val = vm.pop();
        if (std::holds_alternative<Map>(map_val)) {
            auto map = std::get<Map>(map_val);
            std::string key = Linh::to_str(idx);
            auto it = map->find(key);
            if (it != map->end())
                vm.push(it->second);
            else
                vm.push(Value{}); // sol
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_MAP_SET(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto idx = vm.pop();
        auto map_val = vm.pop();
        auto value = vm.pop();
        if (std::holds_alternative<Map>(map_val)) {
            auto map = std::get<Map>(map_val);
            std::string key = Linh::to_str(idx);
            (*map)[key] = value;
            vm.push(map);
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_MAP_KEYS(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto map_val = vm.pop();
        if (std::holds_alternative<Map>(map_val)) {
            auto map = std::get<Map>(map_val);
            auto arr = make_array();
            for (const auto& kv : *map) {
                arr->push_back(kv.first);
            }
            vm.push(arr);
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_MAP_VALUES(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto map_val = vm.pop();
        if (std::holds_alternative<Map>(map_val)) {
            auto map = std::get<Map>(map_val);
            auto arr = make_array();
            for (const auto& kv : *map) {
                arr->push_back(kv.second);
            }
            vm.push(arr);
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_MAP_DELETE(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto idx = vm.pop();
        auto map_val = vm.pop();
        if (std::holds_alternative<Map>(map_val)) {
            auto map = std::get<Map>(map_val);
            std::string key = Linh::to_str(idx);
            map->erase(key);
            vm.push(map);
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_MAP_CLEAR(LiVM& vm, const Instruction&, const BytecodeChunk&, size_t&) {
        auto map_val = vm.pop();
        if (std::holds_alternative<Map>(map_val)) {
            auto map = std::get<Map>(map_val);
            map->clear();
            vm.push(map);
        } else {
            vm.push(Value{}); // sol
        }
    }
    static void handle_TRY(LiVM&, const Instruction&, const BytecodeChunk&, size_t&) {
        /* Xử lý TRY nếu cần */
    }
    static void handle_END_TRY(LiVM&, const Instruction&, const BytecodeChunk&, size_t&) {
        /* Xử lý END_TRY nếu cần */
    }

    // Implementation of run_chunk for function execution
    void LiVM::run_chunk(const BytecodeChunk &chunk) {
        size_t local_ip = 0;
#ifdef _DEBUG
        std::cerr << "[DEBUG] run_chunk: starting execution with " << chunk.size() << " instructions" << std::endl;
#endif
        
        while (local_ip < chunk.size()) {
            const auto &instr = chunk[local_ip];
            
#ifdef _DEBUG
            if (instr.opcode == OpCode::CALL) {
                std::cerr << "[DEBUG] run_chunk: about to execute CALL, stack size = " << this->stack.size() << std::endl;
                if (this->stack.size() >= 2) {
                    std::cerr << "[DEBUG] run_chunk: top of stack index = " << this->stack.back().index() << std::endl;
                    std::cerr << "[DEBUG] run_chunk: second from top index = " << this->stack[this->stack.size()-2].index() << std::endl;
                }
            }
#endif
            
            // Simple execution without optimization for function calls
            switch (instr.opcode) {
                case OpCode::PUSH_INT:
                    push(std::get<int64_t>(instr.operand));
                    break;
                case OpCode::PUSH_UINT:
                    push(std::get<uint64_t>(instr.operand));
                    break;
                case OpCode::PUSH_FLOAT:
                    push(std::get<double>(instr.operand));
                    break;
                case OpCode::PUSH_STR:
                    push(std::get<std::string>(instr.operand));
                    break;
                case OpCode::PUSH_BOOL:
                    push(std::get<bool>(instr.operand));
                    break;
                case OpCode::LOAD_VAR:
                    handle_LOAD_VAR(*this, instr, chunk, local_ip);
                    break;
                case OpCode::STORE_VAR:
                    handle_STORE_VAR(*this, instr, chunk, local_ip);
                    break;
                case OpCode::ADD:
                    handle_ADD(*this, instr, chunk, local_ip);
                    break;
                case OpCode::SUB:
                    handle_SUB(*this, instr, chunk, local_ip);
                    break;
                case OpCode::MUL:
                    handle_MUL(*this, instr, chunk, local_ip);
                    break;
                case OpCode::DIV:
                    handle_DIV(*this, instr, chunk, local_ip);
                    break;
                case OpCode::MOD:
                    handle_MOD(*this, instr, chunk, local_ip);
                    break;
                case OpCode::PRINT:
                    handle_PRINT(*this, instr, chunk, local_ip);
                    break;
                case OpCode::PRINT_MULTIPLE:
                    handle_PRINT_MULTIPLE(*this, instr, chunk, local_ip);
                    break;
                case OpCode::PUSH_FUNCTION:
                    handle_PUSH_FUNCTION(*this, instr, chunk, local_ip);
                    break;
                case OpCode::CREATE_CLOSURE:
                    handle_CREATE_CLOSURE(*this, instr, chunk, local_ip);
                    break;
                case OpCode::CAPTURE_VAR:
                    handle_CAPTURE_VAR(*this, instr, chunk, local_ip);
                    break;
                case OpCode::LOAD_CLOSURE_VAR:
                    handle_LOAD_CLOSURE_VAR(*this, instr, chunk, local_ip);
                    break;
                case OpCode::SWAP:
                    handle_SWAP(*this, instr, chunk, local_ip);
                    break;
                case OpCode::CALL:
                    handle_CALL(*this, instr, chunk, local_ip);
                    break;
                case OpCode::JMP:
                    handle_JMP(*this, instr, chunk, local_ip);
                    break;
                case OpCode::JMP_IF_FALSE:
                    handle_JMP_IF_FALSE(*this, instr, chunk, local_ip);
                    break;
                case OpCode::JMP_IF_TRUE:
                    handle_JMP_IF_TRUE(*this, instr, chunk, local_ip);
                    break;
                case OpCode::RET:
#ifdef _DEBUG
                    std::cerr << "[DEBUG] run_chunk: RET instruction, returning from function" << std::endl;
#endif
                    return; // Return from function
                case OpCode::HALT:
                    return; // End execution
                default:
                    std::cerr << "VM: Unknown opcode in function: " << static_cast<int>(instr.opcode) << std::endl;
                    break;
            }
            local_ip++;
        }
    }

} // namespace Linh