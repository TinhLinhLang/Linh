#ifndef LINH_AST_NODE_HPP
#define LINH_AST_NODE_HPP

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <any>
#include <optional>
#include "../Lexer/Lexer.hpp"

namespace Linh
{
    namespace AST
    {
        // --- Forward declarations for statements and blocks ---
        struct BlockStmt;
        struct IfStmt;
        struct WhileStmt;
        struct FunctionDeclStmt;
        struct ReturnStmt;
        struct BreakStmt;
        struct ContinueStmt;
        struct SwitchStmt;
        struct DeleteStmt;
        struct ThrowStmt;
        struct TryStmt;
        struct ImportStmt;
        struct ExportStmt;
        struct PrintStmt;
        struct ExpressionStmt;
        struct VarDeclStmt;
        struct CaseClause;
        struct CatchClauseNode;
        struct ArrayLiteralExpr;
        struct MapLiteralExpr;
        struct SubscriptExpr;
        struct UninitLiteralExpr;
        struct NewExpr;
        struct ThisExpr;
        struct GroupingExpr;
        struct FuncParamNode;
        struct FunctionExpr;

        struct Stmt
        {
            virtual ~Stmt() = default;
            virtual void accept(class StmtVisitor *) = 0;
        }; // Định nghĩa rỗng
        class StmtVisitor
        {
        public:
            virtual ~StmtVisitor() = default;
            virtual void visitPrintStmt(PrintStmt *stmt) = 0;
            virtual void visitExpressionStmt(ExpressionStmt *stmt) = 0;
            virtual void visitVarDeclStmt(VarDeclStmt *stmt) = 0;
            virtual void visitBlockStmt(BlockStmt *stmt) = 0;
            virtual void visitIfStmt(IfStmt *stmt) = 0;
            virtual void visitWhileStmt(WhileStmt *stmt) = 0;
            virtual void visitFunctionDeclStmt(FunctionDeclStmt *stmt) = 0;
            virtual void visitReturnStmt(ReturnStmt *stmt) = 0;
            virtual void visitBreakStmt(BreakStmt *stmt) = 0;
            virtual void visitContinueStmt(ContinueStmt *stmt) = 0;
            virtual void visitSwitchStmt(SwitchStmt *stmt) = 0;
            virtual void visitDeleteStmt(DeleteStmt *stmt) = 0;
            virtual void visitThrowStmt(ThrowStmt *stmt) = 0;
            virtual void visitTryStmt(TryStmt *stmt) = 0;
            virtual void visitImportStmt(ImportStmt *stmt) = 0;
            virtual void visitExportStmt(ExportStmt *stmt) = 0;
        }; // Định nghĩa rỗng
        using StmtPtr = std::unique_ptr<Stmt>;
        using StmtList = std::vector<StmtPtr>;
        struct BlockStmt;

        // --- Type System Nodes (Giữ nguyên từ lần cập nhật trước) ---
        struct TypeNode;
        struct BaseTypeNode;
        struct SizedIntegerTypeNode;
        struct SizedFloatTypeNode;
        struct MapTypeNode;
        struct ArrayTypeNode;
        struct UnionTypeNode;
        using TypeNodePtr = std::unique_ptr<TypeNode>;

