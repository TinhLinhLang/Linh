#include "Math.hpp"
#include "LiVM/Variable/type.hpp"
#include <cmath>
#include <iostream>
#include <variant>
#include <string>

#ifdef _DEBUG
// Helper to print a Value for debug
static void debug_print_value(const Linh::Value& v, std::ostream& os = std::cerr) {
    os << "[" << Linh::type_of(v) << "] ";
    os << Linh::to_str(v);
}
#endif

namespace Linh
{
    void math_binary_op(LiVM &vm, const Instruction &instr)
    {
        if (vm.stack.size() < 2) {
            std::cerr << "VM stack underflow for binary operation" << std::endl;
            return;
        }
        auto b = vm.stack.back();
        vm.stack.pop_back();
        auto a = vm.stack.back();
        vm.stack.pop_back();
#ifdef _DEBUG
        std::cerr << "[DEBUG] Stack before pop (size=" << vm.stack.size() << "): ";
        debug_print_value(a);
        std::cerr << ", ";
        debug_print_value(b);
        std::cerr << std::endl;
#endif

        // Chuẩn hoá toàn bộ kiểu số về 64-bit:
        // - int8/16/32  -> int64_t
        // - uint8/16/32 -> uint64_t
        // - float32     -> float64 (double)
        auto normalize_number = [](const Value& x) -> Value {
            if (Linh::holds_alternative<int8_t>(x))   return Value(static_cast<int64_t>(Linh::get<int8_t>(x)));
            if (Linh::holds_alternative<int16_t>(x))  return Value(static_cast<int64_t>(Linh::get<int16_t>(x)));
            if (Linh::holds_alternative<int32_t>(x))  return Value(static_cast<int64_t>(Linh::get<int32_t>(x)));
            if (Linh::holds_alternative<uint8_t>(x))  return Value(static_cast<uint64_t>(Linh::get<uint8_t>(x)));
            if (Linh::holds_alternative<uint16_t>(x)) return Value(static_cast<uint64_t>(Linh::get<uint16_t>(x)));
            if (Linh::holds_alternative<uint32_t>(x)) return Value(static_cast<uint64_t>(Linh::get<uint32_t>(x)));
            if (Linh::holds_alternative<float>(x))    return Value(static_cast<double>(Linh::get<float>(x)));
            return x;
        };
        a = normalize_number(a);
        b = normalize_number(b);

        // --- CHẶN CỘNG SAI KIỂU DỮ LIỆU ---
        if (instr.opcode == OpCode::ADD)
        {
            // Nếu một trong hai toán hạng là bool thì không cho phép cộng
            if (Linh::holds_alternative<bool>(a) || Linh::holds_alternative<bool>(b))
            {
#ifdef _DEBUG
                std::cerr << "[ERROR] Invalid operand types for '+' (bool is not allowed): ";
                if (Linh::holds_alternative<bool>(a))
                    std::cerr << (Linh::get<bool>(a) ? "true" : "false");
                else if (Linh::holds_alternative<std::string>(a))
                    std::cerr << '"' << Linh::get<std::string>(a) << '"';
                else if (Linh::holds_alternative<int64_t>(a))
                    std::cerr << Linh::get<int64_t>(a);
                else if (Linh::holds_alternative<uint64_t>(a))
                    std::cerr << Linh::get<uint64_t>(a);
                else if (Linh::holds_alternative<double>(a))
                    std::cerr << Linh::get<double>(a);
                else
                    std::cerr << "(?)";
                std::cerr << " + ";
                if (Linh::holds_alternative<bool>(b))
                    std::cerr << (Linh::get<bool>(b) ? "true" : "false");
                else if (Linh::holds_alternative<std::string>(b))
                    std::cerr << '"' << Linh::get<std::string>(b) << '"';
                else if (Linh::holds_alternative<int64_t>(b))
                    std::cerr << Linh::get<int64_t>(b);
                else if (Linh::holds_alternative<uint64_t>(b))
                    std::cerr << Linh::get<uint64_t>(b);
                else if (Linh::holds_alternative<double>(b))
                    std::cerr << Linh::get<double>(b);
                else
                    std::cerr << "(?)";
                std::cerr << std::endl;
#endif
                vm.push(Value());
                return;
            }
        }
        // --- HỖ TRỢ NỐI CHUỖI ---
        if (instr.opcode == OpCode::ADD &&
            (Linh::holds_alternative<std::string>(a) || Linh::holds_alternative<std::string>(b)))
        {
            // Nếu một trong hai toán hạng là bool thì không cho phép nối chuỗi
            if (Linh::holds_alternative<bool>(a) || Linh::holds_alternative<bool>(b))
            {
#ifdef _DEBUG
                std::cerr << "[ERROR] Invalid operand types for string concatenation: cannot concatenate string and bool." << std::endl;
#endif
                vm.push(Value());
                return;
            }
            std::string sa, sb;
            if (Linh::holds_alternative<std::string>(a))
                sa = Linh::get<std::string>(a);
            else if (Linh::holds_alternative<int64_t>(a))
                sa = std::to_string(Linh::get<int64_t>(a));
            else if (Linh::holds_alternative<double>(a))
                sa = Linh::to_str(a);
            else if (Linh::holds_alternative<uint64_t>(a))
                sa = std::to_string(Linh::get<uint64_t>(a));
            if (Linh::holds_alternative<std::string>(b))
                sb = Linh::get<std::string>(b);
            else if (Linh::holds_alternative<int64_t>(b))
                sb = std::to_string(Linh::get<int64_t>(b));
            else if (Linh::holds_alternative<double>(b))
                sb = Linh::to_str(b);
            else if (Linh::holds_alternative<uint64_t>(b))
                sb = std::to_string(Linh::get<uint64_t>(b));
            vm.push(sa + sb);
            return;
        }
        // --- KẾT THÚC HỖ TRỢ NỐI CHUỖI ---
        // Ưu tiên xử lý uint64_t trước
        if (Linh::holds_alternative<uint64_t>(a) && Linh::holds_alternative<uint64_t>(b))
        {
            uint64_t av = Linh::get<uint64_t>(a);
            uint64_t bv = Linh::get<uint64_t>(b);
            switch (instr.opcode)
            {
            case OpCode::ADD:
                vm.push(av + bv);
                break;
            case OpCode::SUB:
                vm.push(av - bv);
                break;
            case OpCode::MUL:
                vm.push(av * bv);
                break;
            case OpCode::DIV:
                if (bv == 0)
                {
                    std::string err_msg = "Division by zero (uint)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                vm.push(av / bv);
                break;
            case OpCode::MOD:
                if (bv == 0)
                {
                    std::string err_msg = "Modulo by zero (uint)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                vm.push(av % bv);
                break;
            case OpCode::HASH:
                if (bv == 0)
                {
                    std::string err_msg = "Floor division by zero (uint)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                // Floor division cho uint giống chia thường
                vm.push(av / bv);
                break;
            case OpCode::AMP:
                vm.push(av & bv);
                break;
            case OpCode::PIPE:
                vm.push(av | bv);
                break;
            case OpCode::CARET:
                vm.push(av ^ bv);
                break;
            case OpCode::LT_LT:
                vm.push(av << bv);
                break;
            case OpCode::GT_GT:
                vm.push(av >> bv);
                break;
            // --- So sánh ---
            case OpCode::LT:
#ifdef _DEBUG
                std::cerr << "[DEBUG][uint64_t LT] a=" << av << ", b=" << bv << ", result=" << (av < bv) << std::endl;
#endif
                vm.push(av < bv);
                break;
            case OpCode::LTE:
#ifdef _DEBUG
                std::cerr << "[DEBUG][uint64_t LTE] a=" << av << ", b=" << bv << ", result=" << (av <= bv) << std::endl;
#endif
                vm.push(av <= bv);
                break;
            case OpCode::GT:
#ifdef _DEBUG
                std::cerr << "[DEBUG][uint64_t GT] a=" << av << ", b=" << bv << ", result=" << (av > bv) << std::endl;
#endif
                vm.push(av > bv);
                break;
            case OpCode::GTE:
#ifdef _DEBUG
                std::cerr << "[DEBUG][uint64_t GTE] a=" << av << ", b=" << bv << ", result=" << (av >= bv) << std::endl;
#endif
                vm.push(av >= bv);
                break;
            case OpCode::EQ:
#ifdef _DEBUG
                std::cerr << "[DEBUG][uint64_t EQ] a=" << av << ", b=" << bv << ", result=" << (av == bv) << std::endl;
#endif
                vm.push(av == bv);
                break;
            case OpCode::NEQ:
#ifdef _DEBUG
                std::cerr << "[DEBUG][uint64_t NEQ] a=" << av << ", b=" << bv << ", result=" << (av != bv) << std::endl;
#endif
                vm.push(av != bv);
                break;
            default:
                break;
            }
        }
        else if (Linh::holds_alternative<int64_t>(a) && Linh::holds_alternative<int64_t>(b))
        {
            int64_t av = Linh::get<int64_t>(a);
            int64_t bv = Linh::get<int64_t>(b);
            switch (instr.opcode)
            {
            case OpCode::ADD:
                vm.push(av + bv);
                break;
            case OpCode::SUB:
                vm.push(av - bv);
                break;
            case OpCode::MUL:
                vm.push(av * bv);
                break;
            case OpCode::DIV:
            case OpCode::MOD:
                if ((instr.opcode == OpCode::DIV || instr.opcode == OpCode::MOD) && bv == 0)
                {
                    std::string err_msg = "Division by zero (int)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                if (instr.opcode == OpCode::DIV)
                    vm.push(av / bv);
                else
                    vm.push(av % bv);
                break;
            case OpCode::HASH:
                if (bv == 0)
                {
                    std::string err_msg = "Floor division by zero (int)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                // Python-like floor division for int
                if ((av < 0) != (bv < 0) && av % bv != 0)
                    vm.push((av / bv) - 1);
                else
                    vm.push(av / bv);
                break;
            case OpCode::AMP:
                vm.push(av & bv);
                break;
            case OpCode::PIPE:
                vm.push(av | bv);
                break;
            case OpCode::CARET:
                vm.push(av ^ bv);
                break;
            case OpCode::LT_LT:
                vm.push(av << bv);
                break;
            case OpCode::GT_GT:
                vm.push(av >> bv);
                break;
            // --- So sánh ---
            case OpCode::LT:
#ifdef _DEBUG
                std::cerr << "[DEBUG][int64_t LT] a=" << av << ", b=" << bv << ", result=" << (av < bv) << std::endl;
#endif
                vm.push(av < bv);
                break;
            case OpCode::LTE:
#ifdef _DEBUG
                std::cerr << "[DEBUG][int64_t LTE] a=" << av << ", b=" << bv << ", result=" << (av <= bv) << std::endl;
#endif
                vm.push(av <= bv);
                break;
            case OpCode::GT:
#ifdef _DEBUG
                std::cerr << "[DEBUG][int64_t GT] a=" << av << ", b=" << bv << ", result=" << (av > bv) << std::endl;
#endif
                vm.push(av > bv);
                break;
            case OpCode::GTE:
#ifdef _DEBUG
                std::cerr << "[DEBUG][int64_t GTE] a=" << av << ", b=" << bv << ", result=" << (av >= bv) << std::endl;
#endif
                vm.push(av >= bv);
                break;
            case OpCode::EQ:
#ifdef _DEBUG
                std::cerr << "[DEBUG][int64_t EQ] a=" << av << ", b=" << bv << ", result=" << (av == bv) << std::endl;
#endif
                vm.push(av == bv);
                break;
            case OpCode::NEQ:
#ifdef _DEBUG
                std::cerr << "[DEBUG][int64_t NEQ] a=" << av << ", b=" << bv << ", result=" << (av != bv) << std::endl;
#endif
                vm.push(av != bv);
                break;
            default:
                break;
            }
        }
        else if ((Linh::holds_alternative<int64_t>(a) || Linh::holds_alternative<double>(a)) &&
                 (Linh::holds_alternative<int64_t>(b) || Linh::holds_alternative<double>(b)))
        {
            double av = Linh::holds_alternative<int64_t>(a) ? static_cast<double>(Linh::get<int64_t>(a)) : Linh::get<double>(a);
            double bv = Linh::holds_alternative<int64_t>(b) ? static_cast<double>(Linh::get<int64_t>(b)) : Linh::get<double>(b);
            switch (instr.opcode)
            {
            case OpCode::ADD:
                vm.push(av + bv);
                break;
            case OpCode::SUB:
                vm.push(av - bv);
                break;
            case OpCode::MUL:
                vm.push(av * bv);
                break;
            case OpCode::DIV:
                if (bv == 0.0)
                {
                    std::string err_msg = "Division by zero (float)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                else
                {
                    vm.push(av / bv);
                }
                break;
            case OpCode::MOD:
                if (bv == 0.0)
                {
                    std::string err_msg = "Modulo by zero (float)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                else
                {
                    vm.push(std::fmod(av, bv));
                }
                break;
            case OpCode::HASH:
                if (bv == 0.0)
                {
                    std::string err_msg = "Floor division by zero (float)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                else
                {
                    vm.push(std::floor(av / bv));
                }
                break;
            // --- So sánh ---
            case OpCode::LT:
                vm.push(av < bv);
                break;
            case OpCode::LTE:
                vm.push(av <= bv);
                break;
            case OpCode::GT:
                vm.push(av > bv);
                break;
            case OpCode::GTE:
                vm.push(av >= bv);
                break;
            case OpCode::EQ:
                vm.push(av == bv);
                break;
            case OpCode::NEQ:
                vm.push(av != bv);
                break;
            default:
                break;
            }
        }
        else if ((Linh::holds_alternative<int64_t>(a) && Linh::holds_alternative<uint64_t>(b)) ||
                 (Linh::holds_alternative<uint64_t>(a) && Linh::holds_alternative<int64_t>(b)))
        {
            // Trộn int và uint: thăng cấp sang double để thực hiện số học và so sánh an toàn
            double av = Linh::holds_alternative<int64_t>(a) ? static_cast<double>(Linh::get<int64_t>(a)) : static_cast<double>(Linh::get<uint64_t>(a));
            double bv = Linh::holds_alternative<int64_t>(b) ? static_cast<double>(Linh::get<int64_t>(b)) : static_cast<double>(Linh::get<uint64_t>(b));
            switch (instr.opcode)
            {
            case OpCode::ADD:
                vm.push(av + bv);
                break;
            case OpCode::SUB:
                vm.push(av - bv);
                break;
            case OpCode::MUL:
                vm.push(av * bv);
                break;
            case OpCode::DIV:
                if (bv == 0.0)
                {
                    std::string err_msg = "Division by zero (int/uint)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                else
                {
                    vm.push(av / bv);
                }
                break;
            case OpCode::MOD:
                if (bv == 0.0)
                {
                    std::string err_msg = "Modulo by zero (int/uint)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                else
                {
                    vm.push(std::fmod(av, bv));
                }
                break;
            case OpCode::HASH:
                if (bv == 0.0)
                {
                    std::string err_msg = "Floor division by zero (int/uint)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                else
                {
                    vm.push(std::floor(av / bv));
                }
                break;
            // --- So sánh ---
            case OpCode::LT:
                vm.push(av < bv);
                break;
            case OpCode::LTE:
                vm.push(av <= bv);
                break;
            case OpCode::GT:
                vm.push(av > bv);
                break;
            case OpCode::GTE:
                vm.push(av >= bv);
                break;
            case OpCode::EQ:
                vm.push(av == bv);
                break;
            case OpCode::NEQ:
                vm.push(av != bv);
                break;
            default:
                break;
            }
        }
        else if ((Linh::holds_alternative<uint64_t>(a) || Linh::holds_alternative<double>(a)) &&
                 (Linh::holds_alternative<uint64_t>(b) || Linh::holds_alternative<double>(b)))
        {
            // Nếu một bên là double thì ép sang double, còn lại đã xử lý uint64_t ở trên
            double av = Linh::holds_alternative<uint64_t>(a) ? static_cast<double>(Linh::get<uint64_t>(a)) : Linh::get<double>(a);
            double bv = Linh::holds_alternative<uint64_t>(b) ? static_cast<double>(Linh::get<uint64_t>(b)) : Linh::get<double>(b);
            switch (instr.opcode)
            {
            case OpCode::ADD:
                vm.push(av + bv);
                break;
            case OpCode::SUB:
                vm.push(av - bv);
                break;
            case OpCode::MUL:
                vm.push(av * bv);
                break;
            case OpCode::DIV:
                if (bv == 0.0)
                {
                    std::string err_msg = "Division by zero (float/uint)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                else
                {
                    vm.push(av / bv);
                }
                break;
            case OpCode::MOD:
                if (bv == 0.0)
                {
                    std::string err_msg = "Modulo by zero (float/uint)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                else
                {
                    vm.push(std::fmod(av, bv));
                }
                break;
            case OpCode::HASH:
                if (bv == 0.0)
                {
                    std::string err_msg = "Floor division by zero (float/uint)";
#ifdef _DEBUG
                    std::cerr << err_msg << std::endl;
#endif
                    throw std::runtime_error(err_msg);
                }
                else
                {
                    vm.push(std::floor(av / bv));
                }
                break;
            // --- So sánh ---
            case OpCode::LT:
                vm.push(av < bv);
                break;
            case OpCode::LTE:
                vm.push(av <= bv);
                break;
            case OpCode::GT:
                vm.push(av > bv);
                break;
            case OpCode::GTE:
                vm.push(av >= bv);
                break;
            case OpCode::EQ:
                vm.push(av == bv);
                break;
            case OpCode::NEQ:
                vm.push(av != bv);
                break;
            default:
                break;
            }
        }
        else if (Linh::holds_alternative<std::string>(a) && Linh::holds_alternative<std::string>(b))
        {
            const std::string &av = Linh::get<std::string>(a);
            const std::string &bv = Linh::get<std::string>(b);
            switch (instr.opcode)
            {
            case OpCode::EQ:
                vm.push(av == bv);
                break;
            case OpCode::NEQ:
                vm.push(av != bv);
                break;
            case OpCode::LT:
                vm.push(av < bv);
                break;
            case OpCode::LTE:
                vm.push(av <= bv);
                break;
            case OpCode::GT:
                vm.push(av > bv);
                break;
            case OpCode::GTE:
                vm.push(av >= bv);
                break;
            default:
                vm.push(Value());
                break;
            }
        }
        else if (Linh::holds_alternative<bool>(a) && Linh::holds_alternative<bool>(b))
        {
            bool av = Linh::get<bool>(a);
            bool bv = Linh::get<bool>(b);
            switch (instr.opcode)
            {
            case OpCode::EQ:
                vm.push(av == bv);
                break;
            case OpCode::NEQ:
                vm.push(av != bv);
                break;
            default:
                vm.push(Value());
                break;
            }
        }
        else
        {
#ifdef _DEBUG
            std::cerr << "Invalid operand types for arithmetic or comparison" << std::endl;
#endif
            vm.push(Value());
        }
    }
}
