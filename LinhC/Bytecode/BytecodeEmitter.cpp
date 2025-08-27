#include "BytecodeEmitter.hpp"
#include <unordered_set>
#include <iostream>
#include "../../LiVM/Value/Value.hpp" // Để sử dụng Value cho constant folding

namespace Linh
{
    BytecodeEmitter::BytecodeEmitter() {}

    // Constant folding implementation
    std::optional<Value> BytecodeEmitter::try_constant_fold(AST::BinaryExpr* expr) {
        if (!constant_folding_enabled) return std::nullopt;
        
        // Check if both operands are constants
        if (!is_constant_expression(expr->left.get()) || !is_constant_expression(expr->right.get())) {
            return std::nullopt;
        }
        
        // Get constant values
        auto left_literal = dynamic_cast<AST::LiteralExpr*>(expr->left.get());
        auto right_literal = dynamic_cast<AST::LiteralExpr*>(expr->right.get());
        
        if (!left_literal || !right_literal) return std::nullopt;
        
        // Perform constant folding based on operator
        switch (expr->op.type) {
            case TokenType::PLUS: {
                if (std::holds_alternative<int64_t>(left_literal->value) && 
                    std::holds_alternative<int64_t>(right_literal->value)) {
                    return Value(std::get<int64_t>(left_literal->value) + std::get<int64_t>(right_literal->value));
                }
                if (std::holds_alternative<double>(left_literal->value) && 
                    std::holds_alternative<double>(right_literal->value)) {
                    return Value(std::get<double>(left_literal->value) + std::get<double>(right_literal->value));
                }
                break;
            }
            case TokenType::MINUS: {
                if (std::holds_alternative<int64_t>(left_literal->value) && 
                    std::holds_alternative<int64_t>(right_literal->value)) {
                    return Value(std::get<int64_t>(left_literal->value) - std::get<int64_t>(right_literal->value));
                }
                if (std::holds_alternative<double>(left_literal->value) && 
                    std::holds_alternative<double>(right_literal->value)) {
                    return Value(std::get<double>(left_literal->value) - std::get<double>(right_literal->value));
                }
                break;
            }
            case TokenType::STAR: {
                if (std::holds_alternative<int64_t>(left_literal->value) && 
                    std::holds_alternative<int64_t>(right_literal->value)) {
                    return Value(std::get<int64_t>(left_literal->value) * std::get<int64_t>(right_literal->value));
                }
                if (std::holds_alternative<double>(left_literal->value) && 
                    std::holds_alternative<double>(right_literal->value)) {
                    return Value(std::get<double>(left_literal->value) * std::get<double>(right_literal->value));
                }
                break;
            }
            case TokenType::SLASH: {
                if (std::holds_alternative<int64_t>(left_literal->value) && 
                    std::holds_alternative<int64_t>(right_literal->value)) {
                    auto right_val = std::get<int64_t>(right_literal->value);
                    if (right_val != 0) {
                        return Value(std::get<int64_t>(left_literal->value) / right_val);
                    }
                }
                if (std::holds_alternative<double>(left_literal->value) && 
                    std::holds_alternative<double>(right_literal->value)) {
                    auto right_val = std::get<double>(right_literal->value);
                    if (right_val != 0.0) {
                        return Value(std::get<double>(left_literal->value) / right_val);
                    }
                }
                break;
            }
            case TokenType::EQ_EQ: {
                if (left_literal->value == right_literal->value) {
                    return Value(true);
                } else {
                    return Value(false);
                }
            }
            case TokenType::NOT_EQ: {
                if (left_literal->value != right_literal->value) {
                    return Value(true);
                } else {
                    return Value(false);
                }
            }
            case TokenType::LT: {
                if (std::holds_alternative<int64_t>(left_literal->value) && 
                    std::holds_alternative<int64_t>(right_literal->value)) {
                    return Value(std::get<int64_t>(left_literal->value) < std::get<int64_t>(right_literal->value));
                }
                if (std::holds_alternative<double>(left_literal->value) && 
                    std::holds_alternative<double>(right_literal->value)) {
                    return Value(std::get<double>(left_literal->value) < std::get<double>(right_literal->value));
                }
                break;
            }
            case TokenType::GT: {
                if (std::holds_alternative<int64_t>(left_literal->value) && 
                    std::holds_alternative<int64_t>(right_literal->value)) {
                    return Value(std::get<int64_t>(left_literal->value) > std::get<int64_t>(right_literal->value));
                }
                if (std::holds_alternative<double>(left_literal->value) && 
                    std::holds_alternative<double>(right_literal->value)) {
                    return Value(std::get<double>(left_literal->value) > std::get<double>(right_literal->value));
                }
                break;
            }
        }
        
        return std::nullopt;
    }
    
    std::optional<Value> BytecodeEmitter::try_constant_fold(AST::UnaryExpr* expr) {
        if (!constant_folding_enabled) return std::nullopt;
        
        if (!is_constant_expression(expr->right.get())) {
            return std::nullopt;
        }
        
        auto literal = dynamic_cast<AST::LiteralExpr*>(expr->right.get());
        if (!literal) return std::nullopt;
        
        switch (expr->op.type) {
            case TokenType::MINUS: {
                if (std::holds_alternative<int64_t>(literal->value)) {
                    return Value(-std::get<int64_t>(literal->value));
                }
                if (std::holds_alternative<double>(literal->value)) {
                    return Value(-std::get<double>(literal->value));
                }
                break;
            }
            case TokenType::NOT:
            case TokenType::NOT_KW: {
                if (std::holds_alternative<bool>(literal->value)) {
                    return Value(!std::get<bool>(literal->value));
                }
                break;
            }
        }
        
        return std::nullopt;
    }
    
    bool BytecodeEmitter::is_constant_expression(AST::Expr* expr) {
        return dynamic_cast<AST::LiteralExpr*>(expr) != nullptr;
    }
    
    bool BytecodeEmitter::is_dead_code(AST::Stmt* stmt) {
        if (!dead_code_elimination_enabled) return false;
        
        // Check for unreachable code after return/break/continue
        // This is a simplified implementation
        return false;
    }
    
    void BytecodeEmitter::remove_dead_code(AST::StmtList& stmts) {
        if (!dead_code_elimination_enabled) return;
        AST::StmtList new_stmts;
        bool unreachable = false;
        for (auto& stmt : stmts) {
            if (unreachable) continue;
            new_stmts.push_back(std::move(stmt));
            // Nếu là return, break, continue thì các statement sau đó là unreachable
            if (dynamic_cast<AST::ReturnStmt*>(new_stmts.back().get()) ||
                dynamic_cast<AST::BreakStmt*>(new_stmts.back().get()) ||
                dynamic_cast<AST::ContinueStmt*>(new_stmts.back().get())) {
                unreachable = true;
            }
        }
        stmts = std::move(new_stmts);
    }

    void BytecodeEmitter::emit(const AST::StmtList &stmts)
    {
        chunk.clear();
        // --- Emit all statements including function definitions ---
#ifdef _DEBUG
        std::cerr << "[DEBUG] BytecodeEmitter::emit: processing " << stmts.size() << " statements" << std::endl;
#endif
        for (const auto &stmt : stmts)
        {
            if (stmt) {
#ifdef _DEBUG
                std::cerr << "[DEBUG] BytecodeEmitter::emit: processing statement type" << std::endl;
#endif
                stmt->accept(this);
            }
        }
        emit_instr(OpCode::HALT);
    }