        class TypeVisitor
        {
        public:
            virtual ~TypeVisitor() = default;
            virtual std::string visitBaseTypeNode(BaseTypeNode *type_node) = 0;
            virtual std::string visitSizedIntegerTypeNode(SizedIntegerTypeNode *type_node) = 0;
            virtual std::string visitSizedFloatTypeNode(SizedFloatTypeNode *type_node) = 0;
            virtual std::string visitMapTypeNode(MapTypeNode *type_node) = 0;
            virtual std::string visitArrayTypeNode(ArrayTypeNode *type_node) = 0;
            virtual std::string visitUnionTypeNode(UnionTypeNode *type_node) = 0;
        };
        struct TypeNode
        {
            virtual ~TypeNode() = default;
            virtual std::string accept(TypeVisitor *visitor) = 0;
        };
        struct BaseTypeNode : TypeNode
        {
            Token type_keyword_token;
            std::optional<int> template_arg; // <--- thêm dòng này
            BaseTypeNode(Token token) : type_keyword_token(std::move(token)), template_arg(std::nullopt) {}
            std::string accept(TypeVisitor *visitor) override { return visitor->visitBaseTypeNode(this); }
        };
        struct SizedIntegerTypeNode : TypeNode
        {
            Token base_type_keyword_token;
            Token size_token;
            std::optional<int> template_arg; // <--- thêm dòng này
            SizedIntegerTypeNode(Token base_token, Token size_tok)
                : base_type_keyword_token(std::move(base_token)), size_token(std::move(size_tok))
            {
                try
                {
                    template_arg = std::stoi(size_token.lexeme);
                }
                catch (...)
                {
                    template_arg = std::nullopt;
                }
            }
            std::string accept(TypeVisitor *visitor) override { return visitor->visitSizedIntegerTypeNode(this); }
        };
        struct SizedFloatTypeNode : TypeNode
        {
            Token base_type_keyword_token;
            Token size_token;
            std::optional<int> template_arg; // <--- thêm dòng này
            SizedFloatTypeNode(Token base_token, Token size_tok)
                : base_type_keyword_token(std::move(base_token)), size_token(std::move(size_tok))
            {
                try
                {
                    template_arg = std::stoi(size_token.lexeme);
                }
                catch (...)
                {
                    template_arg = std::nullopt;
                }
            }
            std::string accept(TypeVisitor *visitor) override { return visitor->visitSizedFloatTypeNode(this); }
        };
        struct MapTypeNode : TypeNode
        {
            Token map_keyword_token;
            TypeNodePtr key_type;
            TypeNodePtr value_type;
            MapTypeNode(Token map_kw_tok, TypeNodePtr k_type, TypeNodePtr v_type)
                : map_keyword_token(std::move(map_kw_tok)), key_type(std::move(k_type)), value_type(std::move(v_type)) {}
            std::string accept(TypeVisitor *visitor) override { return visitor->visitMapTypeNode(this); }
        };
        struct ArrayTypeNode : TypeNode
        {
            std::optional<Token> array_keyword_token;
            TypeNodePtr element_type;
            Token r_bracket_token;
            ArrayTypeNode(TypeNodePtr el_type, Token r_bracket)
                : array_keyword_token(std::nullopt), element_type(std::move(el_type)), r_bracket_token(std::move(r_bracket)) {}
            ArrayTypeNode(Token arr_kw_tok, TypeNodePtr el_type_for_array_kw, Token dummy_r_bracket_for_consistency)
                : array_keyword_token(std::move(arr_kw_tok)), element_type(std::move(el_type_for_array_kw)), r_bracket_token(std::move(dummy_r_bracket_for_consistency)) {}
            std::string accept(TypeVisitor *visitor) override { return visitor->visitArrayTypeNode(this); }
        };
        struct UnionTypeNode : TypeNode
        {
            std::vector<TypeNodePtr> types;
            Token l_angle_token, r_angle_token;
            UnionTypeNode(Token l_angle, std::vector<TypeNodePtr> t_list, Token r_angle)
                : types(std::move(t_list)), l_angle_token(std::move(l_angle)), r_angle_token(std::move(r_angle)) {}
            std::string accept(TypeVisitor *visitor) override { return visitor->visitUnionTypeNode(this); }
        };

