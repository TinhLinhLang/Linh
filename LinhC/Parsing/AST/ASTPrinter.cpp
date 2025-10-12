// --- LinhC/Parsing/AST/ASTPrinter.cpp ---
#include "ASTPrinter.hpp"
#include <iostream> // Cho std::cerr nếu cần debug

namespace Linh
{
    namespace AST
    {

        void ASTPrinter::indent()
        {
            for (int i = 0; i < m_indent_level; ++i)
                m_builder << "  ";
        }

        // --- Phương thức trợ giúp để in TypeNode ---
        std::string ASTPrinter::print_type_node(TypeNode *type_node)
        {
            if (!type_node)
                return "Untyped";
            return type_node->accept(this); // ASTPrinter giờ là một TypeVisitor
        }

        std::string ASTPrinter::print_optional_type_node(const std::optional<TypeNodePtr> &opt_type_node)
        {
            if (opt_type_node.has_value() && opt_type_node.value() != nullptr)
            {
                return print_type_node(opt_type_node.value().get());
            }
            return "NoneType";
        }

        std::string ASTPrinter::print(const StmtList &statements)
        {
            m_builder.str("");
            m_builder.clear();
            m_indent_level = 0;
            m_builder << "(Program\n";
            m_indent_level++;
            visit_stmt_list_indented(statements);
            m_indent_level--;
            indent();
            m_builder << ")\n";
            return m_builder.str();
        }

        std::string ASTPrinter::print_expr(Expr *expr)
        {
            if (!expr)
                return "(NullExpr)"; // Hoặc một chuỗi khác nếu bạn muốn, ví dụ "uninit_expr"
            std::any result = expr->accept(this);
            try
            {
                return std::any_cast<std::string>(result);
            }
            catch (const std::bad_any_cast &)
            {
                // Điều này không nên xảy ra nếu tất cả các visitExpr trả về std::string
                // std::cerr << "Lỗi: bad_any_cast trong ASTPrinter::print_expr" << std::endl;
                return "(ErrorInPrintExpr_BadAnyCast)";
            }
        }

        std::string ASTPrinter::print_optional_expr(const std::optional<ExprPtr> &opt_expr)
        {
            if (opt_expr.has_value() && opt_expr.value() != nullptr)
            {
                return print_expr(opt_expr.value().get());
            }
            return "None"; // Hoặc "uninit_expr_opt"
        }

        std::string ASTPrinter::parenthesize_expr_visit(const std::string &name, const std::vector<Expr *> &exprs)
        {
            std::ostringstream temp_builder;
            temp_builder << "(" << name;
            for (Expr *expr_ptr : exprs)
            {
                temp_builder << " " << print_expr(expr_ptr); // print_expr đã xử lý nullptr
            }
            temp_builder << ")";
            return temp_builder.str();
        }

        // ExprVisitor implementations
        std::any ASTPrinter::visitBinaryExpr(BinaryExpr *expr)
        {
            return parenthesize_expr_visit(expr->op.lexeme, {expr->left.get(), expr->right.get()});
        }

        std::any ASTPrinter::visitUnaryExpr(UnaryExpr *expr)
        {
            return parenthesize_expr_visit(expr->op.lexeme, {expr->right.get()});
        }

        std::any ASTPrinter::visitPostfixExpr(PostfixExpr *expr)
        {
            std::ostringstream temp_builder;
            temp_builder << "(" << print_expr(expr->operand.get()) << " " << expr->op_token.lexeme << ")";
            return temp_builder.str();
        }

        std::any ASTPrinter::visitLiteralExpr(LiteralExpr *expr)
        {
            std::ostringstream temp_builder;
            std::visit([&](auto &&arg)
                       { 
                using T = std::decay_t<decltype(arg)>; 
                if constexpr (std::is_same_v<T, std::monostate>) temp_builder << "uninit_literal_value";
                else if constexpr (std::is_same_v<T, int64_t>) temp_builder << std::to_string(arg); 
                else if constexpr (std::is_same_v<T, double>) temp_builder << std::to_string(arg); 
                else if constexpr (std::is_same_v<T, std::string>) temp_builder << "\"" << arg << "\""; 
                else if constexpr (std::is_same_v<T, bool>) temp_builder << (arg ? "true" : "false"); }, expr->value);
            return temp_builder.str();
        }

        std::any ASTPrinter::visitGroupingExpr(GroupingExpr *expr)
        {
            return parenthesize_expr_visit("group", {expr->expression.get()});
        }