    void BytecodeEmitter::emit_instr(OpCode op, BytecodeValue val, int line, int col)
    {
        chunk.emplace_back(op, val, line, col);
    }

    int BytecodeEmitter::get_var_index(const std::string &name)
    {
        auto it = var_table.find(name);
        if (it != var_table.end())
            return it->second;
        int idx = next_var_index++;
        var_table[name] = idx;
        return idx;
    }

    // --- ExprVisitor ---
    std::any BytecodeEmitter::visitLiteralExpr(AST::LiteralExpr *expr)
    {
        if (std::holds_alternative<int64_t>(expr->value))
            emit_instr(OpCode::PUSH_INT, std::get<int64_t>(expr->value), expr->getLine(), expr->getCol());
        else if (std::holds_alternative<uint64_t>(expr->value))
            emit_instr(OpCode::PUSH_UINT, std::get<uint64_t>(expr->value), expr->getLine(), expr->getCol());
        else if (std::holds_alternative<double>(expr->value))
            emit_instr(OpCode::PUSH_FLOAT, std::get<double>(expr->value), expr->getLine(), expr->getCol());
        else if (std::holds_alternative<std::string>(expr->value))
            emit_instr(OpCode::PUSH_STR, std::get<std::string>(expr->value), expr->getLine(), expr->getCol());
        else if (std::holds_alternative<bool>(expr->value))
            emit_instr(OpCode::PUSH_BOOL, std::get<bool>(expr->value), expr->getLine(), expr->getCol());
        return {};
    }

    std::any BytecodeEmitter::visitIdentifierExpr(AST::IdentifierExpr *expr)
    {
        // Default behavior: load variable. Special handling is done in visitSubscriptExpr for index identifiers.
        emit_instr(OpCode::LOAD_VAR, get_var_index(expr->name.lexeme), expr->getLine(), expr->getCol());
        return {};
    }

    std::any BytecodeEmitter::visitBinaryExpr(AST::BinaryExpr *expr)
    {
        // Try constant folding first
        auto folded_result = try_constant_fold(expr);
        if (folded_result.has_value()) {
            // Emit the constant result directly
            if (std::holds_alternative<int64_t>(*folded_result)) {
                emit_instr(OpCode::PUSH_INT, std::get<int64_t>(*folded_result), expr->getLine(), expr->getCol());
            } else if (std::holds_alternative<double>(*folded_result)) {
                emit_instr(OpCode::PUSH_FLOAT, std::get<double>(*folded_result), expr->getLine(), expr->getCol());
            } else if (std::holds_alternative<bool>(*folded_result)) {
                emit_instr(OpCode::PUSH_BOOL, std::get<bool>(*folded_result), expr->getLine(), expr->getCol());
            }
            return {};
        }
        
        // Evaluate left and right
        if (expr->left)
            expr->left->accept(this);
        if (expr->right)
            expr->right->accept(this);

        int line = expr->op.line;
        int col = expr->op.column_start;
        switch (expr->op.type)
        {
        case TokenType::PLUS:
            emit_instr(OpCode::ADD, {}, line, col);
            break;
        case TokenType::MINUS:
            emit_instr(OpCode::SUB, {}, line, col);
            break;
        case TokenType::STAR:
            emit_instr(OpCode::MUL, {}, line, col);
            break;
        case TokenType::SLASH:
            emit_instr(OpCode::DIV, {}, line, col);
            break;
        case TokenType::PERCENT:
            emit_instr(OpCode::MOD, {}, line, col);
            break;
        case TokenType::STAR_STAR:
            emit_instr(OpCode::CALL, std::string("pow"), line, col);
            break;
        case TokenType::EQ_EQ:
            emit_instr(OpCode::EQ, {}, line, col);
            break;
        case TokenType::NOT_EQ:
            emit_instr(OpCode::NEQ, {}, line, col);
            break;
        case TokenType::LT:
            emit_instr(OpCode::LT, {}, line, col);
            break;
        case TokenType::GT:
            emit_instr(OpCode::GT, {}, line, col);
            break;
        case TokenType::LT_EQ:
            emit_instr(OpCode::LTE, {}, line, col);
            break;
        case TokenType::GT_EQ:
            emit_instr(OpCode::GTE, {}, line, col);
            break;
        case TokenType::AND_LOGIC:
        case TokenType::AND_KW:
            emit_instr(OpCode::AND, {}, line, col);
            break;
        case TokenType::OR_LOGIC:
        case TokenType::OR_KW:
            emit_instr(OpCode::OR, {}, line, col);
            break;
        case TokenType::HASH:
            emit_instr(OpCode::HASH, {}, line, col);
            break;
        case TokenType::PIPE:
            emit_instr(OpCode::PIPE, {}, line, col);
            break;
        case TokenType::CARET:
            emit_instr(OpCode::CARET, {}, line, col);
            break;
        case TokenType::AMP:
            emit_instr(OpCode::AMP, {}, line, col);
            break;
        case TokenType::LT_LT:
            emit_instr(OpCode::LT_LT, {}, line, col);
            break;
        case TokenType::GT_GT:
            emit_instr(OpCode::GT_GT, {}, line, col);
            break;
        default:
            break;
        }
        return {};
    }

    std::any BytecodeEmitter::visitUnaryExpr(AST::UnaryExpr *expr)
    {
        // Try constant folding first
        auto folded_result = try_constant_fold(expr);
        if (folded_result.has_value()) {
            // Emit the constant result directly
            if (std::holds_alternative<int64_t>(*folded_result)) {
                emit_instr(OpCode::PUSH_INT, std::get<int64_t>(*folded_result), expr->getLine(), expr->getCol());
            } else if (std::holds_alternative<double>(*folded_result)) {
                emit_instr(OpCode::PUSH_FLOAT, std::get<double>(*folded_result), expr->getLine(), expr->getCol());
            } else if (std::holds_alternative<bool>(*folded_result)) {
                emit_instr(OpCode::PUSH_BOOL, std::get<bool>(*folded_result), expr->getLine(), expr->getCol());
            }
            return {};
        }
        
        if (expr->right)
            expr->right->accept(this);
        switch (expr->op.type)
        {
        case TokenType::MINUS:
            emit_instr(OpCode::PUSH_INT, 0, expr->getLine(), expr->getCol());
            emit_instr(OpCode::SWAP, {}, expr->getLine(), expr->getCol()); // Not defined, but you may need to implement SWAP for correct order
            emit_instr(OpCode::SUB, {}, expr->getLine(), expr->getCol());
            break;
        case TokenType::NOT:
        case TokenType::NOT_KW:
            emit_instr(OpCode::NOT, {}, expr->getLine(), expr->getCol());
            break;
        case TokenType::TILDE:
            emit_instr(OpCode::NOT); // <-- Thêm dòng này cho ~
            break;
        default:
            break;
        }
        return {};
    }

    std::any BytecodeEmitter::visitGroupingExpr(AST::GroupingExpr *expr)
    {
        if (expr->expression)
            expr->expression->accept(this);
        return {};
    }

