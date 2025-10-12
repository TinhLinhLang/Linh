#include "Loop.hpp"
#include <variant>

namespace Linh
{
    // Hàm kiểm tra điều kiện cho JMP_IF_TRUE/FALSE
    inline bool eval_condition(const Value& cond) {
        return std::visit([](auto&& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, bool>)
                return arg;
            else if constexpr (std::is_same_v<T, int64_t>)
                return arg != 0;
            else if constexpr (std::is_same_v<T, double>)
                return arg != 0.0;
            else if constexpr (std::is_same_v<T, std::string>)
                return !arg.empty();
            else
                return false;
        }, cond);
    }

    void handle_loop_opcode(LiVM &vm, const Instruction &instr, const BytecodeChunk &chunk, size_t &ip)
    {
        switch (instr.opcode)
        {
        case OpCode::JMP:
            ip = std::get<int64_t>(instr.operand);
            break;
        case OpCode::JMP_IF_FALSE:
        {
            auto cond = vm.pop();
            bool cond_val = eval_condition(cond);
            if (!cond_val)
                ip = std::get<int64_t>(instr.operand);
            else
                ++ip;
            return;
        }
        case OpCode::JMP_IF_TRUE:
        {
            auto cond = vm.pop();
            bool cond_val = eval_condition(cond);
            if (cond_val)
                ip = std::get<int64_t>(instr.operand);
            else
                ++ip;
            return;
        }
        default:
            ++ip;
            break;
        }
    }
}