        // --- ExprVisitor and Expr Nodes ---
        struct BinaryExpr;
        struct UnaryExpr;
        struct LiteralExpr;
        struct GroupingExpr;
        struct IdentifierExpr;
        struct AssignmentExpr;
        struct LogicalExpr;
        struct CallExpr;
        struct PostfixExpr;
        struct UninitLiteralExpr;
        struct NewExpr;
        struct ThisExpr;
        struct ArrayLiteralExpr;       // MỚI
        struct MapLiteralExpr;         // MỚI
        struct SubscriptExpr;          // MỚI
        struct InterpolatedStringExpr; // MỚI
        struct MemberExpr;             // MỚI
        struct MethodCallExpr;         // MỚI
        struct FunctionExpr;

        class ExprVisitor
        {
        public:
            virtual ~ExprVisitor() = default;
            virtual std::any visitBinaryExpr(BinaryExpr *expr) = 0;
            virtual std::any visitUnaryExpr(UnaryExpr *expr) = 0;
            virtual std::any visitLiteralExpr(LiteralExpr *expr) = 0;
            virtual std::any visitGroupingExpr(GroupingExpr *expr) = 0;
            virtual std::any visitIdentifierExpr(IdentifierExpr *expr) = 0;
            virtual std::any visitAssignmentExpr(AssignmentExpr *expr) = 0;
            virtual std::any visitLogicalExpr(LogicalExpr *expr) = 0;
            virtual std::any visitCallExpr(CallExpr *expr) = 0;
            virtual std::any visitPostfixExpr(PostfixExpr *expr) = 0;
            virtual std::any visitUninitLiteralExpr(UninitLiteralExpr *expr) = 0;
            virtual std::any visitNewExpr(NewExpr *expr) = 0;
            virtual std::any visitThisExpr(ThisExpr *expr) = 0;
            virtual std::any visitArrayLiteralExpr(ArrayLiteralExpr *expr) = 0;             // MỚI
            virtual std::any visitMapLiteralExpr(MapLiteralExpr *expr) = 0;                 // MỚI
            virtual std::any visitSubscriptExpr(SubscriptExpr *expr) = 0;                   // MỚI
            virtual std::any visitInterpolatedStringExpr(InterpolatedStringExpr *expr) = 0; // MỚI
            virtual std::any visitMemberExpr(MemberExpr *expr) = 0;                         // MỚI
            virtual std::any visitMethodCallExpr(MethodCallExpr *expr) = 0;                 // MỚI
            virtual std::any visitFunctionExpr(FunctionExpr *expr) = 0;
        };
        struct Expr
        {
            virtual ~Expr() = default;
            virtual std::any accept(ExprVisitor *visitor) = 0;
        };
        using ExprPtr = std::unique_ptr<Expr>;

        struct LiteralExpr : Expr
        {
            LiteralValue value;
            Token token; // Thêm trường này để lưu token gốc
            LiteralExpr(LiteralValue val) : value(std::move(val)), token() {}
            LiteralExpr(LiteralValue val, Token tok) : value(std::move(val)), token(std::move(tok)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitLiteralExpr(this); }
            int getLine() const { return token.line; }
            int getCol() const { return token.column_start; }
        };

        struct IdentifierExpr : Expr
        {
            Token name;
            IdentifierExpr(Token n) : name(std::move(n)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitIdentifierExpr(this); }
            int getLine() const { return name.line; }
            int getCol() const { return name.column_start; }
        };

        struct UnaryExpr : Expr
        {
            Token op;
            ExprPtr right;
            UnaryExpr(Token o, ExprPtr r) : op(std::move(o)), right(std::move(r)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitUnaryExpr(this); }
            int getLine() const { return op.line; }
            int getCol() const { return op.column_start; }
        };

        struct PostfixExpr : Expr
        {
            ExprPtr operand;
            Token op_token;
            PostfixExpr(ExprPtr left, Token op_tok) : operand(std::move(left)), op_token(std::move(op_tok)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitPostfixExpr(this); }
            int getLine() const { return op_token.line; }
            int getCol() const { return op_token.column_start; }
        };