        std::any ASTPrinter::visitIdentifierExpr(IdentifierExpr *expr)
        {
            return std::string(expr->name.lexeme);
        }

        std::any ASTPrinter::visitAssignmentExpr(AssignmentExpr *expr)
        {
            // Hiện tại vẫn gán cho IdentifierExpr. Sẽ cần cập nhật nếu lvalue là Subscript/MemberAccess
            return parenthesize_expr_visit("assign " + expr->name.lexeme, {expr->value.get()});
        }

        std::any ASTPrinter::visitLogicalExpr(LogicalExpr *expr)
        {
            return parenthesize_expr_visit(expr->op.lexeme, {expr->left.get(), expr->right.get()});
        }

        std::any ASTPrinter::visitCallExpr(CallExpr *expr)
        {
            std::vector<Expr *> all_exprs;
            all_exprs.push_back(expr->callee.get());
            for (const auto &arg : expr->arguments)
            {
                all_exprs.push_back(arg.get());
            }
            return parenthesize_expr_visit("call", all_exprs);
        }

        std::any ASTPrinter::visitUninitLiteralExpr(UninitLiteralExpr *expr)
        {
            return std::string(expr->keyword.lexeme); // Sẽ in ra "uninit"
        }

        std::any ASTPrinter::visitNewExpr(NewExpr *expr)
        {
            return parenthesize_expr_visit(expr->keyword_new.lexeme, {expr->class_constructor_call.get()});
        }

        std::any ASTPrinter::visitThisExpr(ThisExpr *expr)
        {
            return std::string(expr->keyword_this.lexeme);
        }

        // StmtVisitor helper
        void ASTPrinter::build_stmt_structure(const std::string &name,
                                              const std::vector<Expr *> &expr_fields,
                                              const std::vector<Stmt *> &stmt_fields,
                                              const std::vector<BlockStmt *> &direct_block_fields,
                                              const std::vector<std::unique_ptr<BlockStmt> *> &unique_ptr_block_fields,
                                              const std::vector<std::pair<std::string, std::string>> &text_fields)
        {
            indent();
            m_builder << "(" << name;
            for (const auto &field : text_fields)
                m_builder << " " << field.first << ":" << field.second;
            for (Expr *expr_ptr : expr_fields)
                m_builder << " " << print_expr(expr_ptr);

            bool has_sub_structure = !stmt_fields.empty() || !direct_block_fields.empty() || !unique_ptr_block_fields.empty();
            if (has_sub_structure)
            {
                m_builder << "\n";
                m_indent_level++;
            }

            for (Stmt *stmt_ptr : stmt_fields)
            {
                if (stmt_ptr)
                    stmt_ptr->accept(this);
                else
                {
                    indent();
                    m_builder << "(nullptr_stmt_field)\n";
                }
            }
            for (BlockStmt *block_ptr : direct_block_fields)
            {
                if (block_ptr)
                    block_ptr->accept(this);
                else
                {
                    indent();
                    m_builder << "(nullptr_direct_block_field)\n";
                }
            }
            for (const auto &unique_block_ptr_ref : unique_ptr_block_fields)
            {
                if (unique_block_ptr_ref && unique_block_ptr_ref->get())
                {
                    (*unique_block_ptr_ref)->accept(this);
                }
                else
                {
                    indent();
                    m_builder << "(nullptr_unique_ptr_block_field)\n";
                }
            }

            if (has_sub_structure)
            {
                m_indent_level--;
                indent();
            }
            m_builder << ")\n";
        }

        void ASTPrinter::visit_stmt_list_indented(const StmtList &stmts)
        {
            for (const auto &stmt_ptr : stmts)
            {
                if (stmt_ptr)
                    stmt_ptr->accept(this);
                else
                {
                    indent();
                    m_builder << "(NullStmtInList)\n";
                }
            }
        }

        // StmtVisitor implementations
        void ASTPrinter::visitExpressionStmt(ExpressionStmt *stmt)
        {
            build_stmt_structure("ExpressionStmt", {stmt->expression.get()});
        }
        void ASTPrinter::visitPrintStmt(PrintStmt *stmt)
        {
            std::vector<Expr*> exprs;
            for (const auto& expr : stmt->expressions) {
                exprs.push_back(expr.get());
            }
            build_stmt_structure("PrintStmt", exprs, {}, {}, {}, {{"kw", stmt->keyword.lexeme}});
        }

