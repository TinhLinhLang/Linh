#pragma once
#include "LinhC/Bytecode/Bytecode.hpp"
#include "LiVM/Variable/Value.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>
#include <chrono>
#include <unordered_set>
#include <functional>

namespace Linh
{
    class LiVM
    {
    public:
        LiVM();
        void run(const BytecodeChunk &chunk);
        void run_chunk(const BytecodeChunk &chunk); // Thêm method này cho function execution

        void type();

        friend void handle_loop_opcode(LiVM &vm, const Instruction &instr, const BytecodeChunk &chunk, size_t &ip);
        friend void math_binary_op(LiVM &vm, const Instruction &instr); // Thêm dòng này
        friend void handle_PUSH_INT(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_PUSH_FLOAT(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_ADD(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_SUB(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_MUL(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_DIV(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_MOD(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_HASH(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_AMP(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_PIPE(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_CARET(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_LT_LT(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_GT_GT(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_EQ(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_NEQ(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_LT(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_GT(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_LTE(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_GTE(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_JMP(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_JMP_IF_FALSE(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_JMP_IF_TRUE(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_CALL(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_RET(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_HALT(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_LOAD_VAR(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_STORE_VAR(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_AND(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_OR(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_NOT(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_INPUT(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_TYPEOF(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_PRINT(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_NOP(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_PUSH_ARRAY(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_ARRAY_GET(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_ARRAY_SET(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_ARRAY_LEN(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_ARRAY_APPEND(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_ARRAY_REMOVE(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_ARRAY_CLEAR(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_ARRAY_CLONE(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_ARRAY_POP(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_PUSH_MAP(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_MAP_GET(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_MAP_SET(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_MAP_KEYS(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_MAP_VALUES(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_MAP_DELETE(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_MAP_CLEAR(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_TRY(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_END_TRY(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_PRINT_MULTIPLE(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_PRINTF(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_POP(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_SWAP(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_DUP(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_PUSH_UINT(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_PUSH_STR(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_PUSH_BOOL(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_PUSH_FUNCTION(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend Value call_function(FunctionPtr, const std::vector<Value>&, LiVM&);
        friend void handle_LOAD_PACKAGE_CONST(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_CALL_PACKAGE_FUNCTION(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);

        // Closure support handlers
        friend void handle_CREATE_CLOSURE(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_CAPTURE_VAR(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);
        friend void handle_LOAD_CLOSURE_VAR(LiVM&, const Instruction&, const BytecodeChunk&, size_t&);

        // Optimization methods
        void enable_instruction_caching(bool enable = true) { instruction_caching_enabled = enable; }
        void enable_stack_optimization(bool enable = true) { stack_optimization_enabled = enable; }
        void enable_hot_path_optimization(bool enable = true) { hot_path_optimization_enabled = enable; }
        
        // Performance monitoring
        size_t get_execution_time_ms() const { return execution_time_ms; }
        size_t get_instruction_count() const { return instruction_count; }
        size_t get_cache_hits() const { return cache_hits; }
        size_t get_cache_misses() const { return cache_misses; }
        double get_instructions_per_ms() const { 
            return execution_time_ms > 0 ? static_cast<double>(instruction_count) / execution_time_ms : 0.0; 
        }

        struct Function
        {
            BytecodeChunk code;
            std::vector<std::string> param_names;
        };
        std::unordered_map<std::string, Function> functions;
        struct CallFrame
        {
            size_t return_ip;
            std::unordered_map<int, Value> locals;
        };
        std::vector<CallFrame> call_stack;

        // Closure support
        std::unordered_map<std::string, Value> current_environment; // Current scope variables for closure capture

        void set_functions(const std::unordered_map<std::string, Function> &funcs)
        {
            functions = funcs;
        }

        // Getter/setter cho biến toàn cục REPL
        const std::unordered_map<int, Value> &get_global_variables() const { return variables; }
        void set_global_variables(const std::unordered_map<int, Value> &vars) { variables = vars; }
        
        // Set exported variables from modules
        void set_exported_variables(const std::unordered_map<std::string, Value> &exported_vars) {
            exported_variables = exported_vars;
        }

    private:
        std::vector<Value> stack;
        std::unordered_map<int, Value> variables;
        std::unordered_map<std::string, Value> exported_variables; // Variables exported from modules
        size_t ip = 0; // instruction pointer
        
        // Optimization flags
        bool instruction_caching_enabled = false; // Tắt cache để đảm bảo các lệnh có side-effect hoạt động đúng
        bool stack_optimization_enabled = true;
        bool hot_path_optimization_enabled = true;
        
        // Performance tracking
        size_t execution_time_ms = 0;
        size_t instruction_count = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        
        // Instruction caching
        std::unordered_map<size_t, std::function<void()>> instruction_cache;
        std::unordered_set<size_t> hot_paths;
        
        // Stack optimization
        static constexpr size_t STACK_RESERVE_SIZE = 1024;
        static constexpr size_t STACK_SHRINK_THRESHOLD = 512;

    public:
        // Module system support
        std::string current_file_path;
        std::string current_module_name;
        void push(const Value &val);
        Value pop();
        Value peek();
        
        // Optimization helpers
        void cache_instruction(size_t ip, const Instruction& instr);
        void optimize_stack();
        void track_hot_path(size_t ip);
        void execute_cached_instruction(size_t ip);
    };
}