    std::any BytecodeEmitter::visitAssignmentExpr(AST::AssignmentExpr *expr)
    {
#ifdef _DEBUG
        std::cerr << "[DEBUG] visitAssignmentExpr: name=" << expr->name.lexeme << std::endl;
#endif
        // Evaluate value and store to variable
        if (expr->value)
        {
            expr->value->accept(this);
        }
        int idx = get_var_index(expr->name.lexeme);
        emit_instr(OpCode::STORE_VAR, idx, expr->getLine(), expr->getCol());
        // Không emit LOAD_VAR ở đây (tránh dư stack cho for-loop)
        return {};
    }

    std::any BytecodeEmitter::visitLogicalExpr(AST::LogicalExpr *expr)
    {
        // Short-circuit logic not implemented, fallback to eager evaluation
        if (expr->left)
            expr->left->accept(this);
        if (expr->right)
            expr->right->accept(this);
        switch (expr->op.type)
        {
        case TokenType::AND_LOGIC:
        case TokenType::AND_KW:
            emit_instr(OpCode::AND, {}, expr->getLine(), expr->getCol());
            break;
        case TokenType::OR_LOGIC:
        case TokenType::OR_KW:
            emit_instr(OpCode::OR, {}, expr->getLine(), expr->getCol());
            break;
        default:
            break;
        }
        return {};
    }

    void BytecodeEmitter::visitPrintStmt(AST::PrintStmt *stmt)
    {
        // Emit code for all expressions
        for (const auto& expr : stmt->expressions) {
            if (expr) {
                expr->accept(this);
            }
        }
        
        // Use PRINT_MULTIPLE if there are multiple expressions, otherwise use PRINT
        if (stmt->expressions.size() > 1) {
            emit_instr(OpCode::PRINT_MULTIPLE, static_cast<int64_t>(stmt->expressions.size()), stmt->getLine(), stmt->getCol());
        } else {
            emit_instr(OpCode::PRINT, {}, stmt->getLine(), stmt->getCol());
        }
    }

    void BytecodeEmitter::visitExpressionStmt(AST::ExpressionStmt *stmt)
    {
        if (stmt->expression)
            stmt->expression->accept(this);

        // --- Sửa tại đây: Không sinh POP nếu là print(...) hoặc là increment trong for-loop ---
        auto call = dynamic_cast<AST::CallExpr *>(stmt->expression.get());
        if (call)
        {
            auto id = dynamic_cast<AST::IdentifierExpr *>(call->callee.get());
            if (id && (id->name.lexeme == "print" || id->name.lexeme == "printil"))
            {
                // Không sinh POP cho print/printil(...)
                return;
            }
        }
        // Không sinh POP nếu expression là dạng a = a + 1 (increment trong for)
        // Đơn giản: nếu là AssignmentExpr thì không POP
        if (dynamic_cast<AST::AssignmentExpr *>(stmt->expression.get()))
        {
            return;
        }
        emit_instr(OpCode::POP, {}, stmt->getLine(), stmt->getCol());
    }

    void BytecodeEmitter::visitVarDeclStmt(AST::VarDeclStmt *stmt)
    {
        int idx = get_var_index(stmt->name.lexeme);
        if (stmt->initializer)
            stmt->initializer->accept(this);
        else
            emit_instr(OpCode::PUSH_INT, 0, stmt->getLine(), stmt->getCol()); // default 0
        emit_instr(OpCode::STORE_VAR, idx, stmt->getLine(), stmt->getCol());
    }

    void BytecodeEmitter::visitBlockStmt(AST::BlockStmt *stmt)
    {
        for (const auto &s : stmt->statements)
        {
            if (s)
                s->accept(this);
        }
    }

    void BytecodeEmitter::visitIfStmt(AST::IfStmt *stmt)
    {
        // if (condition) then_branch else else_branch
        // [condition]
        // JMP_IF_FALSE else_branch
        // [then_branch]
        // JMP end
        // else_branch:
        // [else_branch]
        // end:

        // Evaluate condition
        if (stmt->condition)
            stmt->condition->accept(this);

        // Placeholder for JMP_IF_FALSE to else branch
        size_t jmp_if_false_pos = chunk.size();
        emit_instr(OpCode::JMP_IF_FALSE, int64_t(-1), stmt->getLine(), stmt->getCol()); // placeholder

        // Emit then branch
        if (stmt->then_branch)
            stmt->then_branch->accept(this);

        // Jump to end (skip else branch)
        size_t jmp_to_end_pos = chunk.size();
        emit_instr(OpCode::JMP, int64_t(-1), stmt->getLine(), stmt->getCol()); // placeholder

        // Else branch position
        size_t else_pos = chunk.size();
        if (stmt->else_branch)
            stmt->else_branch->accept(this);

        // End position
        size_t end_pos = chunk.size();

        // Fix JMP_IF_FALSE to jump to else branch
        chunk[jmp_if_false_pos].operand = int64_t(else_pos);

        // Fix JMP to jump to end
        chunk[jmp_to_end_pos].operand = int64_t(end_pos);
    }
    void BytecodeEmitter::visitWhileStmt(AST::WhileStmt *stmt)
    {
        // Tối ưu hóa: Nếu điều kiện là hằng false, bỏ qua body
        if (stmt->condition) {
            auto literal = dynamic_cast<AST::LiteralExpr*>(stmt->condition.get());
            if (literal) {
                bool always_false = false;
                if (std::holds_alternative<bool>(literal->value)) always_false = !std::get<bool>(literal->value);
                else if (std::holds_alternative<int64_t>(literal->value)) always_false = std::get<int64_t>(literal->value) == 0;
                else if (std::holds_alternative<double>(literal->value)) always_false = std::get<double>(literal->value) == 0.0;
                else if (std::holds_alternative<std::string>(literal->value)) always_false = std::get<std::string>(literal->value).empty();
                if (always_false) {
                    // Không sinh code cho body, chỉ nhảy qua
                    return;
                }
            }
        }
        // Bình thường
        size_t cond_pos = chunk.size();
        if (stmt->condition)
            stmt->condition->accept(this);
        size_t jmp_if_false_pos = chunk.size();
        emit_instr(OpCode::JMP_IF_FALSE, int64_t(-1), stmt->getLine(), stmt->getCol()); // placeholder
        if (stmt->body)
            stmt->body->accept(this);
        emit_instr(OpCode::JMP, int64_t(cond_pos), stmt->getLine(), stmt->getCol());
        size_t end_pos = chunk.size();
        chunk[jmp_if_false_pos].operand = int64_t(end_pos);
    }

    void BytecodeEmitter::visitDoWhileStmt(AST::DoWhileStmt *stmt)
    {
        size_t loop_start = chunk.size();
        if (stmt->body)
            stmt->body->accept(this);
        // Tối ưu hóa: Nếu điều kiện là hằng false, không sinh JMP_IF_TRUE
        if (stmt->condition) {
            auto literal = dynamic_cast<AST::LiteralExpr*>(stmt->condition.get());
            bool always_false = false;
            if (literal) {
                if (std::holds_alternative<bool>(literal->value)) always_false = !std::get<bool>(literal->value);
                else if (std::holds_alternative<int64_t>(literal->value)) always_false = std::get<int64_t>(literal->value) == 0;
                else if (std::holds_alternative<double>(literal->value)) always_false = std::get<double>(literal->value) == 0.0;
                else if (std::holds_alternative<std::string>(literal->value)) always_false = std::get<std::string>(literal->value).empty();
            }
            if (!always_false)
                stmt->condition->accept(this);
            if (!always_false)
                emit_instr(OpCode::JMP_IF_TRUE, int64_t(loop_start), stmt->getLine(), stmt->getCol());
        }
    }

