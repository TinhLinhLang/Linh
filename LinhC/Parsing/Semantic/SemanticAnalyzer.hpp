#pragma once
#include "../AST/ASTNode.hpp"
#include "../../Error.hpp"
#include "../../../LiVM/Std/Std.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <stack>
#include <unordered_set>
#include <memory>
#include <future>

namespace Linh
{
    namespace Semantic
    {

        class SemanticAnalyzer : public AST::StmtVisitor, public AST::ExprVisitor
        {
        public:
            std::vector<Linh::Error> errors;

            void analyze(const AST::StmtList &stmts, bool reset_state = true);

            // Optimization methods
            void enable_caching(bool enable = true) { caching_enabled = enable; }
            void enable_early_exit(bool enable = true) { early_exit_enabled = enable; }
            void enable_parallel_analysis(bool enable = true) { parallel_analysis_enabled = enable; }
            
            // Performance monitoring
            size_t get_analysis_time_ms() const { return analysis_time_ms; }
            size_t get_cache_hits() const { return cache_hits; }
            size_t get_cache_misses() const { return cache_misses; }

            // StmtVisitor overrides
            void visitExpressionStmt(AST::ExpressionStmt *stmt) override;
            void visitPrintStmt(AST::PrintStmt *stmt) override;
            void visitVarDeclStmt(AST::VarDeclStmt *stmt) override;
            void visitBlockStmt(AST::BlockStmt *stmt) override;
            void visitIfStmt(AST::IfStmt *stmt) override;
            void visitWhileStmt(AST::WhileStmt *stmt) override;
            void visitFunctionDeclStmt(AST::FunctionDeclStmt *stmt) override;
            void visitReturnStmt(AST::ReturnStmt *stmt) override;
            void visitBreakStmt(AST::BreakStmt *stmt) override;
            void visitContinueStmt(AST::ContinueStmt *stmt) override;
            void visitSwitchStmt(AST::SwitchStmt *stmt) override;
            void visitDeleteStmt(AST::DeleteStmt *stmt) override;
            void visitThrowStmt(AST::ThrowStmt *stmt) override;
            void visitTryStmt(AST::TryStmt *stmt) override;
            void visitImportStmt(AST::ImportStmt *stmt) override;
            void visitExportStmt(AST::ExportStmt *stmt) override;

            // ExprVisitor overrides (only need UninitLiteralExpr for this rule)
            std::any visitBinaryExpr(AST::BinaryExpr *expr) override;
            std::any visitUnaryExpr(AST::UnaryExpr *expr) override;
            std::any visitLiteralExpr(AST::LiteralExpr *expr) override;
            std::any visitGroupingExpr(AST::GroupingExpr *expr) override;
            std::any visitIdentifierExpr(AST::IdentifierExpr *expr) override;
            std::any visitAssignmentExpr(AST::AssignmentExpr *expr) override;
            std::any visitLogicalExpr(AST::LogicalExpr *expr) override;
            std::any visitCallExpr(AST::CallExpr *expr) override;
            std::any visitPostfixExpr(AST::PostfixExpr *expr) override;
            std::any visitUninitLiteralExpr(AST::UninitLiteralExpr *expr) override;
            std::any visitNewExpr(AST::NewExpr *expr) override;
            std::any visitThisExpr(AST::ThisExpr *expr) override;
            std::any visitArrayLiteralExpr(AST::ArrayLiteralExpr *expr) override;
            std::any visitMapLiteralExpr(AST::MapLiteralExpr *expr) override;
            std::any visitSubscriptExpr(AST::SubscriptExpr *expr) override;
            std::any visitInterpolatedStringExpr(AST::InterpolatedStringExpr *expr) override;
            std::any visitMemberExpr(AST::MemberExpr *expr) override;
            std::any visitMethodCallExpr(AST::MethodCallExpr *expr) override;
            std::any visitFunctionExpr(AST::FunctionExpr *expr) override;

            const std::vector<Linh::Error> &get_errors() const;
            void set_current_file_path(const std::string& path);

        private:
            // Current file path for module resolution
            std::string current_file_path;
            
            // Optimization flags
            bool caching_enabled = true;
            bool early_exit_enabled = true;
            bool parallel_analysis_enabled = false;
            
            // Performance tracking
            size_t analysis_time_ms = 0;
            size_t cache_hits = 0;
            size_t cache_misses = 0;
            
            // Caching for type checking
            std::unordered_map<std::string, std::string> type_cache; // expression_hash -> type
            std::unordered_map<std::string, bool> error_cache; // error_key -> has_error
            
            // Early exit tracking
            bool should_early_exit = false;
            
            // Context flags
            bool in_index_context = false; // true when visiting the index expression of a subscript (e.g., map[key])
            
            // Helper methods for optimization
            std::string get_expression_hash(AST::Expr* expr);
            std::string get_error_key(const AST::Stmt* stmt, const std::string& error_type);
            std::string get_error_key(const AST::Expr* expr, const std::string& error_type);
            bool check_cached_error(const AST::Stmt* stmt, const std::string& error_type);
            bool check_cached_error(const AST::Expr* expr, const std::string& error_type);
            void cache_error(const AST::Stmt* stmt, const std::string& error_type);
            void cache_error(const AST::Expr* expr, const std::string& error_type);
            
            // Parallel analysis helpers
            void analyze_statement_parallel(AST::Stmt* stmt);
            std::vector<Linh::Error> merge_errors(const std::vector<std::vector<Linh::Error>>& error_lists);

            bool is_sol_type(const std::optional<AST::TypeNodePtr> &type);
            bool is_sol_expr(const AST::ExprPtr &expr);

            // --- Quản lý scope ---
            std::vector<std::unordered_map<std::string, bool>> var_scopes;
            std::unordered_map<std::string, bool> global_functions;

            // --- Bổ sung quản lý loại biến, kiểu biến, số lượng tham số hàm ---
            std::unordered_map<std::string, std::string> var_types;        // tên biến -> kiểu
            std::unordered_map<std::string, std::string> var_kinds;        // tên biến -> "vas"/"const"/"var"
            std::unordered_map<std::string, size_t> function_param_counts; // tên hàm -> số lượng tham số
            std::unordered_map<std::string, size_t> var_func_param_counts; // tên biến function object -> số lượng tham số

            // --- Bổ sung quản lý str_limit cho từng biến kiểu str<index> ---
            std::unordered_map<std::string, int> var_str_limit; // tên biến -> str_limit

            // --- LiPM package management ---
            std::unordered_set<std::string> imported_packages; // Track imported LiPM packages

            void begin_scope();
            void end_scope();
            void declare_var(const std::string &name, const std::string &kind = "", const std::string &type = "");
            bool is_var_declared(const std::string &name);
            void declare_function(const std::string &name, size_t param_count = 0);
            bool is_function_declared(const std::string &name);

            // Helper: xác định kiểu literal cho Linh từ LiteralExpr
            static std::string get_linh_literal_type(const AST::LiteralExpr *lit);
        };

    } // namespace Semantic
} // namespace Linh