        struct BinaryExpr : Expr
        {
            ExprPtr left;
            Token op;
            ExprPtr right;
            BinaryExpr(ExprPtr l, Token o, ExprPtr r) : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitBinaryExpr(this); }
            int getLine() const { return op.line; }
            int getCol() const { return op.column_start; }
        };

        struct LogicalExpr : Expr
        {
            ExprPtr left;
            Token op;
            ExprPtr right;
            LogicalExpr(ExprPtr l, Token o, ExprPtr r) : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitLogicalExpr(this); }
            int getLine() const { return op.line; }
            int getCol() const { return op.column_start; }
        };

        struct AssignmentExpr : Expr
        {
            Token name;
            ExprPtr value;
            AssignmentExpr(Token n, ExprPtr val) : name(std::move(n)), value(std::move(val)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitAssignmentExpr(this); }
            int getLine() const { return name.line; }
            int getCol() const { return name.column_start; }
        };

        struct CallExpr : Expr
        {
            ExprPtr callee;
            Token paren;
            std::vector<ExprPtr> arguments;
            CallExpr(ExprPtr callee_expr, Token p, std::vector<ExprPtr> args) : callee(std::move(callee_expr)), paren(std::move(p)), arguments(std::move(args)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitCallExpr(this); }
            int getLine() const { return paren.line; }
            int getCol() const { return paren.column_start; }
        };

        struct InterpolatedStringExpr : Expr
        {
            std::vector<std::variant<std::string, ExprPtr>> parts;
            Token first_token; // Thêm trường này để lưu vị trí
            InterpolatedStringExpr(std::vector<std::variant<std::string, ExprPtr>> p, Token tok = Token()) : parts(std::move(p)), first_token(tok) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitInterpolatedStringExpr(this); }
            int getLine() const { return first_token.line; }
            int getCol() const { return first_token.column_start; }
        };

        struct MemberExpr : Expr
        {
            ExprPtr object;
            std::string property;
            Token dot_token;
            Token property_token;
            // Package constant fields
            bool is_package_constant = false;
            std::string package_name;
            std::string constant_name;
            
            MemberExpr(ExprPtr obj, Token dot, Token prop_tok)
                : object(std::move(obj)), property(prop_tok.lexeme), dot_token(std::move(dot)), property_token(std::move(prop_tok)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitMemberExpr(this); }
            int getLine() const { return dot_token.line; }
            int getCol() const { return dot_token.column_start; }
        };

        struct MethodCallExpr : Expr
        {
            ExprPtr object;
            std::string method_name;
            Token dot_token;
            Token method_token;
            std::vector<ExprPtr> arguments;
            Token lparen_token;
            Token rparen_token;
            MethodCallExpr(ExprPtr obj, Token dot, Token method_tok, Token lparen, std::vector<ExprPtr> args, Token rparen)
                : object(std::move(obj)), method_name(method_tok.lexeme), dot_token(std::move(dot)), method_token(std::move(method_tok)), lparen_token(std::move(lparen)), arguments(std::move(args)), rparen_token(std::move(rparen)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitMethodCallExpr(this); }
            int getLine() const { return dot_token.line; }
            int getCol() const { return dot_token.column_start; }
        };

        // --- Statement nodes ---
        struct PrintStmt : Stmt
        {
            Token keyword;
            std::vector<ExprPtr> expressions;
            PrintStmt(Token kw, std::vector<ExprPtr> exprs) : keyword(std::move(kw)), expressions(std::move(exprs)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitPrintStmt(this); }
            int getLine() const { return keyword.line; }
            int getCol() const { return keyword.column_start; }
        };

        struct ExpressionStmt : Stmt
        {
            ExprPtr expression;
            Token first_token; // Thêm trường này nếu muốn lưu vị trí
            ExpressionStmt(ExprPtr expr, Token tok = Token()) : expression(std::move(expr)), first_token(tok) {}
            void accept(StmtVisitor *visitor) override { visitor->visitExpressionStmt(this); }
            int getLine() const { return first_token.line; }
            int getCol() const { return first_token.column_start; }
        };

        struct VarDeclStmt : Stmt
        {
            Token keyword;
            Token name;
            std::optional<TypeNodePtr> declared_type;
            ExprPtr initializer;

            VarDeclStmt(Token kw, Token n, std::optional<TypeNodePtr> type, ExprPtr init)
                : keyword(std::move(kw)), name(std::move(n)), declared_type(std::move(type)), initializer(std::move(init)) {}

            void accept(StmtVisitor *visitor) override { visitor->visitVarDeclStmt(this); }
            int getLine() const { return keyword.line; }
            int getCol() const { return keyword.column_start; }
        };

        struct WhileStmt : Stmt
        {
            Token keyword_while;
            ExprPtr condition;
            StmtPtr body;
            WhileStmt(Token kw, ExprPtr cond, StmtPtr b) : keyword_while(std::move(kw)), condition(std::move(cond)), body(std::move(b)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitWhileStmt(this); }
            int getLine() const { return keyword_while.line; }
            int getCol() const { return keyword_while.column_start; }
        };

        struct ReturnStmt : Stmt
        {
            Token keyword_return;
            ExprPtr value;
            ReturnStmt(Token kw, ExprPtr val) : keyword_return(std::move(kw)), value(std::move(val)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitReturnStmt(this); }
            int getLine() const { return keyword_return.line; }
            int getCol() const { return keyword_return.column_start; }
        };

        struct BreakStmt : Stmt
        {
            Token keyword;
            BreakStmt(Token kw) : keyword(std::move(kw)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitBreakStmt(this); }
            int getLine() const { return keyword.line; }
            int getCol() const { return keyword.column_start; }
        };
        struct ContinueStmt : Stmt
        {
            Token keyword;
            ContinueStmt(Token kw) : keyword(std::move(kw)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitContinueStmt(this); }
            int getLine() const { return keyword.line; }
            int getCol() const { return keyword.column_start; }
        };

        struct CaseClause
        {
            std::optional<ExprPtr> case_value;
            bool is_default;
            StmtList statements;
            Token keyword_token;
            Token colon_token;

            CaseClause(ExprPtr val, StmtList stmts_list, Token kw_tok, Token colon_tok)
                : case_value(std::move(val)), is_default(false),
                  statements(std::move(stmts_list)),
                  keyword_token(std::move(kw_tok)),
                  colon_token(std::move(colon_tok))
            {
            }

            CaseClause(StmtList stmts_list, Token kw_tok, Token colon_tok)
                : case_value(std::nullopt), is_default(true),
                  statements(std::move(stmts_list)),
                  keyword_token(std::move(kw_tok)),
                  colon_token(std::move(colon_tok))
            {
            }
        };
        struct SwitchStmt : Stmt
        {
            Token keyword_switch;
            ExprPtr expression_to_switch_on;
            std::vector<CaseClause> cases;
            Token opening_brace;
            SwitchStmt(Token kw, ExprPtr expr, std::vector<CaseClause> cls, Token brace) : keyword_switch(std::move(kw)), expression_to_switch_on(std::move(expr)), cases(std::move(cls)), opening_brace(std::move(brace)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitSwitchStmt(this); }
        };
        struct DeleteStmt : Stmt
        {
            Token keyword_delete;
            ExprPtr expression_to_delete;
            DeleteStmt(Token kw, ExprPtr expr) : keyword_delete(std::move(kw)), expression_to_delete(std::move(expr)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitDeleteStmt(this); }
        };
        struct ThrowStmt : Stmt
        {
            Token keyword;
            ExprPtr expression;
            ThrowStmt(Token kw, ExprPtr expr) : keyword(std::move(kw)), expression(std::move(expr)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitThrowStmt(this); }
        };

        struct CatchClauseNode
        {
            Token keyword_catch;
            std::optional<Token> exception_variable;
            std::unique_ptr<BlockStmt> body;
            CatchClauseNode(Token kw, Token var_name, std::unique_ptr<BlockStmt> b) : keyword_catch(std::move(kw)), exception_variable(std::move(var_name)), body(std::move(b)) {}
            CatchClauseNode(Token kw, std::unique_ptr<BlockStmt> b) : keyword_catch(std::move(kw)), exception_variable(std::nullopt), body(std::move(b)) {}
        };
        struct TryStmt : Stmt
        {
            Token keyword_try;
            std::unique_ptr<BlockStmt> try_block;
            std::vector<CatchClauseNode> catch_clauses;
            std::optional<std::unique_ptr<BlockStmt>> finally_block;
            std::optional<Token> keyword_finally;
            TryStmt(Token kw_try, std::unique_ptr<BlockStmt> try_b, std::vector<CatchClauseNode> catches, std::optional<std::unique_ptr<BlockStmt>> finally_b = std::nullopt, std::optional<Token> kw_finally_opt = std::nullopt) : keyword_try(std::move(kw_try)), try_block(std::move(try_b)), catch_clauses(std::move(catches)), finally_block(std::move(finally_b)), keyword_finally(kw_finally_opt) {}
            void accept(StmtVisitor *visitor) override { visitor->visitTryStmt(this); }
        };
        struct ImportStmt : Stmt // <--- Thêm node mới
        {
            Token import_kw;
            std::vector<Token> names; // rỗng nếu chỉ import module
            Token from_kw;            // type == FROM_KW nếu có, else type == END_OF_FILE
            Token module_name;
            Token semicolon;
            ImportStmt(Token import_kw, std::vector<Token> names, Token from_kw, Token module_name, Token semicolon)
                : import_kw(std::move(import_kw)), names(std::move(names)), from_kw(std::move(from_kw)), module_name(std::move(module_name)), semicolon(std::move(semicolon)) {}
            // Đơn giản hóa: names rỗng và from_kw.type == END_OF_FILE nếu chỉ import module
            void accept(StmtVisitor *visitor) override { visitor->visitImportStmt(this); }
        };
        
        struct ExportStmt : Stmt
        {
            Token export_kw;
            StmtPtr declaration; // Function, variable, or other declaration to export
            ExportStmt(Token export_kw, StmtPtr decl)
                : export_kw(std::move(export_kw)), declaration(std::move(decl)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitExportStmt(this); }
        };

        // --- MapEntryNode definition for MapLiteralExpr ---
        struct MapEntryNode
        {
            ExprPtr key;
            Token colon_token;
            ExprPtr value;
            MapEntryNode(ExprPtr k, Token colon, ExprPtr v)
                : key(std::move(k)), colon_token(std::move(colon)), value(std::move(v)) {}
        };

        // --- Definitions for all AST nodes (add after forward declarations) ---

        struct BlockStmt : Stmt
        {
            StmtList statements;
            Token opening_brace;
            BlockStmt(StmtList stmts, Token brace) : statements(std::move(stmts)), opening_brace(std::move(brace)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitBlockStmt(this); }
        };

        struct IfStmt : Stmt
        {
            Token keyword_if;
            ExprPtr condition;
            StmtPtr then_branch;
            StmtPtr else_branch;
            IfStmt(Token kw, ExprPtr cond, StmtPtr then_b, StmtPtr else_b)
                : keyword_if(std::move(kw)), condition(std::move(cond)), then_branch(std::move(then_b)), else_branch(std::move(else_b)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitIfStmt(this); }
            int getLine() const { return keyword_if.line; }
            int getCol() const { return keyword_if.column_start; }
        };

        struct UninitLiteralExpr : Expr
        {
            Token keyword;
            UninitLiteralExpr(Token kw) : keyword(std::move(kw)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitUninitLiteralExpr(this); }
        };

        struct NewExpr : Expr
        {
            Token keyword_new;
            ExprPtr class_constructor_call;
            NewExpr(Token kw, ExprPtr call) : keyword_new(std::move(kw)), class_constructor_call(std::move(call)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitNewExpr(this); }
        };

        struct ThisExpr : Expr
        {
            Token keyword_this;
            ThisExpr(Token kw) : keyword_this(std::move(kw)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitThisExpr(this); }
        };

        struct GroupingExpr : Expr
        {
            ExprPtr expression;
            GroupingExpr(ExprPtr expr) : expression(std::move(expr)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitGroupingExpr(this); }
        };

        struct ArrayLiteralExpr : Expr
        {
            Token l_bracket;
            std::vector<ExprPtr> elements;
            Token r_bracket;
            ArrayLiteralExpr(Token l, std::vector<ExprPtr> elems, Token r)
                : l_bracket(std::move(l)), elements(std::move(elems)), r_bracket(std::move(r)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitArrayLiteralExpr(this); }
            int getLine() const { return l_bracket.line; }
            int getCol() const { return l_bracket.column_start; }
        };

        struct MapLiteralExpr : Expr
        {
            Token l_brace;
            std::vector<MapEntryNode> entries;
            Token r_brace;
            MapLiteralExpr(Token l, std::vector<MapEntryNode> ents, Token r)
                : l_brace(std::move(l)), entries(std::move(ents)), r_brace(std::move(r)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitMapLiteralExpr(this); }
        };

        struct SubscriptExpr : Expr
        {
            ExprPtr object;
            Token l_bracket_token;
            ExprPtr index;
            Token r_bracket_token;
            SubscriptExpr(ExprPtr obj, Token l_br, ExprPtr idx, Token r_br)
                : object(std::move(obj)), l_bracket_token(std::move(l_br)), index(std::move(idx)), r_bracket_token(std::move(r_br)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitSubscriptExpr(this); }
        };

        struct FuncParamNode
        {
            Token name;
            std::optional<TypeNodePtr> type;
            bool is_static; // true nếu có 'vas' keyword
            
            FuncParamNode(Token n, std::optional<TypeNodePtr> t, bool static_ = false) 
                : name(std::move(n)), type(std::move(t)), is_static(static_) {}
        };

        struct FunctionDeclStmt : Stmt
        {
            Token keyword_func;
            Token name;
            std::vector<FuncParamNode> params;
            std::optional<TypeNodePtr> return_type;
            std::unique_ptr<BlockStmt> body;
            FunctionDeclStmt(Token kw, Token n, std::vector<FuncParamNode> p, std::optional<TypeNodePtr> ret, std::unique_ptr<BlockStmt> b)
                : keyword_func(std::move(kw)), name(std::move(n)), params(std::move(p)), return_type(std::move(ret)), body(std::move(b)) {}
            void accept(StmtVisitor *visitor) override { visitor->visitFunctionDeclStmt(this); }
            int getLine() const { return keyword_func.line; }
            int getCol() const { return keyword_func.column_start; }
        };

        struct FunctionExpr : Expr {
            Token keyword_func;
            std::vector<FuncParamNode> params;
            std::optional<TypeNodePtr> return_type;
            std::unique_ptr<BlockStmt> body;
            FunctionExpr(Token kw, std::vector<FuncParamNode> p, std::optional<TypeNodePtr> ret, std::unique_ptr<BlockStmt> b)
                : keyword_func(std::move(kw)), params(std::move(p)), return_type(std::move(ret)), body(std::move(b)) {}
            std::any accept(ExprVisitor *visitor) override { return visitor->visitFunctionExpr(this); }
            int getLine() const { return keyword_func.line; }
            int getCol() const { return keyword_func.column_start; }
        };

    } // namespace AST
} // namespace Linh
#endif // LINH_AST_NODE_HPP
       // NOTE: 'uninit' is a primitive type in Tinh Linh. It is not 'null' and always exists in memory.