    void BytecodeEmitter::visitFunctionDeclStmt(AST::FunctionDeclStmt *stmt) {
        std::vector<FunctionParameter> function_params;
        
        // Chuyển đổi từ FuncParamNode sang FunctionParameter
        for (const auto& param : stmt->params) {
            std::optional<std::string> param_type = std::nullopt;
            
            if (param.type.has_value()) {
                // Chuyển đổi TypeNode thành string
                // TODO: Implement proper type string conversion
                param_type = "dynamic"; // Placeholder
            }
            
            function_params.emplace_back(param.name.lexeme, param_type, param.is_static);
        }
        
        // Tạo bytecode cho thân hàm
        BytecodeChunk function_body;
        {
            BytecodeEmitter body_emitter;
            if (stmt->body) {
                for (const auto& body_stmt : stmt->body->statements) {
                    if (body_stmt) {
                        body_stmt->accept(&body_emitter);
                    }
                }
            }
            
            // Thêm SWAP instruction nếu có CALL và stack có 2 elements
            // Điều này đảm bảo function object ở trên cùng trước khi gọi CALL
            if (!body_emitter.chunk.empty()) {
                for (size_t i = 0; i < body_emitter.chunk.size(); ++i) {
                    if (body_emitter.chunk[i].opcode == OpCode::CALL) {
                        // Thêm SWAP trước CALL để đảm bảo function object ở trên cùng
                        body_emitter.chunk.insert(body_emitter.chunk.begin() + i, 
                            Instruction{OpCode::SWAP, {}, stmt->getLine(), stmt->getCol()});
                        break;
                    }
                }
            }
            
            // Thêm RET instruction nếu không có return statement
            if (body_emitter.chunk.empty() || body_emitter.chunk.back().opcode != OpCode::RET) {
                body_emitter.emit_instr(OpCode::RET, {}, stmt->getLine(), stmt->getCol());
            }
            function_body = body_emitter.chunk; // Gán lại đúng
        }
        
        // Tạo FunctionObject với thân hàm
#ifdef _DEBUG
        std::cerr << "[DEBUG] visitFunctionDeclStmt: function body has " << function_body.size() << " instructions" << std::endl;
        for (size_t i = 0; i < function_body.size(); ++i) {
            std::cerr << "[DEBUG] Function " << stmt->name.lexeme << " body[" << i << "]: " << static_cast<int>(function_body[i].opcode) << std::endl;
        }
#endif
        auto fn = create_function(stmt->name.lexeme, function_params, function_body);
        
        // Lưu function object vào biến
        int var_idx = get_var_index(stmt->name.lexeme);
        
        // Push function object lên stack
#ifdef _DEBUG
        std::cerr << "[DEBUG] visitFunctionDeclStmt: creating function object for " << stmt->name.lexeme << std::endl;
#endif
        emit_instr(OpCode::PUSH_FUNCTION, fn, stmt->getLine(), stmt->getCol());
        
        // Store vào biến
        emit_instr(OpCode::STORE_VAR, var_idx, stmt->getLine(), stmt->getCol());
    }
    void BytecodeEmitter::visitReturnStmt(AST::ReturnStmt *stmt)
    {
        if (stmt->value)
            stmt->value->accept(this);
        emit_instr(OpCode::RET, {}, stmt->getLine(), stmt->getCol());
    }
    void BytecodeEmitter::visitBreakStmt(AST::BreakStmt *) {}
    void BytecodeEmitter::visitContinueStmt(AST::ContinueStmt *) {}
    void BytecodeEmitter::visitSwitchStmt(AST::SwitchStmt *stmt)
    {
        // --- Sinh bytecode cho switch-case ---
        if (stmt->expression_to_switch_on)
            stmt->expression_to_switch_on->accept(this);

        size_t case_count = stmt->cases.size();
        std::vector<size_t> case_jump_addrs(case_count, 0);
        std::vector<size_t> case_body_addrs(case_count, 0);
        size_t default_case_idx = size_t(-1);

        // Để sửa JMP (break) sau khi biết địa chỉ kết thúc switch
        std::vector<size_t> break_jmp_addrs;

        // 2. Sinh code kiểm tra từng case
        for (size_t i = 0; i < case_count; ++i)
        {
            const auto &case_clause = stmt->cases[i];
            if (case_clause.is_default)
            {
                default_case_idx = i;
                continue;
            }
            emit_instr(OpCode::DUP);
            if (case_clause.case_value.has_value() && case_clause.case_value.value())
                case_clause.case_value.value()->accept(this);
            else
                emit_instr(OpCode::PUSH_INT, 0);

            emit_instr(OpCode::EQ);
            size_t jmp_if_true_addr = chunk.size();
            emit_instr(OpCode::JMP_IF_TRUE, int64_t(-1));
            case_jump_addrs[i] = jmp_if_true_addr;
            // Không sinh POP ở đây!
        }

        size_t jmp_default_addr = chunk.size();
        emit_instr(OpCode::JMP, int64_t(-1));

        // 3. Sinh code cho từng case body
        for (size_t i = 0; i < case_count; ++i)
        {
            case_body_addrs[i] = chunk.size();
            if (!stmt->cases[i].is_default)
                chunk[case_jump_addrs[i]].operand = int64_t(case_body_addrs[i]);
            // Chỉ pop switch_value khi thực sự vào case body
            emit_instr(OpCode::POP);

            // Sinh code cho các statement trong case
            for (const auto &s : stmt->cases[i].statements)
            {
                if (s)
                {
                    // Nếu là BreakStmt thì sinh JMP và lưu lại vị trí để sửa sau
                    if (dynamic_cast<AST::BreakStmt *>(s.get()))
                    {
                        size_t break_jmp_addr = chunk.size();
                        emit_instr(OpCode::JMP, int64_t(-1));
                        break_jmp_addrs.push_back(break_jmp_addr);
                        // Sau break thì không sinh code cho các statement tiếp theo trong case
                        break;
                    }
                    s->accept(this);
                }
            }
        }

        // 4. Default case
        size_t default_addr = chunk.size();
        if (default_case_idx != size_t(-1))
        {
            chunk[jmp_default_addr].operand = int64_t(default_addr);
            // Khi vào default, pop switch_value
            emit_instr(OpCode::POP);
            for (const auto &s : stmt->cases[default_case_idx].statements)
            {
                if (s)
                {
                    if (dynamic_cast<AST::BreakStmt *>(s.get()))
                    {
                        size_t break_jmp_addr = chunk.size();
                        emit_instr(OpCode::JMP, int64_t(-1));
                        break_jmp_addrs.push_back(break_jmp_addr);
                        break;
                    }
                    s->accept(this);
                }
            }
        }
        else
        {
            chunk[jmp_default_addr].operand = int64_t(default_addr);
            emit_instr(OpCode::POP);
        }

        // Địa chỉ kết thúc switch
        size_t end_switch_addr = chunk.size();

        // Sửa lại tất cả JMP (break) để nhảy ra ngoài switch
        for (size_t addr : break_jmp_addrs)
        {
            chunk[addr].operand = int64_t(end_switch_addr);
        }
    }

