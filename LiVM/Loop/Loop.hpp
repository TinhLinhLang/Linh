#pragma once
#include "LiVM.hpp"
#include "LinhC/Bytecode/Bytecode.hpp"

namespace Linh
{
    bool eval_condition(const Value& cond); // Thêm prototype cho eval_condition
    void handle_loop_opcode(LiVM &vm, const Instruction &instr, const BytecodeChunk &chunk, size_t &ip);
}
