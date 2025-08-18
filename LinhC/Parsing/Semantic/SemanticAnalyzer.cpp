#include "SemanticAnalyzer.hpp"
#include "../../../config.hpp"
#include <array>                              // thêm dòng này
#include <fstream>                            // thêm dòng này
#include <unordered_set>                      // <--- add this line
#include "../Parser/Parser.hpp"               // thêm dòng này
#include "../../Bytecode/BytecodeEmitter.hpp" // Thêm dòng này để dùng BytecodeEmitter
#include <chrono>
#include <sstream>

// === Helper: kiểm tra subtype giữa hai type string (hỗ trợ array/map) ===
static bool is_subtype_type_string(const std::string& sub, const std::string& super) {
    if (sub == super) return true;
    if (super == "any") return true;
    // array<any> là supertype của mọi array
    if (super == "array<any>" && sub.find("array<") == 0) return true;
    // map<any,any> là supertype của mọi map
    if (super == "map<any,any>" && sub.find("map<") == 0) return true;
    // array<T>
    if (sub.find("array<") == 0 && super.find("array<") == 0) {
        auto get_inner = [](const std::string& s) -> std::string {
            size_t l = s.find('<');
            size_t r = s.rfind('>');
            if (l == std::string::npos || r == std::string::npos || r <= l+1) return "any";
            return s.substr(l+1, r-l-1);
        };
        return is_subtype_type_string(get_inner(sub), get_inner(super));
    }
    // map<K,V>
    if (sub.find("map<") == 0 && super.find("map<") == 0) {
        auto get_inner = [](const std::string& s) -> std::pair<std::string,std::string> {
            size_t l = s.find('<');
            size_t r = s.rfind('>');
            if (l == std::string::npos || r == std::string::npos || r <= l+1) return {"any","any"};
            std::string inside = s.substr(l+1, r-l-1);
            size_t comma = inside.find(',');
            if (comma == std::string::npos) return {inside, "any"};
            std::string k = inside.substr(0, comma);
            std::string v = inside.substr(comma+1);
            // Trim
            k.erase(0, k.find_first_not_of(" \t\n")); k.erase(k.find_last_not_of(" \t\n")+1);
            v.erase(0, v.find_first_not_of(" \t\n")); v.erase(v.find_last_not_of(" \t\n")+1);
            return {k, v};
        };
        auto [k1, v1] = get_inner(sub);
        auto [k2, v2] = get_inner(super);
        return is_subtype_type_string(k1, k2) && is_subtype_type_string(v1, v2);
    }
    // Có thể mở rộng cho union/option/tuple nếu cần
    return false;
}

// Đặt helper vào đúng namespace và dùng Linh::ErrorStage::Semantic
void push_semantic_error(std::vector<Linh::Error>& errors, int line, int col, const std::string& message) {
    std::ostringstream oss;
    oss << "ERROR [Line: " << line << ", Col: " << col << "] SemanticError: " << message;
    errors.push_back({Linh::ErrorStage::Semantic, "SemanticError", oss.str(), line, col});
}

namespace Linh
{
    namespace Semantic
    {
        extern Linh::BytecodeEmitter *g_main_emitter;

        // Thêm các mảng constexpr cho bit options
        constexpr std::array<int, 4> int_and_uint_bit_options = {8, 16, 32, 64};
        constexpr std::array<int, 2> float_bit_options = {32, 64};