    void BytecodeEmitter::visitDeleteStmt(AST::DeleteStmt *) {}
    void BytecodeEmitter::visitThrowStmt(AST::ThrowStmt *) {}
    void BytecodeEmitter::visitTryStmt(AST::TryStmt *stmt)
    {
        // Đặt nhãn cho catch, finally, end
        size_t try_start = chunk.size();
        size_t catch_pos = 0, finally_pos = 0, end_pos = 0;

        // Đặt biến error_var đặc biệt (ở đây dùng index -9999)
        std::string error_var = "error";

        // Đặt chỗ TRY, sẽ sửa operand sau
        size_t try_instr_pos = chunk.size();
        emit_instr(OpCode::TRY, std::make_tuple(int64_t(0), int64_t(0), int64_t(0), error_var), stmt->keyword_try.line, stmt->keyword_try.column_start);

        // Sinh code cho try_block
        if (stmt->try_block)
            stmt->try_block->accept(this);

        // Sau try_block, nhảy đến finally (nếu có), hoặc end
        size_t after_try = chunk.size();
        emit_instr(OpCode::JMP, int64_t(0), stmt->keyword_try.line, stmt->keyword_try.column_start); // placeholder

        // catch
        catch_pos = chunk.size();
        if (!stmt->catch_clauses.empty())
        {
            // Chỉ lấy catch đầu tiên (giản lược)
            auto &catch_clause = stmt->catch_clauses[0];
            // Gán biến error (index -9999)
            // (Ở đây không sinh code, VM sẽ tự gán khi lỗi)
            if (catch_clause.body)
                catch_clause.body->accept(this);
        }

        // finally
        finally_pos = chunk.size();
        if (stmt->finally_block.has_value() && stmt->finally_block.value())
        {
            stmt->finally_block.value()->accept(this);
        }

        // end
        end_pos = chunk.size();
        emit_instr(OpCode::END_TRY, {}, stmt->keyword_try.line, stmt->keyword_try.column_start);

        // Sửa lại JMP sau try_block để nhảy qua catch đến finally/end
        chunk[after_try].operand = int64_t(finally_pos);

        // Sửa lại TRY operand
        chunk[try_instr_pos].operand = std::make_tuple(
            int64_t(catch_pos),
            int64_t(finally_pos),
            int64_t(end_pos),
            error_var);
    }

    std::any BytecodeEmitter::visitCallExpr(AST::CallExpr *expr)
    {
        // Special case: input(...), type(...), id(...), printil(...)
        if (auto id = dynamic_cast<AST::IdentifierExpr *>(expr->callee.get()))
        {
            if (id->name.lexeme == "input")
            {
                if (!expr->arguments.empty())
                    expr->arguments[0]->accept(this);
                else
                    emit_instr(OpCode::PUSH_STR, std::string(""), expr->getLine(), expr->getCol());
                emit_instr(OpCode::INPUT, {}, expr->getLine(), expr->getCol());
                return {};
            }
            if (id->name.lexeme == "type")
            {
                if (!expr->arguments.empty())
                    expr->arguments[0]->accept(this);
                else
                    emit_instr(OpCode::PUSH_STR, std::string(""), expr->getLine(), expr->getCol());
                emit_instr(OpCode::TYPEOF, {}, expr->getLine(), expr->getCol());
                return {};
            }
            if (id->name.lexeme == "id")
            {
                if (!expr->arguments.empty())
                    expr->arguments[0]->accept(this);
                else
                    emit_instr(OpCode::PUSH_STR, std::string(""), expr->getLine(), expr->getCol());
                emit_instr(OpCode::ID, {}, expr->getLine(), expr->getCol());
                return {};
            }
            if (id->name.lexeme == "printil")
            {
                if (!expr->arguments.empty())
                    expr->arguments[0]->accept(this);
                else
                    emit_instr(OpCode::PUSH_STR, std::string(""), expr->getLine(), expr->getCol());
                emit_instr(OpCode::PRINTF, {}, expr->getLine(), expr->getCol());
                return {};
            }
            if (id->name.lexeme == "bytes")
            {
                if (!expr->arguments.empty())
                    expr->arguments[0]->accept(this);
                else
                    emit_instr(OpCode::PUSH_STR, std::string(""), expr->getLine(), expr->getCol());
                emit_instr(OpCode::CALL, std::string("bytes"), expr->getLine(), expr->getCol());
                return {};
            }
            if (id->name.lexeme == "len")
            {
                if (!expr->arguments.empty())
                    expr->arguments[0]->accept(this);
                else
                    emit_instr(OpCode::PUSH_STR, std::string(""), expr->getLine(), expr->getCol());
                emit_instr(OpCode::CALL, std::string("len"), expr->getLine(), expr->getCol());
                return {};
            }
            if (id->name.lexeme == "bin")
            {
                if (!expr->arguments.empty())
                    expr->arguments[0]->accept(this);
                else
                    emit_instr(OpCode::PUSH_INT, 0, expr->getLine(), expr->getCol());
                emit_instr(OpCode::CALL, std::string("bin"), expr->getLine(), expr->getCol());
                return {};
            }
            // --- User-defined function call ---
            // Emit arguments trước
            for (auto &arg : expr->arguments)
                if (arg)
                    arg->accept(this);
            // Sau đó mới LOAD_VAR cho function object
            emit_instr(OpCode::LOAD_VAR, get_var_index(id->name.lexeme), expr->getLine(), expr->getCol());
            // Cuối cùng CALL
            emit_instr(OpCode::CALL, id->name.lexeme, expr->getLine(), expr->getCol());
            return {};
        }
        // Hỗ trợ a.append(x) và a.remove(x)
        // Nếu callee là MemberExpr (giả sử có AST::MemberExpr hoặc SubscriptExpr)
        // Đơn giản: Nếu callee là dạng a.append hoặc a.remove
        if (auto member = dynamic_cast<AST::MemberExpr *>(expr->callee.get()))
        {
            // Fs package methods: open, read, readline, readlines, write, append, close
            if (auto id = dynamic_cast<AST::IdentifierExpr *>(member->object.get()))
            {
                if (id->name.lexeme == "fs" && 
                    (member->property == "open" || member->property == "read" || member->property == "readline" || member->property == "readlines" || member->property == "write" || member->property == "append" || member->property == "close" || member->property == "encoding" || member->property == "seek" || member->property == "size" || member->property == "exists" || member->property == "bRead" || member->property == "bWrite" || member->property == "bAppend" || member->property == "listdir" || member->property == "mkdir" || member->property == "remove" || member->property == "rmdir" || member->property == "rename" || member->property == "copy" || member->property == "move" || member->property == "stat" || member->property == "isdir" || member->property == "isfile" || member->property == "isempty" || member->property == "isopen" || member->property == "rmall"))
                {
                    // Emit arguments first (if any)
                    for (const auto &arg : expr->arguments)
                        if (arg)
                            arg->accept(this);
                    
                    // Then emit the package function call
                    std::string full_name = "fs." + member->property;
                    emit_instr(OpCode::CALL_PACKAGE_FUNCTION, full_name, expr->getLine(), expr->getCol());
                    return {};
                }
                if (id->name.lexeme == "json" && (member->property == "decode" || member->property == "encode"))
                {
                    for (const auto &arg : expr->arguments)
                        if (arg)
                            arg->accept(this);
                    std::string full_name = "json." + member->property;
                    emit_instr(OpCode::CALL_PACKAGE_FUNCTION, full_name, expr->getLine(), expr->getCol());
                    return {};
                }
                if (id->name.lexeme == "os" && (member->property == "getcwd" || member->property == "chdir"))
                {
                    for (const auto &arg : expr->arguments)
                        if (arg)
                            arg->accept(this);
                    std::string full_name = "os." + member->property;
                    emit_instr(OpCode::CALL_PACKAGE_FUNCTION, full_name, expr->getLine(), expr->getCol());
                    return {};
                }
            }
            
            // member->object: biểu thức array, member->property: tên phương thức
            if (member->property == "append" && expr->arguments.size() == 1)
            {
                // Đánh giá object (array)
                if (member->object)
                    member->object->accept(this);
                // Đánh giá argument (giá trị cần append)
                expr->arguments[0]->accept(this);
                emit_instr(OpCode::ARRAY_APPEND, {}, expr->getLine(), expr->getCol());
                return {};
            }
            if (member->property == "remove" && expr->arguments.size() == 1)
            {
                // Đánh giá object (array)
                if (member->object)
                    member->object->accept(this);
                // Đánh giá argument (giá trị cần remove)
                expr->arguments[0]->accept(this);
                emit_instr(OpCode::ARRAY_REMOVE, {}, expr->getLine(), expr->getCol());
                return {};
            }
            if (member->property == "clear" && expr->arguments.empty())
            {
                if (member->object)
                    member->object->accept(this);
                emit_instr(OpCode::ARRAY_CLEAR, {}, expr->getLine(), expr->getCol());
                return {};
            }
            if (member->property == "clone" && expr->arguments.empty())
            {
                if (member->object)
                    member->object->accept(this);
                emit_instr(OpCode::ARRAY_CLONE, {}, expr->getLine(), expr->getCol());
                return {};
            }
            if (member->property == "pop")
            {
                if (member->object)
                    member->object->accept(this);
                if (expr->arguments.empty())
                {
                    // a.pop()
                    emit_instr(OpCode::ARRAY_POP, {}, expr->getLine(), expr->getCol());
                }
                else if (expr->arguments.size() == 1)
                {
                    // a.pop(index)
                    expr->arguments[0]->accept(this);
                    emit_instr(OpCode::ARRAY_POP, {}, expr->getLine(), expr->getCol());
                }
                return {};
            }
            if (member->property == "delete" && expr->arguments.size() == 1)
            {
                // Đánh giá object (map)
                if (member->object)
                    member->object->accept(this);
                // Đánh giá argument (key)
                expr->arguments[0]->accept(this);
                emit_instr(OpCode::MAP_DELETE, {}, expr->getLine(), expr->getCol());
                return {};
            }
            if (member->property == "clear" && expr->arguments.empty())
            {
                if (member->object)
                    member->object->accept(this);
                emit_instr(OpCode::MAP_CLEAR, {}, expr->getLine(), expr->getCol());
                return {};
            }
            if (member->property == "keys" && expr->arguments.empty())
            {
                if (member->object)
                    member->object->accept(this);
                emit_instr(OpCode::MAP_KEYS, {}, expr->getLine(), expr->getCol());
                return {};
            }
            
            // Package function calls (e.g., time.sleep, math.abs)
            auto id = dynamic_cast<AST::IdentifierExpr *>(member->object.get());
            if (id) {
                std::string package_name = id->name.lexeme;
                std::string function_name = member->property_token.lexeme;
                
                // Check if it's a known package
                if (package_name == "time" || package_name == "math") {
                    // Emit arguments first
                    for (auto &arg : expr->arguments)
                        if (arg)
                            arg->accept(this);
                    
                    // Emit CALL with package.function name
                    std::string full_name = package_name + "." + function_name;
                    emit_instr(OpCode::CALL, full_name, expr->getLine(), expr->getCol());
                    return {};
                }
            }
            
            // Package function calls (e.g., time.sleep, math.abs)
            auto package_id = dynamic_cast<AST::IdentifierExpr *>(member->object.get());
            if (package_id) {
                std::string package_name = package_id->name.lexeme;
                std::string function_name = member->property_token.lexeme;
                
                // Check if it's a known package
                if (package_name == "time" || package_name == "math") {
                    // Emit arguments first
                    for (auto &arg : expr->arguments)
                        if (arg)
                            arg->accept(this);
                    
                    // Emit CALL with package.function name
                    std::string full_name = package_name + "." + function_name;
                    emit_instr(OpCode::CALL, full_name, expr->getLine(), expr->getCol());
                    return {};
                }
            }
            
            if (member->property == "values" && expr->arguments.empty())
            {
                if (member->object)
                    member->object->accept(this);
                emit_instr(OpCode::MAP_VALUES, {}, expr->getLine(), expr->getCol());
                return {};
            }
        }
        // Not implemented yet for other calls
        return {};
    }