        void ASTPrinter::visitVarDeclStmt(VarDeclStmt *stmt)
        {
            std::string type_str = "Untyped";
            if (stmt->declared_type.has_value() && stmt->declared_type.value() != nullptr)
            {
                type_str = print_type_node(stmt->declared_type.value().get());
            }
            // initializer có thể là nullptr, print_expr sẽ xử lý
            build_stmt_structure("VarDeclStmt",
                                 {stmt->initializer.get()},
                                 {}, {}, {},
                                 {{"kw", stmt->keyword.lexeme},
                                  {"name", stmt->name.lexeme},
                                  {"type", type_str}});
        }

        void ASTPrinter::visitBlockStmt(BlockStmt *stmt)
        {
            indent();
            m_builder << "(Block (brace_at_line:" << stmt->opening_brace.line << ")\n";
            m_indent_level++;
            visit_stmt_list_indented(stmt->statements);
            m_indent_level--;
            indent();
            m_builder << ")\n";
        }
        void ASTPrinter::visitIfStmt(IfStmt *stmt)
        {
            indent();
            m_builder << "(If " << print_expr(stmt->condition.get()) << "\n";
            m_indent_level++;
            indent();
            m_builder << "(Then\n";
            m_indent_level++;
            if (stmt->then_branch)
                stmt->then_branch->accept(this);
            else
            {
                indent();
                m_builder << "(EmptyBranch)\n";
            }
            m_indent_level--;
            indent();
            m_builder << ")\n";
            if (stmt->else_branch)
            {
                indent();
                m_builder << "(Else\n";
                m_indent_level++;
                stmt->else_branch->accept(this);
                m_indent_level--;
                indent();
                m_builder << ")\n";
            }
            m_indent_level--;
            indent();
            m_builder << ")\n";
        }
        void ASTPrinter::visitWhileStmt(WhileStmt *stmt)
        {
            indent();
            m_builder << "(While " << print_expr(stmt->condition.get()) << "\n";
            m_indent_level++;
            indent();
            m_builder << "(Body\n";
            m_indent_level++;
            if (stmt->body)
                stmt->body->accept(this);
            else
            {
                indent();
                m_builder << "(EmptyBody)\n";
            }
            m_indent_level--;
            indent();
            m_builder << ")\n";
            m_indent_level--;
            indent();
            m_builder << ")\n";
        }

        void ASTPrinter::visitFunctionDeclStmt(FunctionDeclStmt *stmt)
        {
            std::string params_str;
            for (size_t i = 0; i < stmt->params.size(); ++i)
            {
                const auto &param_node = stmt->params[i];
                params_str += param_node.name.lexeme;
                if (param_node.type.has_value() && param_node.type.value() != nullptr)
                {
                    params_str += ":" + print_type_node(param_node.type.value().get());
                }
                if (i < stmt->params.size() - 1)
                    params_str += ", ";
            }

            std::string return_type_str = "ImplicitVoid";
            if (stmt->return_type.has_value() && stmt->return_type.value() != nullptr)
            {
                return_type_str = print_type_node(stmt->return_type.value().get());
            }

            build_stmt_structure("FunctionDeclStmt", {}, {}, {}, {&stmt->body},
                                 {{"name", stmt->name.lexeme},
                                  {"params", params_str},
                                  {"return_type", return_type_str}});
        }

