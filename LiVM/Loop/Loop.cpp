#include "Loop.hpp"
#include <variant>

namespace Linh
{
    // Hàm kiểm tra điều kiện cho JMP_IF_TRUE/FALSE
    inline bool eval_condition(const Value& cond) {
        if (Linh::holds_alternative<bool>(cond))
            return Linh::get<bool>(cond);
        else if (Linh::holds_alternative<int64_t>(cond))
            return Linh::get<int64_t>(cond) != 0;
        else if (Linh::holds_alternative<double>(cond))
            return Linh::get<double>(cond) != 0.0;
        else if (Linh::holds_alternative<std::string>(cond))
            return !Linh::get<std::string>(cond).empty();
        else
            return false;
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