    std::any BytecodeEmitter::visitPostfixExpr(AST::PostfixExpr *expr)
    {
        // Hỗ trợ a++ và a--
        // Chỉ hỗ trợ cho IdentifierExpr
        auto id = dynamic_cast<AST::IdentifierExpr *>(expr->operand.get());
        if (!id)
            return {};
        int idx = get_var_index(id->name.lexeme);
        // LOAD_VAR idx
        emit_instr(OpCode::LOAD_VAR, idx, expr->getLine(), expr->getCol());
        // PUSH_INT 1
        emit_instr(OpCode::PUSH_INT, 1, expr->getLine(), expr->getCol());
        if (expr->op_token.type == TokenType::PLUS_PLUS)
            emit_instr(OpCode::ADD, {}, expr->getLine(), expr->getCol());
        else if (expr->op_token.type == TokenType::MINUS_MINUS)
            emit_instr(OpCode::SUB, {}, expr->getLine(), expr->getCol());
        // STORE_VAR idx
        emit_instr(OpCode::STORE_VAR, idx, expr->getLine(), expr->getCol());
        // Optionally, load value back (for expression value)
        emit_instr(OpCode::LOAD_VAR, idx, expr->getLine(), expr->getCol());
        return {};
    }

    std::any BytecodeEmitter::visitUninitLiteralExpr(AST::UninitLiteralExpr *expr)
    {
        // Emit a neutral placeholder then convert it to 'sol' via builtin call.
        // This avoids consuming any previously pushed values (e.g., map keys).
        emit_instr(OpCode::PUSH_INT, int64_t(0), expr->keyword.line, expr->keyword.column_start);
        emit_instr(OpCode::CALL, std::string("sol"), expr->keyword.line, expr->keyword.column_start);
        return {};
    }

    std::any BytecodeEmitter::visitNewExpr(AST::NewExpr *expr)
    {
        // Not implemented yet
        return {};
    }

    std::any BytecodeEmitter::visitThisExpr(AST::ThisExpr *expr)
    {
        // Not implemented yet
        return {};
    }

    std::any BytecodeEmitter::visitArrayLiteralExpr(AST::ArrayLiteralExpr *expr)
    {
        // Emit code cho từng phần tử (theo thứ tự)
        for (const auto &elem : expr->elements)
        {
            if (elem)
                elem->accept(this);
        }
        // Sau đó emit PUSH_ARRAY với số lượng phần tử
        emit_instr(OpCode::PUSH_ARRAY, static_cast<int64_t>(expr->elements.size()), expr->getLine(), expr->getCol());
        return {};
    }