        void ASTPrinter::visitReturnStmt(ReturnStmt *stmt)
        {
            build_stmt_structure("ReturnStmt", {stmt->value.get()}, {}, {}, {}, {{"kw", stmt->keyword_return.lexeme}});
        }
        void ASTPrinter::visitBreakStmt(BreakStmt *stmt)
        {
            build_stmt_structure("BreakStmt", {}, {}, {}, {}, {{"kw", stmt->keyword.lexeme}});
        }
        void ASTPrinter::visitContinueStmt(ContinueStmt *stmt)
        {
            build_stmt_structure("ContinueStmt", {}, {}, {}, {}, {{"kw", stmt->keyword.lexeme}});
        }
        void ASTPrinter::visitSwitchStmt(SwitchStmt *stmt)
        {
            indent();
            m_builder << "(SwitchOn kw:" << stmt->keyword_switch.lexeme << " " << print_expr(stmt->expression_to_switch_on.get()) << "\n";
            m_indent_level++;
            for (const auto &case_clause : stmt->cases)
            {
                indent();
                if (case_clause.is_default)
                {
                    m_builder << "(DefaultCase kw:" << case_clause.keyword_token.lexeme << "\n";
                }
                else
                {
                    m_builder << "(Case kw:" << case_clause.keyword_token.lexeme << " val:" << print_optional_expr(case_clause.case_value) << "\n";
                }
                m_indent_level++;
                visit_stmt_list_indented(case_clause.statements);
                m_indent_level--;
                indent();
                m_builder << ")\n";
            }
            m_indent_level--;
            indent();
            m_builder << ")\n";
        }
        void ASTPrinter::visitDeleteStmt(DeleteStmt *stmt)
        {
            build_stmt_structure("DeleteStmt", {stmt->expression_to_delete.get()}, {}, {}, {}, {{"kw", stmt->keyword_delete.lexeme}});
        }
        void ASTPrinter::visitThrowStmt(ThrowStmt *stmt)
        {
            build_stmt_structure("ThrowStmt", {stmt->expression.get()}, {}, {}, {}, {{"kw", stmt->keyword.lexeme}});
        }
        void ASTPrinter::visitTryStmt(TryStmt *stmt)
        {
            indent();
            m_builder << "(Try kw:" << stmt->keyword_try.lexeme << "\n";
            m_indent_level++;

            indent();
            m_builder << "(TryBlock\n";
            m_indent_level++;
            if (stmt->try_block)
            {
                stmt->try_block->accept(this);
            }
            else
            {
                indent();
                m_builder << "(EmptyTryBlock)\n";
            }
            m_indent_level--;
            indent();
            m_builder << ")\n";

            for (const auto &catch_clause : stmt->catch_clauses)
            {
                indent();
                m_builder << "(Catch kw:" << catch_clause.keyword_catch.lexeme;
                if (catch_clause.exception_variable.has_value())
                {
                    m_builder << " var:" << catch_clause.exception_variable.value().lexeme;
                }
                m_builder << "\n";
                m_indent_level++;
                if (catch_clause.body)
                {
                    catch_clause.body->accept(this);
                }
                else
                {
                    indent();
                    m_builder << "(EmptyCatchBody)\n";
                }
                m_indent_level--;
                indent();
                m_builder << ")\n";
            }

            if (stmt->finally_block.has_value() && stmt->finally_block.value())
            {
                std::string finally_kw_str = "finally_kw_missing";
                if (stmt->keyword_finally.has_value())
                {
                    finally_kw_str = stmt->keyword_finally.value().lexeme;
                }
                indent();
                m_builder << "(Finally kw:" << finally_kw_str << "\n";
                m_indent_level++;
                stmt->finally_block.value()->accept(this);
                m_indent_level--;
                indent();
                m_builder << ")\n";
            }
            m_indent_level--;
            indent();
            m_builder << ")\n";
        }

        void ASTPrinter::visitImportStmt(ImportStmt *stmt)
        {
            indent();
            if (!stmt->names.empty())
            {
                m_builder << "(ImportStmt kw:" << stmt->import_kw.lexeme << " names:[";
                for (size_t i = 0; i < stmt->names.size(); ++i)
                {
                    m_builder << stmt->names[i].lexeme;
                    if (i + 1 < stmt->names.size())
                        m_builder << ", ";
                }
                m_builder << "]";
                if (stmt->from_kw.type == TokenType::FROM_KW)
                    m_builder << " from:" << stmt->from_kw.lexeme << " module:" << stmt->module_name.lexeme;
                m_builder << ")\n";
            }
            else
            {
                m_builder << "(ImportModule kw:" << stmt->import_kw.lexeme << " module:" << stmt->module_name.lexeme << ")\n";
            }
        }

        // --- TypeVisitor implementations ---
        std::string ASTPrinter::visitBaseTypeNode(BaseTypeNode *type_node)
        {
            // Hiển thị str<index> nếu có
            if (type_node->type_keyword_token.type == TokenType::STR_KW && type_node->template_arg.has_value())
                return type_node->type_keyword_token.lexeme + "<" + std::to_string(type_node->template_arg.value()) + ">";
            // Hiển thị int/uint/float<bit> nếu có
            if ((type_node->type_keyword_token.type == TokenType::INT_KW ||
                 type_node->type_keyword_token.type == TokenType::UINT_KW ||
                 type_node->type_keyword_token.type == TokenType::FLOAT_KW) &&
                type_node->template_arg.has_value())
                return type_node->type_keyword_token.lexeme + "<" + std::to_string(type_node->template_arg.value()) + ">";
            // Hiển thị byte đúng tên
            if (type_node->type_keyword_token.type == TokenType::BYTE_KW)
                return "byte";
            return type_node->type_keyword_token.lexeme;
        }

