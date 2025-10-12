// --- LinhC/Parsing/AST/ASTPrinter.hpp ---
#ifndef LINH_AST_PRINTER_HPP
#define LINH_AST_PRINTER_HPP

#include "ASTNode.hpp" // ASTNode.hpp đã bao gồm khai báo cho TypeVisitor
#include <string>
#include <sstream>
#include <vector>
#include <optional> // Để sử dụng std::optional

namespace Linh
{
    namespace AST
    {
        // ASTPrinter bây giờ kế thừa từ ExprVisitor, StmtVisitor, VÀ TypeVisitor
        class ASTPrinter : public ExprVisitor, public StmtVisitor, public TypeVisitor
        {
        public:
            std::string print(const StmtList &statements);
            std::string print_expr(Expr *expr); // Public helper cho biểu thức đơn lẻ nếu cần

            // ExprVisitor implementations
            std::any visitBinaryExpr(BinaryExpr *expr) override;
            std::any visitUnaryExpr(UnaryExpr *expr) override;
            std::any visitLiteralExpr(LiteralExpr *expr) override;
            std::any visitGroupingExpr(GroupingExpr *expr) override;
            std::any visitIdentifierExpr(IdentifierExpr *expr) override;
            std::any visitAssignmentExpr(AssignmentExpr *expr) override;
            std::any visitLogicalExpr(LogicalExpr *expr) override;
            std::any visitCallExpr(CallExpr *expr) override;
            std::any visitPostfixExpr(PostfixExpr *expr) override;
            std::any visitUninitLiteralExpr(UninitLiteralExpr *expr) override;
            std::any visitNewExpr(NewExpr *expr) override;
            std::any visitThisExpr(ThisExpr *expr) override;
            std::any visitArrayLiteralExpr(ArrayLiteralExpr *expr) override;
            std::any visitMapLiteralExpr(MapLiteralExpr *expr) override;
            std::any visitSubscriptExpr(SubscriptExpr *expr) override; // MỚI
            std::any visitInterpolatedStringExpr(InterpolatedStringExpr *expr) override;
            std::any visitMethodCallExpr(MethodCallExpr *expr) override; // <--- Thêm dòng này

            // StmtVisitor implementations
            void visitExpressionStmt(ExpressionStmt *stmt) override;
            void visitPrintStmt(PrintStmt *stmt) override;
            void visitVarDeclStmt(VarDeclStmt *stmt) override;
            void visitBlockStmt(BlockStmt *stmt) override;
            void visitIfStmt(IfStmt *stmt) override;
            void visitWhileStmt(WhileStmt *stmt) override;
            void visitFunctionDeclStmt(FunctionDeclStmt *stmt) override;
            void visitReturnStmt(ReturnStmt *stmt) override;
            void visitBreakStmt(BreakStmt *stmt) override;
            void visitContinueStmt(ContinueStmt *stmt) override;
            void visitSwitchStmt(SwitchStmt *stmt) override;
            void visitDeleteStmt(DeleteStmt *stmt) override;
            void visitThrowStmt(ThrowStmt *stmt) override;
            void visitTryStmt(TryStmt *stmt) override;
            void visitImportStmt(ImportStmt *stmt) override; // <--- Thêm dòng này

            // TypeVisitor implementations
            std::string visitBaseTypeNode(BaseTypeNode *type_node) override;
            std::string visitSizedIntegerTypeNode(SizedIntegerTypeNode *type_node) override;
            std::string visitSizedFloatTypeNode(SizedFloatTypeNode *type_node) override;
            std::string visitMapTypeNode(MapTypeNode *type_node) override;
            std::string visitArrayTypeNode(ArrayTypeNode *type_node) override;
            std::string visitUnionTypeNode(UnionTypeNode *type_node) override;

        private:
            std::ostringstream m_builder;
            int m_indent_level = 0;

            void indent();
            std::string parenthesize_expr_visit(const std::string &name, const std::vector<Expr *> &exprs);
            void build_stmt_structure(const std::string &name,
                                      const std::vector<Expr *> &expr_fields = {},
                                      const std::vector<Stmt *> &stmt_fields = {},
                                      const std::vector<BlockStmt *> &direct_block_fields = {},
                                      const std::vector<std::unique_ptr<BlockStmt> *> &unique_ptr_block_fields = {},
                                      const std::vector<std::pair<std::string, std::string>> &text_fields = {});
            void visit_stmt_list_indented(const StmtList &stmts);
            std::string print_optional_expr(const std::optional<ExprPtr> &opt_expr);

            // Các hàm trợ giúp mới cho kiểu
            std::string print_type_node(TypeNode *type_node);
            std::string print_optional_type_node(const std::optional<TypeNodePtr> &opt_type_node);
        };

    } // namespace AST
} // namespace Linh
#endif // LINH_AST_PRINTER_HPP