    std::any BytecodeEmitter::visitMapLiteralExpr(AST::MapLiteralExpr *expr)
    {
        // Emit code cho từng key, value (theo thứ tự)
        for (const auto &entry : expr->entries)
        {
            if (entry.key)
                entry.key->accept(this);
            if (entry.value)
                entry.value->accept(this);
        }
        // Sau đó emit PUSH_MAP với số lượng cặp
        emit_instr(OpCode::PUSH_MAP, static_cast<int64_t>(expr->entries.size()), expr->l_brace.line, expr->l_brace.column_start);
        return {};
    }

    std::any BytecodeEmitter::visitSubscriptExpr(AST::SubscriptExpr *expr)
    {
        // Evaluate object first
        if (expr->object)
            expr->object->accept(this);
        
        bool use_map_get = false;
        // Evaluate index: if it's an identifier, treat it as a string key (for maps)
        if (expr->index) {
            if (auto id = dynamic_cast<AST::IdentifierExpr*>(expr->index.get())) {
                emit_instr(OpCode::PUSH_STR, id->name.lexeme, id->getLine(), id->getCol());
                use_map_get = true;
            } else if (auto lit = dynamic_cast<AST::LiteralExpr*>(expr->index.get())) {
                // If index is a string literal, prefer MAP_GET
                if (std::holds_alternative<std::string>(lit->value)) {
                    lit->accept(this);
                    use_map_get = true;
                } else {
                    lit->accept(this);
                }
            } else {
                expr->index->accept(this);
            }
        }
        
        // Choose opcode based on index form: string or identifier -> MAP_GET, otherwise ARRAY_GET
        if (use_map_get) {
            emit_instr(OpCode::MAP_GET, {}, expr->l_bracket_token.line, expr->l_bracket_token.column_start);
        } else {
            emit_instr(OpCode::ARRAY_GET, {}, expr->l_bracket_token.line, expr->l_bracket_token.column_start);
        }
        return {};
    }

    std::any BytecodeEmitter::visitInterpolatedStringExpr(AST::InterpolatedStringExpr *expr)
    {
        // Nếu chỉ có 1 phần là chuỗi thì PUSH_STR luôn
        if (expr->parts.size() == 1 && std::holds_alternative<std::string>(expr->parts[0]))
        {
            emit_instr(OpCode::PUSH_STR, std::get<std::string>(expr->parts[0]), expr->getLine(), expr->getCol());
            return {};
        }
        // Duyệt từng phần, đẩy từng phần lên stack (dưới dạng string)
        for (const auto &part : expr->parts)
        {
            if (std::holds_alternative<std::string>(part))
            {
                emit_instr(OpCode::PUSH_STR, std::get<std::string>(part), expr->getLine(), expr->getCol());
            }
            else
            {
                // Phần là biểu thức, emit cho expr, rồi ép về string bằng cách nối với ""
                auto *subexpr = std::get<AST::ExprPtr>(part).get();
                if (subexpr)
                {
                    // Nếu là MemberExpr và là package constant, emit đúng LOAD_PACKAGE_CONST
                    if (auto member = dynamic_cast<AST::MemberExpr *>(subexpr)) {
                        if (member->is_package_constant) {
                            std::string full_name = member->package_name + "." + member->constant_name;
                            emit_instr(OpCode::LOAD_PACKAGE_CONST, full_name, member->getLine(), member->getCol());
                        } else if (auto id = dynamic_cast<AST::IdentifierExpr *>(member->object.get())) {
                            if (id->name.lexeme == "math") {
                                std::string full_name = id->name.lexeme + "." + member->property_token.lexeme;
                                emit_instr(OpCode::LOAD_PACKAGE_CONST, full_name, member->getLine(), member->getCol());
                            } else {
                                subexpr->accept(this);
                            }
                        } else {
                            subexpr->accept(this);
                        }
                    } else {
                        subexpr->accept(this);
                    }
                    emit_instr(OpCode::PUSH_STR, std::string(""), expr->getLine(), expr->getCol());
                    emit_instr(OpCode::ADD, {}, expr->getLine(), expr->getCol()); // ép về string
                }
            }
        }
        // Nối tất cả lại thành một chuỗi (dùng toán tử +)
        for (size_t i = 1; i < expr->parts.size(); ++i)
        {
            emit_instr(OpCode::ADD, {}, expr->getLine(), expr->getCol());
        }
        return {};
    }

    void BytecodeEmitter::visitImportStmt(AST::ImportStmt * /*stmt*/)
    {
        // Không sinh bytecode cho import (hoặc xử lý import module ở đây nếu cần)
    }

    std::any BytecodeEmitter::visitMemberExpr(AST::MemberExpr *expr)
    {
#ifdef _DEBUG
        std::cerr << "[DEBUG] visitMemberExpr called!" << std::endl;
        std::cerr << "[DEBUG] MemberExpr: object=";
#endif
        if (auto id = dynamic_cast<AST::IdentifierExpr *>(expr->object.get())) {
#ifdef _DEBUG
            std::cerr << id->name.lexeme;
#endif
        } else {
#ifdef _DEBUG
            std::cerr << "(not identifier)";
#endif
        }
#ifdef _DEBUG
        std::cerr << ", property=" << expr->property_token.lexeme << std::endl;
#endif
#ifdef _DEBUG
        std::cerr << "[DEBUG] is_package_constant: " << (expr->is_package_constant ? "true" : "false") << std::endl;
#endif
        
        // Kiểm tra xem có phải package constant không
        if (expr->is_package_constant) {
            // Allow os.name and os.arch as constants, but treat other os.* as functions
            if (expr->package_name == "os" && 
                expr->constant_name != "name" && 
                expr->constant_name != "arch") {
                return {};
            }
            std::string full_name = expr->package_name + "." + expr->constant_name;
#ifdef _DEBUG
            std::cerr << "[DEBUG] Emitting LOAD_PACKAGE_CONST: " << full_name << std::endl;
#endif
            emit_instr(OpCode::LOAD_PACKAGE_CONST, full_name, expr->getLine(), expr->getCol());
            return {};
        }
        
        // Fallback: Nếu object là IdentifierExpr và là package mặc định, sinh LOAD_PACKAGE_CONST
        auto id = dynamic_cast<AST::IdentifierExpr *>(expr->object.get());
        if (id) {
            // Luôn xử lý math.* và fs.* như package constant
            if (id->name.lexeme == "math" || id->name.lexeme == "fs") {
                // Sinh opcode LOAD_PACKAGE_CONST với operand "package.property"
                std::string full_name = id->name.lexeme + "." + expr->property_token.lexeme;
#ifdef _DEBUG
                std::cerr << "[DEBUG] Emitting LOAD_PACKAGE_CONST (fallback): " << full_name << std::endl;
#endif
                emit_instr(OpCode::LOAD_PACKAGE_CONST, full_name, expr->getLine(), expr->getCol());
                return {};
            }
            if (id->name.lexeme == "json") {
                // json không có constant, chỉ functions; tránh phát sinh LOAD_PACKAGE_CONST
                return {};
            }
        }
        // Nếu không phải package, xử lý như cũ (truy cập thuộc tính của map/object)
        if (expr->object)
            expr->object->accept(this);
        // Không sinh bytecode cho property ở đây (xử lý trong visitCallExpr nếu là method)
        return {};
    }