        std::string ASTPrinter::visitSizedIntegerTypeNode(SizedIntegerTypeNode *type_node)
        {
            // int<bit> hoặc uint<bit>
            return type_node->base_type_keyword_token.lexeme + "<" + type_node->size_token.lexeme + ">";
        }

        std::string ASTPrinter::visitSizedFloatTypeNode(SizedFloatTypeNode *type_node)
        {
            // float<bit>
            return type_node->base_type_keyword_token.lexeme + "<" + type_node->size_token.lexeme + ">";
        }

        std::string ASTPrinter::visitMapTypeNode(MapTypeNode *type_node)
        {
            std::string key_str = type_node->key_type ? print_type_node(type_node->key_type.get()) : "AnyKey";
            std::string val_str = type_node->value_type ? print_type_node(type_node->value_type.get()) : "AnyValue";
            return type_node->map_keyword_token.lexeme + "<" + key_str + ", " + val_str + ">";
        }

        std::string ASTPrinter::visitArrayTypeNode(ArrayTypeNode *type_node)
        {
            if (type_node->array_keyword_token.has_value())
            {
                // Nếu là 'array', element_type trong ASTNode là nullptr, không cần in
                return type_node->array_keyword_token.value().lexeme;
            }
            // Ngược lại, là ElementType[]
            std::string el_str = type_node->element_type ? print_type_node(type_node->element_type.get()) : "UnknownElementType";
            return el_str + "[]";
        }

        std::string ASTPrinter::visitUnionTypeNode(UnionTypeNode *type_node)
        {
            std::string union_str = "<";
            for (size_t i = 0; i < type_node->types.size(); ++i)
            {
                if (type_node->types[i])
                {
                    union_str += print_type_node(type_node->types[i].get());
                }
                else
                {
                    union_str += "NullTypeInUnion"; // Trường hợp không nên xảy ra nếu Parser đúng
                }
                if (i < type_node->types.size() - 1)
                {
                    union_str += ", ";
                }
            }
            union_str += ">";
            return union_str;
        }

        std::any ASTPrinter::visitArrayLiteralExpr(ArrayLiteralExpr *expr)
        {
            std::ostringstream temp_builder;
            temp_builder << "(array-literal";
            for (const auto &element : expr->elements)
            {
                temp_builder << " " << print_expr(element.get());
            }
            temp_builder << ")";
            return temp_builder.str();
        }

        std::any ASTPrinter::visitMapLiteralExpr(MapLiteralExpr *expr)
        {
            std::ostringstream temp_builder;
            temp_builder << "(map-literal";
            if (!expr->entries.empty())
            {
                m_indent_level++;
                for (const auto &entry : expr->entries)
                {
                    temp_builder << "\n";
                    indent();
                    temp_builder << "(entry " << print_expr(entry.key.get())
                                 << " " << entry.colon_token.lexeme << " "
                                 << print_expr(entry.value.get()) << ")";
                }
                m_indent_level--;
                temp_builder << "\n";
                indent();
            }
            temp_builder << ")";
            return temp_builder.str();
        }

        std::any ASTPrinter::visitSubscriptExpr(SubscriptExpr *expr)
        {
            std::ostringstream temp_builder;
            temp_builder << "(subscript " << print_expr(expr->object.get())
                         << " [ " << print_expr(expr->index.get()) << " ])";
            return temp_builder.str();
        }

        std::any ASTPrinter::visitInterpolatedStringExpr(InterpolatedStringExpr *expr)
        {
            std::ostringstream temp_builder;
            temp_builder << "(interpolated-string";
            for (const auto &part : expr->parts)
            {
                if (std::holds_alternative<std::string>(part))
                {
                    temp_builder << " \"" << std::get<std::string>(part) << "\"";
                }
                else
                {
                    temp_builder << " &{ " << print_expr(std::get<ExprPtr>(part).get()) << " }";
                }
            }
            temp_builder << ")";
            return temp_builder.str();
        }

        std::any ASTPrinter::visitMethodCallExpr(MethodCallExpr *expr)
        {
            // In dạng: (method-call object method_name ...args)
            std::ostringstream oss;
            oss << "(method-call " << print_expr(expr->object.get()) << " ." << expr->method_name;
            for (const auto &arg : expr->arguments)
                oss << " " << print_expr(arg.get());
            oss << ")";
            return oss.str();
        }

    } // namespace AST
} // namespace Linh