        void SemanticAnalyzer::analyze(const AST::StmtList &stmts, bool reset_state)
        {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            if (reset_state)
            {
                var_scopes.clear();
                global_functions.clear();
                type_cache.clear();
                error_cache.clear();
                cache_hits = 0;
                cache_misses = 0;
                should_early_exit = false;
                
                begin_scope();
                
                if (parallel_analysis_enabled) {
                    // Parallel analysis for large codebases
                    std::vector<std::future<std::vector<Linh::Error>>> futures;
                    for (const auto &stmt : stmts) {
                        if (stmt) {
                            auto s = stmt.get();
                            futures.push_back(std::async(std::launch::async, [this, s]() {
                                std::vector<Linh::Error> local_errors;
                                SemanticAnalyzer local_analyzer;
                                local_analyzer.enable_caching(caching_enabled);
                                local_analyzer.enable_early_exit(early_exit_enabled);
                                local_analyzer.enable_parallel_analysis(false);
                                try {
                                    s->accept(&local_analyzer);
                                    local_errors = local_analyzer.errors;
                                } catch (...) {}
                                return local_errors;
                            }));
                        }
                    }
                    
                    // Collect results
                    for (auto& future : futures) {
                        auto local_errors = future.get();
                        errors.insert(errors.end(), local_errors.begin(), local_errors.end());
                    }
                } else {
                    // Sequential analysis
                    for (const auto &stmt : stmts)
                    {
                        if (stmt && !should_early_exit)
                            stmt->accept(this);
                    }
                }
                end_scope();
            }
            else
            {
                // REPL mode: giữ lại scope toàn cục, không pop/push scope
                if (var_scopes.empty())
                {
                    begin_scope(); // Đảm bảo luôn có scope ngoài cùng
                }
                for (const auto &stmt : stmts)
                {
                    if (stmt && !should_early_exit)
                        stmt->accept(this);
                }
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            analysis_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        }

        // Helper methods for optimization
        std::string SemanticAnalyzer::get_expression_hash(AST::Expr* expr) {
            if (!expr) return "null";
            
            std::ostringstream oss;
            oss << expr;
            
            return oss.str();
        }
        
        std::string SemanticAnalyzer::get_error_key(const AST::Stmt* stmt, const std::string& error_type) {
            if (!stmt) return "null:" + error_type;
            std::ostringstream oss;
            oss << stmt << ":" << error_type;
            return oss.str();
        }
        std::string SemanticAnalyzer::get_error_key(const AST::Expr* expr, const std::string& error_type) {
            if (!expr) return "null:" + error_type;
            std::ostringstream oss;
            oss << expr << ":" << error_type;
            return oss.str();
        }
        bool SemanticAnalyzer::check_cached_error(const AST::Stmt* stmt, const std::string& error_type) {
            if (!caching_enabled) return false;
            std::string key = get_error_key(stmt, error_type);
            auto it = error_cache.find(key);
            if (it != error_cache.end()) { cache_hits++; return it->second; }
            cache_misses++;
            return false;
        }
        bool SemanticAnalyzer::check_cached_error(const AST::Expr* expr, const std::string& error_type) {
            if (!caching_enabled) return false;
            std::string key = get_error_key(expr, error_type);
            auto it = error_cache.find(key);
            if (it != error_cache.end()) { cache_hits++; return it->second; }
            cache_misses++;
            return false;
        }
        void SemanticAnalyzer::cache_error(const AST::Stmt* stmt, const std::string& error_type) {
            if (!caching_enabled) return;
            std::string key = get_error_key(stmt, error_type);
            error_cache[key] = true;
        }
        void SemanticAnalyzer::cache_error(const AST::Expr* expr, const std::string& error_type) {
            if (!caching_enabled) return;
            std::string key = get_error_key(expr, error_type);
            error_cache[key] = true;
        }
        
        void SemanticAnalyzer::analyze_statement_parallel(AST::Stmt* stmt) {
            if (!stmt) return;
            
            try {
                stmt->accept(this);
            } catch (...) {
                // Handle exceptions in parallel processing
            }
        }
        
        std::vector<Linh::Error> SemanticAnalyzer::merge_errors(const std::vector<std::vector<Linh::Error>>& error_lists) {
            std::vector<Linh::Error> merged;
            for (const auto& error_list : error_lists) {
                merged.insert(merged.end(), error_list.begin(), error_list.end());
            }
            return merged;
        }

        void SemanticAnalyzer::begin_scope()
        {
            var_scopes.push_back({});
            // Đăng ký built-in function printil (1 tham số)
            if (global_functions.count("printil") == 0)
            {
                declare_function("printil", 1);
            }
        }
        void SemanticAnalyzer::end_scope()
        {
            if (!var_scopes.empty())
                var_scopes.pop_back();
        }
        void SemanticAnalyzer::declare_var(const std::string &name, const std::string &kind /*= ""*/, const std::string &type /*= ""*/)
        {
            if (!var_scopes.empty())
                var_scopes.back()[name] = true;
            if (!kind.empty())
                var_kinds[name] = kind;
            if (!type.empty())
                var_types[name] = type;
        }
        bool SemanticAnalyzer::is_var_declared(const std::string &name)
        {
            // Nếu là package đã import, coi như đã khai báo
            if (imported_packages.count(name) > 0)
                return true;
            for (auto it = var_scopes.rbegin(); it != var_scopes.rend(); ++it)
            {
                if (it->count(name))
                    return true;
            }
            return false;
        }
        void SemanticAnalyzer::declare_function(const std::string &name, size_t param_count /*= 0*/)
        {
            global_functions[name] = true;
            function_param_counts[name] = param_count;
        }

        // Danh sách các hàm built-in hợp lệ
        static const std::unordered_set<std::string> builtin_functions = {
            "print", "input", "str", "int", "float", "bool", "len", "id", "type", "uint", "pow", "printil", "bytes", "bin"};

        bool SemanticAnalyzer::is_function_declared(const std::string &name)
        {
            // Nếu là hàm built-in thì luôn hợp lệ
            if (builtin_functions.count(name))
                return true;
            return global_functions.count(name) > 0;
        }

        void SemanticAnalyzer::visitExpressionStmt(AST::ExpressionStmt *stmt)
        {
            if (should_early_exit) return;
            
            if (stmt->expression)
                stmt->expression->accept(this);
        }
        void SemanticAnalyzer::visitPrintStmt(AST::PrintStmt *stmt)
        {
            for (const auto& expr : stmt->expressions) {
                if (expr) {
                    expr->accept(this);
                }
            }
        }
        void SemanticAnalyzer::visitVarDeclStmt(AST::VarDeclStmt *stmt)
        {
            // Early exit check
            if (should_early_exit) return;
            
            // Check cached error
            if (check_cached_error(static_cast<const AST::Stmt*>(stmt), "var_decl_error")) {
                return;
            }
            
            // Check for sol type/value rules
            bool type_is_sol = is_sol_type(stmt->declared_type);
            bool value_is_sol = is_sol_expr(stmt->initializer);

            std::string kw = stmt->keyword.lexeme;
            int line = stmt->keyword.line;
            int col = stmt->keyword.column_start;

            if (kw == "vas" || kw == "const")
            {
                // Bỏ báo lỗi cho phép vas và const được khởi tạo với sol
                // if (value_is_sol)
                // {
                //     errors.emplace_back("'" + kw + "' cannot be initialized with 'sol' value.", line, col);
                // }
            }
            else if (kw == "var")
            {
                // Only error if explicit type is given (not sol) and value is sol
                if (stmt->declared_type.has_value() && !type_is_sol && value_is_sol)
                {
                    push_semantic_error(errors, line, col, "'var' with a specific type cannot be initialized with 'sol' value.");
                }
            }
            // Kiểm tra trùng tên biến trong cùng scope
            if (!var_scopes.empty() && var_scopes.back().count(stmt->name.lexeme))
            {
                push_semantic_error(errors, stmt->name.line, stmt->name.column_start, "Variable '" + stmt->name.lexeme + "' redeclared in the same scope.");
            }
            // Kiểm tra trùng tên biến với tên hàm toàn cục
            if (is_function_declared(stmt->name.lexeme))
            {
                push_semantic_error(errors, stmt->name.line, stmt->name.column_start, "Variable '" + stmt->name.lexeme + "' redeclared as a function name.");
            }

            // Kiểm tra tham số hàm không trùng tên với biến toàn cục (scope ngoài cùng)
            if (!var_scopes.empty() && var_scopes.front().count(stmt->name.lexeme))
            {
                // Đã kiểm tra ở trên
            }

            // Lưu loại biến và kiểu biến (giản lược: chỉ lấy tên kiểu nếu có)
            std::string kind = kw;
            std::string type;
            int bit_width = 0;
            std::string base_type_name;
            bool has_template = false;
            int str_limit = -1;
            if (stmt->declared_type.has_value() && stmt->declared_type.value())
            {
                auto *base = dynamic_cast<AST::BaseTypeNode *>(stmt->declared_type.value().get());
                auto *sized_int = dynamic_cast<AST::SizedIntegerTypeNode *>(stmt->declared_type.value().get());
                auto *sized_float = dynamic_cast<AST::SizedFloatTypeNode *>(stmt->declared_type.value().get());
                auto *map_type = dynamic_cast<AST::MapTypeNode *>(stmt->declared_type.value().get());
                auto *array_type = dynamic_cast<AST::ArrayTypeNode *>(stmt->declared_type.value().get());

                if (base)
                {
                    type = base->type_keyword_token.lexeme;
                    base_type_name = type;
                    if (base->template_arg.has_value())
                    {
                        bit_width = base->template_arg.value();
                        has_template = true;
                    }
                    if (base->type_keyword_token.lexeme == "str" && base->template_arg.has_value())
                    {
                        str_limit = base->template_arg.value();
                    }
                }
                else if (sized_int)
                {
                    type = sized_int->base_type_keyword_token.lexeme;
                    base_type_name = type;
                    if (sized_int->template_arg.has_value())
                    {
                        bit_width = sized_int->template_arg.value();
                        has_template = true;
                    }
                }
                else if (sized_float)
                {
                    type = sized_float->base_type_keyword_token.lexeme;
                    base_type_name = type;
                    if (sized_float->template_arg.has_value())
                    {
                        bit_width = sized_float->template_arg.value();
                        has_template = true;
                    }
                }
                else if (map_type)
                {
                    type = "map";
                    base_type_name = type;
                    has_template = true;
                }
                else if (array_type)
                {
                    type = "array";
                    base_type_name = type;
                    has_template = true;
                }
                else
                {
                    type = "";
                }
            }
            else if (stmt->initializer)
            {
                if (auto lit = dynamic_cast<AST::LiteralExpr *>(stmt->initializer.get()))
                {
                    type = SemanticAnalyzer::get_linh_literal_type(lit);
                }
                // Nếu initializer là IdentifierExpr và nó là function đã khai báo
                else if (auto id = dynamic_cast<AST::IdentifierExpr *>(stmt->initializer.get()))
                {
                    if (is_function_declared(id->name.lexeme)) {
                        type = "function";
                        if (function_param_counts.count(id->name.lexeme))
                            var_func_param_counts[stmt->name.lexeme] = function_param_counts[id->name.lexeme];
                    }
                }
                // Nếu initializer là anonymous function (FunctionExpr)
                else if (auto fnexpr = dynamic_cast<AST::FunctionExpr *>(stmt->initializer.get()))
                {
                    type = "function";
                    var_func_param_counts[stmt->name.lexeme] = fnexpr->params.size();
                }
                // Nếu initializer là function call (CallExpr)
                else if (auto callexpr = dynamic_cast<AST::CallExpr *>(stmt->initializer.get()))
                {
                    // Check if the call is to a function that returns a function
                    if (auto id = dynamic_cast<AST::IdentifierExpr *>(callexpr->callee.get()))
                    {
                        if (is_function_declared(id->name.lexeme)) {
                            // Assume function calls return functions for now
                            // In a more sophisticated implementation, you'd track return types
                            type = "function";
                            // We don't know the exact parameter count, so we'll leave it undefined
                            // The runtime will handle the actual call
                        }
                    }
                }
            }
            else
            {
                type = "";
            }

            // Chỉ cho phép <...> với int, uint, float, map, array, str
            if (has_template)
            {
                if (type != "int" && type != "uint" && type != "float" && type != "map" && type != "array" && type != "str")
                {
                    push_semantic_error(errors, stmt->name.line, stmt->name.column_start, "Type '" + type + "' does not support template specification (e.g. '<...>').");
                }
            }

            // Kiểm tra số bit hợp lệ cho kiểu số
            if (!type.empty() && (type == "int" || type == "uint"))
            {
                if (bit_width != 0)
                {
                    bool valid = false;
                    for (int opt : int_and_uint_bit_options)
                    {
                        if (bit_width == opt)
                        {
                            valid = true;
                            break;
                        }
                    }
                    if (!valid)
                    {
                        push_semantic_error(errors, stmt->name.line, stmt->name.column_start, "Invalid bit width for type '" + type + "<" + std::to_string(bit_width) + ">' (must be one of: 8, 16, 32, 64).");
                    }
                }
            }
            else if (!type.empty() && type == "float")
            {
                if (bit_width != 0)
                {
                    bool valid = false;
                    for (int opt : float_bit_options)
                    {
                        if (bit_width == opt)
                        {
                            valid = true;
                            break;
                        }
                    }
                    if (!valid)
                    {
                        push_semantic_error(errors, stmt->name.line, stmt->name.column_start, "Invalid bit width for type 'float<" + std::to_string(bit_width) + ">' (must be one of: 32, 64).");
                    }
                }
            }
            // Kiểm tra str<index> chỉ cho phép số nguyên dương
            if (type == "str" && bit_width != 0)
            {
                if (bit_width <= 0)
                {
                    push_semantic_error(errors, stmt->name.line, stmt->name.column_start, "Type 'str<" + std::to_string(bit_width) + ">' index must be positive.");
                }
            }

            // Nếu có giới hạn str<index> thì lưu vào var_str_limit
            if (type == "str" && str_limit > 0)
            {
                var_str_limit[stmt->name.lexeme] = str_limit;
            }
            // Nếu có giới hạn str<index> và có initializer là LiteralExpr thì cắt chuỗi
            if (type == "str" && str_limit > 0 && stmt->initializer)
            {
                if (auto lit = dynamic_cast<AST::LiteralExpr *>(stmt->initializer.get()))
                {
                    if (std::holds_alternative<std::string>(lit->value))
                    {
                        std::string val = std::get<std::string>(lit->value);
                        if (static_cast<int>(val.size()) > str_limit)
                        {
                            // Cắt chuỗi
                            std::string cut_val = val.substr(0, str_limit);
                            lit->value = cut_val;
                        }
                    }
                }
            }
            // Kiểm tra kiểu không phải số/map/array/str mà lại có template/bit_width
            if (!type.empty() && type != "int" && type != "uint" && type != "float" && type != "map" && type != "array" && type != "str" && has_template)
            {
                push_semantic_error(errors, stmt->name.line, stmt->name.column_start, "Type '" + type + "' does not support template specification (e.g. '<...>').");
            }

            declare_var(stmt->name.lexeme, kind, type);

            // Recursively check initializer
            if (stmt->initializer)
                stmt->initializer->accept(this);
        }
        void SemanticAnalyzer::visitBlockStmt(AST::BlockStmt *stmt)
        {
            begin_scope();
            for (const auto &s : stmt->statements)
            {
                if (s)
                    s->accept(this);
            }
            end_scope();
        }
        void SemanticAnalyzer::visitIfStmt(AST::IfStmt *stmt)
        {
            if (stmt->condition)
                stmt->condition->accept(this);
            if (stmt->then_branch)
                stmt->then_branch->accept(this);
            if (stmt->else_branch)
                stmt->else_branch->accept(this);
        }
        void SemanticAnalyzer::visitWhileStmt(AST::WhileStmt *stmt)
        {
            if (stmt->condition)
                stmt->condition->accept(this);
            // Đánh dấu scope là trong vòng lặp
            begin_scope();
            var_scopes.back()["__loop__"] = true;
            if (stmt->body)
                stmt->body->accept(this);
            end_scope();
        }
        void SemanticAnalyzer::visitDoWhileStmt(AST::DoWhileStmt *stmt)
        {
            if (stmt->body)
                stmt->body->accept(this);
            if (stmt->condition)
                stmt->condition->accept(this);
        }
        void SemanticAnalyzer::visitFunctionDeclStmt(AST::FunctionDeclStmt *stmt)
        {
            // Kiểm tra trùng tên hàm với biến toàn cục (scope ngoài cùng)
            if (!var_scopes.empty() && var_scopes.front().count(stmt->name.lexeme))
            {
                push_semantic_error(errors, stmt->name.line, stmt->name.column_start, "Function '" + stmt->name.lexeme + "' redeclared as a variable name.");
            }
            // Đăng ký tên hàm vào global_functions và lưu số lượng tham số
            declare_function(stmt->name.lexeme, stmt->params.size());

            // Kiểm tra trùng tên tham số trong danh sách tham số
            std::unordered_map<std::string, bool> param_names;
            for (const auto &param : stmt->params)
            {
                if (param_names.count(param.name.lexeme))
                {
                    push_semantic_error(errors, param.name.line, param.name.column_start, "Parameter '" + param.name.lexeme + "' redeclared in parameter list.");
                }
                param_names[param.name.lexeme] = true;
            }

            begin_scope();
            // Đăng ký tham số vào scope mới
            for (const auto &param : stmt->params)
            {
                declare_var(param.name.lexeme);
            }
            // Kiểm tra return trong hàm nếu có yêu cầu trả về giá trị
            bool has_return = false;
            if (stmt->body)
            {
                // Duyệt các statement trong body để tìm return
                for (const auto &s : stmt->body->statements)
                {
                    if (dynamic_cast<AST::ReturnStmt *>(s.get()))
                        has_return = true;
                }
                stmt->body->accept(this);
            }
            // Giả sử stmt->return_type là optional<TypeNodePtr> và nullptr nếu không có
            if (stmt->return_type.has_value() && stmt->return_type.value() && !has_return)
            {
                // --- Sửa: Không bắt buộc return nếu kiểu trả về là void hoặc uninit ---
                auto *base = dynamic_cast<AST::BaseTypeNode *>(stmt->return_type.value().get());
                if (!base || (base->type_keyword_token.type != TokenType::VOID_KW && base->type_keyword_token.type != TokenType::SOL_KW))
                {
                    push_semantic_error(errors, stmt->name.line, stmt->name.column_start, "Function '" + stmt->name.lexeme + "' must have a return statement.");
                }
            }
            end_scope();
        }
        void SemanticAnalyzer::visitReturnStmt(AST::ReturnStmt *stmt)
        {
            if (stmt->value)
                stmt->value->accept(this);
        }
        void SemanticAnalyzer::visitBreakStmt(AST::BreakStmt *stmt)
        {
            // Kiểm tra break ngoài vòng lặp hoặc switch
            bool in_loop_or_switch = false;
            for (auto it = var_scopes.rbegin(); it != var_scopes.rend(); ++it)
            {
                if (it->count("__loop__") || it->count("__switch__"))
                {
                    in_loop_or_switch = true;
                    break;
                }
            }
            if (!in_loop_or_switch)
            {
                push_semantic_error(errors, stmt->getLine(), stmt->getCol(), "'break' statement not inside a loop or switch.");
            }
        }
        void SemanticAnalyzer::visitContinueStmt(AST::ContinueStmt *stmt)
        {
            // Kiểm tra continue ngoài vòng lặp hoặc switch
            bool in_loop_or_switch = false;
            for (auto it = var_scopes.rbegin(); it != var_scopes.rend(); ++it)
            {
                if (it->count("__loop__") || it->count("__switch__"))
                {
                    in_loop_or_switch = true;
                    break;
                }
            }
            if (!in_loop_or_switch)
            {
                push_semantic_error(errors, stmt->getLine(), stmt->getCol(), "'continue' statement not inside a loop or switch.");
            }
        }
        void SemanticAnalyzer::visitSwitchStmt(AST::SwitchStmt *stmt)
        {
            if (stmt->expression_to_switch_on)
                stmt->expression_to_switch_on->accept(this);

            // Đánh dấu scope là trong switch
            begin_scope();
            var_scopes.back()["__switch__"] = true;
            for (const auto &c : stmt->cases)
            {
                for (const auto &s : c.statements)
                {
                    if (s)
                        s->accept(this);
                }
            }
            end_scope();
        }
        void SemanticAnalyzer::visitDeleteStmt(AST::DeleteStmt *stmt)
        {
            if (stmt->expression_to_delete)
                stmt->expression_to_delete->accept(this);
        }
        void SemanticAnalyzer::visitThrowStmt(AST::ThrowStmt *stmt)
        {
            if (stmt->expression)
                stmt->expression->accept(this);
        }
        void SemanticAnalyzer::visitTryStmt(AST::TryStmt *stmt)
        {
            if (stmt->try_block)
                stmt->try_block->accept(this);
            for (const auto &c : stmt->catch_clauses)
            {
                // --- Sửa tại đây: Đưa biến catch vào scope ---
                if (c.exception_variable.has_value())
                {
                    begin_scope();
                    declare_var(c.exception_variable.value().lexeme);
                    if (c.body)
                        c.body->accept(this);
                    end_scope();
                }
                else
                {
                    if (c.body)
                        c.body->accept(this);
                }
            }
            if (stmt->finally_block.has_value() && stmt->finally_block.value())
            {
                stmt->finally_block.value()->accept(this);
            }
        }

        void SemanticAnalyzer::visitImportStmt(AST::ImportStmt *stmt)
        {
            // Đảm bảo chỉ xử lý import module dạng: import module_name;
            if (!stmt->module_name.lexeme.empty())
            {
                std::string module_name = stmt->module_name.lexeme;
                
                // Check if it's a LiPM package first
                if (Linh::LiPM::package_exists(module_name))
                {
                    // This is a LiPM package, mark it as imported
                    imported_packages.insert(module_name);
                    return;
                }
                
                // Fall back to file-based module import
                std::string module_path = "Li/" + module_name;
                if (module_path.find(".li") == std::string::npos)
                    module_path += ".li";
                std::ifstream mod_file(module_path);
                if (!mod_file)
                {
                    push_semantic_error(errors, stmt->module_name.line, stmt->module_name.column_start, "Cannot open module file: " + module_path);
                    return;
                }
                std::string line, mod_source;
                while (std::getline(mod_file, line))
                    mod_source += line + "\n";
                // Lex và parse module
                Linh::Lexer mod_lexer(mod_source);
                auto mod_tokens = mod_lexer.scan_tokens();
                Linh::Parser mod_parser(mod_tokens);
                auto mod_ast = mod_parser.parse();
                if (mod_parser.had_error())
                {
                    push_semantic_error(errors, stmt->module_name.line, stmt->module_name.column_start, "Syntax error in module: " + module_path);
                    return;
                }
                // Phân tích semantic cho module (không reset state để giữ lại các hàm/biến)
                this->analyze(mod_ast, false);

                // --- Sinh bytecode cho module và merge function table ---
                Linh::BytecodeEmitter mod_emitter;
                mod_emitter.emit(mod_ast);
                // Giả sử bạn có một con trỏ emitter chính hoặc một biến toàn cục để merge
                if (g_main_emitter)
                {
                    auto &main_funcs = g_main_emitter->get_functions(); // non-const reference
                    for (const auto &kv : mod_emitter.get_functions())
                    {
                        // Nếu chưa có trong main emitter thì thêm vào
                        if (main_funcs.count(kv.first) == 0)
                        {
                            main_funcs.insert(kv);
                        }
                    }
                }
                // --- Kết thúc merge ---
            }
            // ...nếu muốn hỗ trợ import name from module thì xử lý thêm ở đây...
        }

        // ExprVisitor (only need to traverse, except UninitLiteralExpr)
        std::any SemanticAnalyzer::visitBinaryExpr(AST::BinaryExpr *expr)
        {
            if (expr->left)
                expr->left->accept(this);
            if (expr->right)
                expr->right->accept(this);
            return {};
        }
        std::any SemanticAnalyzer::visitUnaryExpr(AST::UnaryExpr *expr)
        {
            if (expr->right)
                expr->right->accept(this);
            return {};
        }
        std::any SemanticAnalyzer::visitLiteralExpr(AST::LiteralExpr *) { return {}; }
        std::any SemanticAnalyzer::visitGroupingExpr(AST::GroupingExpr *expr)
        {
            if (expr->expression)
                expr->expression->accept(this);
            return {};
        }
        std::any SemanticAnalyzer::visitIdentifierExpr(AST::IdentifierExpr *expr)
        {
            // Allow built-in functions and packages as identifiers without declaration
            static const std::unordered_set<std::string> builtin_funcs = {
                "input", "type", "str", "int", "float", "bool", "uint", "id", "bytes", "bin"}; // Thêm "id", bytes
            static const std::unordered_set<std::string> builtin_packages = {
                "math", "fs"}; // Built-in packages
            if (builtin_funcs.count(expr->name.lexeme) || builtin_packages.count(expr->name.lexeme))
            {
                return {};
            }
            // --- Sửa tại đây: Cho phép error.message nếu error đã khai báo ---
            const std::string &lex = expr->name.lexeme;
            auto dot_pos = lex.find('.');
            if (dot_pos != std::string::npos)
            {
                std::string base = lex.substr(0, dot_pos);
                std::string member = lex.substr(dot_pos + 1);
                
                // Check if this is a package constant (e.g., math.pi)
                if (imported_packages.count(base) || base == "math" || base == "fs")
                {
                    // This is a package constant, check if it exists
                    if (Linh::LiPM::get_constant(base, member).index() != 0) // Not sol
                    {
                        return {}; // Package constant exists, allow it
                    }
                    else
                    {
                        push_semantic_error(errors, expr->name.line, expr->name.column_start, "Package '" + base + "' does not have constant '" + member + "'.");
                        return {};
                    }
                }
                else if (is_var_declared(base))
                {
                    // Cho phép error.message nếu error đã khai báo
                    return {};
                }
            }
            // Ưu tiên kiểm tra hàm trước biến
            if (is_function_declared(expr->name.lexeme))
            {
                // Nếu là tên hàm, không báo lỗi dùng như biến (cho phép dùng tên hàm như giá trị hàm)
                return {};
            }
            // Kiểm tra biến đã khai báo chưa
            if (!is_var_declared(expr->name.lexeme))
            {
                push_semantic_error(errors, expr->name.line, expr->name.column_start, "Variable '" + expr->name.lexeme + "' used before declaration.");
            }
            return {};
        }
        std::any SemanticAnalyzer::visitAssignmentExpr(AST::AssignmentExpr *expr)
        {
            std::string name = expr->name.lexeme;
            if (!name.empty())
            {
                // Không cho phép gán lại cho const
                if (var_kinds.count(name) && var_kinds[name] == "const")
                {
                    push_semantic_error(errors, expr->name.line, expr->name.column_start, "Cannot assign to '" + name + "' because it is declared as 'const'.");
                }
                // Không cho phép đổi kiểu cho vas
                else if (var_kinds.count(name) && var_kinds[name] == "vas" && var_types.count(name))
                {
                    std::string old_type = var_types[name];
                    std::string new_type;
                    // Nếu là literal thì lấy kiểu như cũ
                    if (expr->value)
                    {
                        if (auto lit = dynamic_cast<AST::LiteralExpr *>(expr->value.get()))
                        {
                            new_type = get_linh_literal_type(lit);
                        }
                        else if (auto id = dynamic_cast<AST::IdentifierExpr *>(expr->value.get()))
                        {
                            // Nếu gán từ biến khác, lấy kiểu của biến đó nếu có
                            std::string rhs_name = id->name.lexeme;
                            if (var_types.count(rhs_name))
                                new_type = var_types[rhs_name];
                        }
                        else if (auto bin = dynamic_cast<AST::BinaryExpr *>(expr->value.get()))
                        {
                            // Nếu là biểu thức nhị phân, thử lấy kiểu của vế trái hoặc phải nếu là literal/identifier
                            std::string left_type, right_type;
                            if (auto litl = dynamic_cast<AST::LiteralExpr *>(bin->left.get()))
                                left_type = get_linh_literal_type(litl);
                            else if (auto idl = dynamic_cast<AST::IdentifierExpr *>(bin->left.get()))
                                if (var_types.count(idl->name.lexeme))
                                    left_type = var_types[idl->name.lexeme];
                            if (auto litr = dynamic_cast<AST::LiteralExpr *>(bin->right.get()))
                                right_type = get_linh_literal_type(litr);
                            else if (auto idr = dynamic_cast<AST::IdentifierExpr *>(bin->right.get()))
                                if (var_types.count(idr->name.lexeme))
                                    right_type = var_types[idr->name.lexeme];
                            // Ưu tiên lấy kiểu giống old_type nếu có
                            if (left_type == old_type)
                                new_type = left_type;
                            else if (right_type == old_type)
                                new_type = right_type;
                            else if (!left_type.empty())
                                new_type = left_type;
                            else if (!right_type.empty())
                                new_type = right_type;
                        }
                        // Có thể mở rộng cho các loại biểu thức khác nếu cần
                    }
                    if (!new_type.empty() && !old_type.empty() && !is_subtype_type_string(new_type, old_type))
                    {
                        push_semantic_error(errors, expr->name.line, expr->name.column_start, "Cannot change type of 'vas' variable '" + name + "' from '" + old_type + "' to '" + new_type + "'.");
                    }
                    // Nếu không xác định được kiểu mới (biểu thức phức tạp), không cho phép đổi kiểu
                    if (new_type.empty() && expr->value)
                    {
                        push_semantic_error(errors, expr->name.line, expr->name.column_start, "Cannot assign non-literal or unknown type to 'vas' variable '" + name + "'.");
                    }
                }
                // Nếu là var thì cho phép đổi kiểu (cập nhật lại var_types)
                else if (var_kinds.count(name) && var_kinds[name] == "var" && var_types.count(name))
                {
                    std::string new_type;
                    if (expr->value)
                    {
                        if (auto lit = dynamic_cast<AST::LiteralExpr *>(expr->value.get()))
                        {
                            new_type = get_linh_literal_type(lit);
                        }
                        else if (auto id = dynamic_cast<AST::IdentifierExpr *>(expr->value.get()))
                        {
                            std::string rhs_name = id->name.lexeme;
                            if (var_types.count(rhs_name))
                                new_type = var_types[rhs_name];
                        }
                        else if (auto bin = dynamic_cast<AST::BinaryExpr *>(expr->value.get()))
                        {
                            std::string left_type, right_type;
                            if (auto litl = dynamic_cast<AST::LiteralExpr *>(bin->left.get()))
                                left_type = get_linh_literal_type(litl);
                            else if (auto idl = dynamic_cast<AST::IdentifierExpr *>(bin->left.get()))
                                if (var_types.count(idl->name.lexeme))
                                    left_type = var_types[idl->name.lexeme];
                            if (auto litr = dynamic_cast<AST::LiteralExpr *>(bin->right.get()))
                                right_type = get_linh_literal_type(litr);
                            else if (auto idr = dynamic_cast<AST::IdentifierExpr *>(bin->right.get()))
                                if (var_types.count(idr->name.lexeme))
                                    right_type = var_types[idr->name.lexeme];
                            if (!left_type.empty())
                                new_type = left_type;
                            else if (!right_type.empty())
                                new_type = right_type;
                        }
                    }
                    if (!new_type.empty())
                    {
                        var_types[name] = new_type;
                    }
                }
                // --- Cắt chuỗi khi gán lại cho biến kiểu str<index> ---
                if (var_types.count(name) && var_types[name] == "str" && var_str_limit.count(name))
                {
                    int str_limit = var_str_limit[name];
                    if (str_limit > 0 && expr->value)
                    {
                        if (auto lit = dynamic_cast<AST::LiteralExpr *>(expr->value.get()))
                        {
                            if (std::holds_alternative<std::string>(lit->value))
                            {
                                std::string val = std::get<std::string>(lit->value);
                                if (static_cast<int>(val.size()) > str_limit)
                                {
                                    std::string cut_val = val.substr(0, str_limit);
                                    lit->value = cut_val;
                                }
                            }
                        }
                    }
                }
            }
            if (expr->value)
                expr->value->accept(this);
            return {};
        }
        std::any SemanticAnalyzer::visitLogicalExpr(AST::LogicalExpr *expr)
        {
            if (expr->left)
                expr->left->accept(this);
            if (expr->right)
                expr->right->accept(this);
            return {};
        }
        std::any SemanticAnalyzer::visitCallExpr(AST::CallExpr *expr)
        {
            // Nếu callee là IdentifierExpr thì kiểm tra tên hàm
            if (auto id = dynamic_cast<AST::IdentifierExpr *>(expr->callee.get()))
            {
                // --- BẮT LỖI printil('...') ---
                if (id->name.lexeme == "printil" && !expr->arguments.empty())
                {
                    auto *arg0 = expr->arguments[0].get();
                    if (auto lit = dynamic_cast<AST::LiteralExpr *>(arg0))
                    {
                        // Kiểm tra token.lexeme bắt đầu và kết thúc bằng dấu nháy đơn
                        const std::string &tok_lex = lit->token.lexeme;
                        if (tok_lex.size() >= 2 && tok_lex.front() == '\'' && tok_lex.back() == '\'')
                        {
                            push_semantic_error(errors, lit->getLine(), lit->getCol(), "printil() argument must use double quotes (\"...\") not single quotes (\'...\').");
                        }
                    }
                }
                // --- GIỚI HẠN bool(x) chỉ cho phép 0, 1, 0.0, 1.0 ---
                if (id->name.lexeme == "bool" && expr->arguments.size() == 1) {
                    auto *arg0 = expr->arguments[0].get();
                    bool valid = false;
                    if (auto lit = dynamic_cast<AST::LiteralExpr *>(arg0)) {
                        if (std::holds_alternative<int64_t>(lit->value)) {
                            int64_t v = std::get<int64_t>(lit->value);
                            valid = (v == 0 || v == 1);
                        } else if (std::holds_alternative<double>(lit->value)) {
                            double v = std::get<double>(lit->value);
                            valid = (v == 0.0 || v == 1.0);
                        }
                    }
                    // Nếu không phải literal 0/1/0.0/1.0 thì luôn báo lỗi
                    if (!valid) {
                        push_semantic_error(errors, expr->getLine(), expr->getCol(), "Only literal 0, 1, 0.0, 1.0 are allowed for bool() conversion.");
                    }
                }
                // Allow built-in conversion functions without declaration
                static const std::unordered_set<std::string> builtin_funcs = {
                    "input", "type", "str", "int", "float", "bool", "uint", "id", "bytes", "bin"}; // Thêm "id", bytes
                if (!builtin_funcs.count(id->name.lexeme))
                {
                    // Nếu là function đã khai báo thì kiểm tra như cũ
                    if (is_function_declared(id->name.lexeme)) {
                        // Kiểm tra số lượng tham số khi gọi hàm (trừ input, type)
                        if (id->name.lexeme != "input" && id->name.lexeme != "type" && function_param_counts.count(id->name.lexeme))
                        {
                            size_t expected = function_param_counts[id->name.lexeme];
                            size_t actual = expr->arguments.size();
                            if (expected != actual)
                            {
                                push_semantic_error(errors, id->name.line, id->name.column_start, "Function '" + id->name.lexeme + "' called with wrong number of arguments (expected " + std::to_string(expected) + ", got " + std::to_string(actual) + ").");
                            }
                        }
                    } else {
                        // Nếu là biến kiểu function object thì cho phép gọi như hàm
                        if (var_types.count(id->name.lexeme) && var_types[id->name.lexeme] == "function") {
                            // Kiểm tra số lượng tham số nếu có thể
                            if (var_func_param_counts.count(id->name.lexeme)) {
                                size_t expected = var_func_param_counts[id->name.lexeme];
                                size_t actual = expr->arguments.size();
                                if (expected != actual) {
                                    push_semantic_error(errors, id->name.line, id->name.column_start, "Function variable '" + id->name.lexeme + "' called with wrong number of arguments (expected " + std::to_string(expected) + ", got " + std::to_string(actual) + ").");
                                }
                            }
                        } else {
                            // Kiểm tra xem có phải là variable đã khai báo không (bao gồm parameters)
                            if (is_var_declared(id->name.lexeme)) {
                                // Variable đã khai báo, có thể là function object hoặc parameter
                                // Cho phép gọi như function
                            } else {
                                // Không phải function object, không phải variable đã khai báo, báo lỗi
                                push_semantic_error(errors, id->name.line, id->name.column_start, "Function '" + id->name.lexeme + "' called but not declared.");
                            }
                        }
                    }
                }
            }
            if (expr->callee)
                expr->callee->accept(this);
            for (const auto &arg : expr->arguments)
            {
                if (arg)
                    arg->accept(this);
            }
            return {};
        }
        std::any SemanticAnalyzer::visitPostfixExpr(AST::PostfixExpr *expr)
        {
            if (expr->operand)
                expr->operand->accept(this);
            return {};
        }
        std::any SemanticAnalyzer::visitUninitLiteralExpr(AST::UninitLiteralExpr *)
        {
            // Nothing to do here
            return {};
        }
        std::any SemanticAnalyzer::visitNewExpr(AST::NewExpr *expr)
        {
            if (expr->class_constructor_call)
                expr->class_constructor_call->accept(this);
            return {};
        }
        std::any SemanticAnalyzer::visitThisExpr(AST::ThisExpr *) { return {}; }
        std::any SemanticAnalyzer::visitArrayLiteralExpr(AST::ArrayLiteralExpr *expr)
        {
            for (const auto &el : expr->elements)
            {
                if (el)
                    el->accept(this);
            }
            return {};
        }
        std::any SemanticAnalyzer::visitMapLiteralExpr(AST::MapLiteralExpr *expr)
        {
            for (const auto &entry : expr->entries)
            {
                if (entry.key)
                    entry.key->accept(this);
                if (entry.value)
                    entry.value->accept(this);
            }
            return {};
        }
        std::any SemanticAnalyzer::visitSubscriptExpr(AST::SubscriptExpr *expr)
        {
            if (expr->object)
                expr->object->accept(this);
            if (expr->index)
                expr->index->accept(this);
            return {};
        }
        std::any SemanticAnalyzer::visitInterpolatedStringExpr(AST::InterpolatedStringExpr *expr)
        {
            for (const auto &part : expr->parts)
            {
                if (std::holds_alternative<AST::ExprPtr>(part))
                {
                    auto &e = std::get<AST::ExprPtr>(part);
                    if (e)
                        e->accept(this);
                }
            }
            return {};
        }
        std::any SemanticAnalyzer::visitMemberExpr(AST::MemberExpr *expr)
        {
            // Kiểm tra xem object có phải là package đã import không
            if (auto id = dynamic_cast<AST::IdentifierExpr *>(expr->object.get()))
            {
                std::string package_name = id->name.lexeme;
                std::string property_name = expr->property_token.lexeme;
                
                // Kiểm tra xem package có được import không hoặc là built-in package
                if (imported_packages.count(package_name) > 0 || package_name == "math" || package_name == "fs")
                {
#ifdef _DEBUG
                    std::cerr << "[DEBUG] Found package: " << package_name << "." << property_name << std::endl;
#endif
                    // Đánh dấu đây là package constant
                    expr->is_package_constant = true;
                    expr->package_name = package_name;
                    expr->constant_name = property_name;
                }
            }
            
            // Vẫn gọi accept cho object để semantic analysis bình thường
            if (expr->object)
                expr->object->accept(this);
            return {};
        }
        std::any SemanticAnalyzer::visitMethodCallExpr(AST::MethodCallExpr *expr)
        {
            if (expr->object)
                expr->object->accept(this);
            for (const auto &arg : expr->arguments)
                if (arg)
                    arg->accept(this);
            return {};
        }
        std::any SemanticAnalyzer::visitFunctionExpr(AST::FunctionExpr *expr)
        {
            // Cho phép anonymous function expression, không cần kiểm tra gì đặc biệt
            // Có thể mở rộng kiểm tra tham số, return type nếu cần
            return {};
        }

        // Helper: xác định kiểu literal cho Linh từ LiteralExpr
        std::string SemanticAnalyzer::get_linh_literal_type(const AST::LiteralExpr *lit)
        {
            if (!lit)
                return "";
            // Giả sử LiteralExpr có trường value kiểu std::variant<std::monostate, int64_t, uint64_t, double, std::string, bool>
            if (std::holds_alternative<int64_t>(lit->value))
                return "int";
            if (std::holds_alternative<uint64_t>(lit->value))
                return "uint";
            if (std::holds_alternative<double>(lit->value))
                return "float";
            if (std::holds_alternative<std::string>(lit->value))
                return "str";
            if (std::holds_alternative<bool>(lit->value))
                return "bool";
            // Có thể bổ sung các kiểu khác nếu cần
            return "";
        }

        // Helpers
        bool SemanticAnalyzer::is_sol_type(const std::optional<AST::TypeNodePtr> &type)
        {
            if (!type.has_value() || !type.value())
                return false;
            auto *base = dynamic_cast<AST::BaseTypeNode *>(type.value().get());
            return base && base->type_keyword_token.type == TokenType::SOL_KW;
        }

        bool SemanticAnalyzer::is_sol_expr(const AST::ExprPtr &expr)
        {
            if (!expr)
                return false;
            return dynamic_cast<AST::UninitLiteralExpr *>(expr.get()) != nullptr;
        }

        // Semantic error retrieval
        const std::vector<Linh::Error> &SemanticAnalyzer::get_errors() const
        {
            return errors;
        }

    } // namespace Semantic
} // namespace Linh