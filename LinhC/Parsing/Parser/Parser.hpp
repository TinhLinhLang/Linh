#ifndef LINH_PARSER_HPP
#define LINH_PARSER_HPP

// Parser for the Tinh Linh Language (Linh Interpreter)
// Part of the Tinh Linh language ecosystem:
//   - Linh (this project): Interpreter implementation
//   - Tinh: AOT compiler with runtime (similar to Go)
//   - Lithium: Language standard specification

#include <vector>
#include <stdexcept>
#include <string>
#include <optional>
#include "../Lexer/Lexer.hpp"
#include "../AST/ASTNode.hpp" // Quan trọng

namespace Linh
{

    class Parser
    {
    public:
        explicit Parser(const std::vector<Token> &tokens);
        AST::StmtList parse();
        bool had_error() const { return m_had_error; }

        class ParseError : public std::runtime_error
        {
        public:
            Token token_info;
            ParseError(const Token &token, const std::string &message)
                : std::runtime_error(message), token_info(token) {}
        };

    private:
        // --- Utility Methods ---
        Token previous() const;
        Token peek() const;
        Token peek_next() const;
        Token advance();
        bool is_at_end() const;
        bool check(TokenType type) const;
        bool check_next(TokenType type) const;
        bool match(const std::vector<TokenType> &types);
        Token consume(TokenType type, const std::string &error_message);
        ParseError error(const Token &token, const std::string &message);
        void synchronize();

        // --- Type Parsing Methods ---
        AST::TypeNodePtr parse_type();
        AST::TypeNodePtr parse_primary_type();                                   // Xử lý kiểu cơ bản, ID, ( Type )
        AST::TypeNodePtr parse_array_suffix_type(AST::TypeNodePtr element_type); // Xử lý `[]` sau một kiểu
        AST::TypeNodePtr parse_union_type();                                     // Xử lý `<...>`
        AST::TypeNodePtr parse_map_type();                                       // Xử lý `map<...>`
        AST::ExprPtr create_zero_value_initializer_for_type(const AST::TypeNode *type_node, const Token &reference_token_for_pos);

        // --- Expression Parsing ---
        AST::ExprPtr expression();
        AST::ExprPtr assignment();
        AST::ExprPtr logical_or();
        AST::ExprPtr logical_and();
        AST::ExprPtr equality();
        AST::ExprPtr comparison();
        AST::ExprPtr term();
        AST::ExprPtr factor();
        AST::ExprPtr exponentiation();
        AST::ExprPtr unary();
        AST::ExprPtr call_or_member_access();
        AST::ExprPtr primary();
        AST::ExprPtr finish_call(AST::ExprPtr callee);
        AST::ExprPtr parse_array_literal();
        AST::ExprPtr parse_map_literal();
        AST::ExprPtr parse_interpolated_string(const Token &first_str_token);
        AST::ExprPtr parse_function_expression();

        // --- Bitwise precedence ---
        AST::ExprPtr bitwise_or();
        AST::ExprPtr bitwise_xor();
        AST::ExprPtr bitwise_and();
        AST::ExprPtr bitwise_shift();

        // --- Declaration and Statement Parsing ---
        AST::StmtPtr declaration();
        AST::StmtPtr var_declaration(Token keyword_token);
        AST::StmtPtr function_declaration(Token func_keyword);
        AST::StmtPtr import_statement(); // <--- Thêm dòng này
        AST::StmtPtr export_statement(); // export statement
        AST::StmtPtr statement();
        AST::StmtPtr print_statement();
        AST::StmtPtr if_statement();
        AST::StmtPtr while_statement();
        AST::StmtPtr for_statement();
        AST::StmtPtr switch_statement();
        AST::StmtPtr return_statement();
        AST::StmtPtr break_statement();
        AST::StmtPtr continue_statement();
        AST::StmtPtr delete_statement();
        AST::StmtPtr throw_statement();
        AST::StmtPtr try_statement();
        std::unique_ptr<AST::BlockStmt> block();
        AST::StmtPtr expression_statement();

        const std::vector<Token> &m_tokens;
        size_t m_current = 0;
        bool m_had_error = false;
    };

} // namespace Linh
#endif // LINH_PARSER_HPP