    std::any BytecodeEmitter::visitMethodCallExpr(AST::MethodCallExpr *expr)
    {
        // Math package methods: abs, ceil, floor, round, trunc
        auto id = dynamic_cast<AST::IdentifierExpr *>(expr->object.get());
        if (id && id->name.lexeme == "math")
        {
            // Check if it's a math function
            if ((expr->method_name == "abs" || expr->method_name == "ceil" || 
                 expr->method_name == "floor" || expr->method_name == "round" || 
                 expr->method_name == "trunc" || expr->method_name == "sin" ||
                 expr->method_name == "cos" || expr->method_name == "tan" ||
                 expr->method_name == "asin" || expr->method_name == "acos" ||
                 expr->method_name == "atan" || expr->method_name == "radians" ||
                 expr->method_name == "sinh" || expr->method_name == "cosh" || expr->method_name == "tanh" ||
                 expr->method_name == "asinh" || expr->method_name == "acosh" || expr->method_name == "atanh" ||
                 expr->method_name == "sqrt" || expr->method_name == "cbrt" ||
                 expr->method_name == "exp" || expr->method_name == "expm1" ||
                 expr->method_name == "log" || expr->method_name == "log1p" ||
                 expr->method_name == "log10" || expr->method_name == "log2") && expr->arguments.size() == 1)
            {
                // Emit the argument first
                expr->arguments[0]->accept(this);
                // Then emit the function call
                emit_instr(OpCode::CALL, expr->method_name, expr->getLine(), expr->getCol());
                return {};
            }
            else if (expr->method_name == "atan2" && expr->arguments.size() == 2)
            {
                // Emit the arguments in reverse order (y first, then x)
                expr->arguments[1]->accept(this); // y
                expr->arguments[0]->accept(this); // x
                // Then emit the function call
                emit_instr(OpCode::CALL, expr->method_name, expr->getLine(), expr->getCol());
                return {};
            }
            else if (expr->method_name == "pow" && expr->arguments.size() == 2)
            {
                // Emit the arguments in reverse order (exponent first, then base)
                expr->arguments[1]->accept(this); // exponent
                expr->arguments[0]->accept(this); // base
                // Then emit the function call
                emit_instr(OpCode::CALL, expr->method_name, expr->getLine(), expr->getCol());
                return {};
            }
        }
        
        // Fs package methods: open, read, close
        if (id && id->name.lexeme == "fs")
        {
            // Check if it's a fs function
            if ((expr->method_name == "open" || expr->method_name == "read" || expr->method_name == "close" || expr->method_name == "bRead" || expr->method_name == "bWrite" || expr->method_name == "bAppend"))
            {
                // Emit arguments first (if any)
                for (const auto &arg : expr->arguments)
                    if (arg)
                        arg->accept(this);
                
                // Then emit the package function call
                std::string full_name = "fs." + expr->method_name;
                emit_instr(OpCode::CALL_PACKAGE_FUNCTION, full_name, expr->getLine(), expr->getCol());
                return {};
            }
        }
        
        // Map methods: delete, clear, keys, values
        if (expr->method_name == "delete" && expr->arguments.size() == 1)
        {
            if (expr->object)
                expr->object->accept(this);
            expr->arguments[0]->accept(this);
            emit_instr(OpCode::MAP_DELETE, {}, expr->getLine(), expr->getCol());
            return {};
        }
        if (expr->method_name == "clear" && expr->arguments.empty())
        {
            if (expr->object)
                expr->object->accept(this);
            emit_instr(OpCode::MAP_CLEAR, {}, expr->getLine(), expr->getCol());
            return {};
        }
        if (expr->method_name == "keys" && expr->arguments.empty())
        {
            if (expr->object)
                expr->object->accept(this);
            emit_instr(OpCode::MAP_KEYS, {}, expr->getLine(), expr->getCol());
            return {};
        }
        if (expr->method_name == "values" && expr->arguments.empty())
        {
            if (expr->object)
                expr->object->accept(this);
            emit_instr(OpCode::MAP_VALUES, {}, expr->getLine(), expr->getCol());
            return {};
        }
        // You can add array methods here if needed
        // Default: just visit object and arguments (no-op)
        if (expr->object)
            expr->object->accept(this);
        for (const auto &arg : expr->arguments)
            if (arg)
                arg->accept(this);
        return {};
    }

    std::any BytecodeEmitter::visitFunctionExpr(AST::FunctionExpr *expr) {
        // Tạo function object không tên (anonymous)
        std::vector<FunctionParameter> function_params;
        for (const auto &param : expr->params) {
            std::optional<std::string> param_type = std::nullopt;
            if (param.type.has_value() && param.type.value()) {
                // TODO: convert TypeNode to string
                param_type = "any";
            }
            function_params.emplace_back(param.name.lexeme, param_type, param.is_static);
        }
        
        BytecodeChunk function_body;
        BytecodeEmitter body_emitter;
        body_emitter.var_table = var_table; // inherit outer var table for closure
        
        // Check if function body uses variables from outer scope
        bool needs_closure = false;
        std::unordered_set<std::string> captured_vars;
        
        // Simple heuristic: if function body references variables not in its own scope
        // and those variables exist in the outer scope, we need a closure
        // This is a simplified approach - a more sophisticated implementation would
        // do proper static analysis of variable usage
        
        if (expr->body) {
            expr->body->accept(&body_emitter);
            
            // Check if any variables from outer scope are used
            for (const auto& [var_name, var_index] : var_table) {
                // If variable exists in outer scope and might be used in function
                // (This is a simplified check - in practice you'd do proper analysis)
                if (var_index < next_var_index) {
                    captured_vars.insert(var_name);
                    needs_closure = true;
                }
            }
        }
        
        function_body = body_emitter.chunk;
        
        // Tên hàm rỗng cho anonymous
        auto fn = create_function("", function_params, function_body);
        emit_instr(OpCode::PUSH_FUNCTION, fn, expr->getLine(), expr->getCol());
        
        // If closure is needed, create it
        if (needs_closure) {
            // Emit CAPTURE_VAR for each captured variable
            for (const auto& var_name : captured_vars) {
                emit_instr(OpCode::CAPTURE_VAR, var_name, expr->getLine(), expr->getCol());
            }
            
            // Create closure from the function
            emit_instr(OpCode::CREATE_CLOSURE, {}, expr->getLine(), expr->getCol());
        }
        
        return {};
    }
    
    // Closure support methods
    std::unordered_set<std::string> BytecodeEmitter::get_used_variables_in_scope(const AST::StmtList& stmts) {
        std::unordered_set<std::string> used_vars;
        
        // Simple implementation: collect all variable names that are accessed
        // In a more sophisticated implementation, you'd do proper static analysis
        for (const auto& stmt : stmts) {
            if (stmt) {
                // This is a simplified approach - in practice you'd traverse the AST
                // to find all variable references
                // For now, we'll return an empty set and rely on the heuristic in visitFunctionExpr
            }
        }
        
        return used_vars;
    }
    
    bool BytecodeEmitter::needs_closure_for_function(const AST::FunctionExpr* expr) {
        if (!expr || !expr->body) {
            return false;
        }
        
        // Simple heuristic: if the function body contains any statements,
        // assume it might need closure (this is a very basic approach)
        // In a real implementation, you'd do proper static analysis to detect
        // variable usage from outer scopes
        
        return true; // For now, always assume closure might be needed
    }
}