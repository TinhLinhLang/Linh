// --- LinhC/Parsing/Parser/ParseExpression.cpp ---
#include "Parser.hpp" // Header chính của Parser
#include <iostream>   // Cho std::cout (debug)
#include <string>     // Cho std::string
#include <vector>     // Cho std::vector trong finish_call

// Nếu bạn có hàm debug_token_info và muốn sử dụng ở đây,
// bạn có thể include tệp chứa nó hoặc định nghĩa lại ở đây.

namespace Linh
{
    // --- Expression Parsing Methods ---

    AST::ExprPtr Parser::expression()
    {
        // Start from the lowest precedence: assignment
        return assignment();
    }

    // --- Bitwise precedence: | ^ & << >> ---
    // Correct precedence order:
    // logical_or (lowest)
    //   -> logical_and
    //     -> bitwise_or (|)
    //       -> bitwise_xor (^)
    //         -> bitwise_and (&)
    //           -> bitwise_shift (<<, >>)
    //             -> comparison
    //               -> term (+, -)
    //                 -> factor (*, /, %, #)
    //                   -> exponentiation (**)
    //                     -> unary

    AST::ExprPtr Parser::assignment()
    {
        AST::ExprPtr expr = logical_or(); // Parse vế trái tiềm năng

        // Kiểm tra các toán tử gán
        if (match({TokenType::ASSIGN, TokenType::PLUS_ASSIGN, TokenType::MINUS_ASSIGN,
                   TokenType::STAR_ASSIGN, TokenType::SLASH_ASSIGN, TokenType::PERCENT_ASSIGN,
                   TokenType::HASH_ASSIGN}))
        {
            Token equals_op_token = previous(); // Token toán tử gán
            AST::ExprPtr value = assignment();  // Parse vế phải (đệ quy, cho phép a = b = c)

            // Kiểm tra xem vế trái có phải là một lvalue hợp lệ không
            if (AST::IdentifierExpr *identifier_lvalue = dynamic_cast<AST::IdentifierExpr *>(expr.get()))
            {
                // --- Allow assignment to any identifier (semantic will check declaration) ---
                if (equals_op_token.type == TokenType::ASSIGN)
                { // Gán đơn giản '='
                    return std::make_unique<AST::AssignmentExpr>(identifier_lvalue->name, std::move(value));
                }
                else
                { // Gán phức hợp (+=, -=, ...)
                    // Chuyển đổi thành: name = name op value
                    TokenType binary_op_type;
                    std::string binary_op_lexeme;
                    switch (equals_op_token.type)
                    {
                    case TokenType::PLUS_ASSIGN:
                        binary_op_type = TokenType::PLUS;
                        binary_op_lexeme = "+";
                        break;
                    case TokenType::MINUS_ASSIGN:
                        binary_op_type = TokenType::MINUS;
                        binary_op_lexeme = "-";
                        break;
                    case TokenType::STAR_ASSIGN:
                        binary_op_type = TokenType::STAR;
                        binary_op_lexeme = "*";
                        break;
                    case TokenType::SLASH_ASSIGN:
                        binary_op_type = TokenType::SLASH;
                        binary_op_lexeme = "/";
                        break;
                    case TokenType::PERCENT_ASSIGN:
                        binary_op_type = TokenType::PERCENT;
                        binary_op_lexeme = "%";
                        break;
                    case TokenType::HASH_ASSIGN:
                        binary_op_type = TokenType::HASH;
                        binary_op_lexeme = "#";
                        break;
                    default:
                        // Trường hợp này không nên xảy ra nếu match() hoạt động đúng
                        throw error(equals_op_token, "Toán tử gán phức hợp không hợp lệ (lỗi nội bộ).");
                    }
                    // Tạo token cho toán tử binary tương ứng
                    Token actual_binary_op_token(binary_op_type, binary_op_lexeme, std::monostate{}, equals_op_token.line, equals_op_token.column_start);
                    // Vế trái của binary operation là chính Identifier đó
                    auto left_operand_for_binary = std::make_unique<AST::IdentifierExpr>(identifier_lvalue->name);
                    // Tạo biểu thức binary: name op value
                    auto compound_rhs_expr = std::make_unique<AST::BinaryExpr>(std::move(left_operand_for_binary), actual_binary_op_token, std::move(value));
                    // Tạo biểu thức gán: name = (name op value)
                    return std::make_unique<AST::AssignmentExpr>(identifier_lvalue->name, std::move(compound_rhs_expr));
                }
            }
            // Nếu expr không phải IdentifierExpr (ví dụ: 10 = x), thì đó là lỗi
            throw error(equals_op_token, "Vế trái của phép gán không hợp lệ. Phải là một biến.");
        }
        return expr; // Nếu không có toán tử gán, trả về biểu thức đã parse (vế trái)
    }

    AST::ExprPtr Parser::logical_or()
    {
        // logical_and ( ( '||' | 'or' ) logical_and )*
        AST::ExprPtr expr = logical_and();
        while (match({TokenType::OR_LOGIC, TokenType::OR_KW}))
        {
            Token op = previous();
            AST::ExprPtr right = logical_and();
            expr = std::make_unique<AST::LogicalExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    AST::ExprPtr Parser::logical_and()
    {
        // equality ( ( '&&' | 'and' ) equality )*
        AST::ExprPtr expr = equality();
        while (match({TokenType::AND_LOGIC, TokenType::AND_KW}))
        {
            Token op = previous();
            AST::ExprPtr right = equality();
            expr = std::make_unique<AST::LogicalExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    AST::ExprPtr Parser::equality()
    {
        // comparison ( ( '==' | '!=' ) comparison )*
        AST::ExprPtr expr = comparison();
        while (match({TokenType::EQ_EQ, TokenType::NOT_EQ}))
        {
            Token op = previous();
            AST::ExprPtr right = comparison();
            expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    AST::ExprPtr Parser::bitwise_or()
    {
        AST::ExprPtr expr = bitwise_xor();
        while (match({TokenType::PIPE}))
        {
            Token op = previous();
            AST::ExprPtr right = bitwise_xor();
            expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }
    AST::ExprPtr Parser::bitwise_xor()
    {
        AST::ExprPtr expr = bitwise_and();
        while (match({TokenType::CARET}))
        {
            Token op = previous();
            AST::ExprPtr right = bitwise_and();
            expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }
    AST::ExprPtr Parser::bitwise_and()
    {
        AST::ExprPtr expr = bitwise_shift();
        while (match({TokenType::AMP}))
        {
            Token op = previous();
            AST::ExprPtr right = bitwise_shift();
            expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }
    AST::ExprPtr Parser::bitwise_shift()
    {
        AST::ExprPtr expr = comparison();
        while (match({TokenType::LT_LT, TokenType::GT_GT}))
        {
            Token op = previous();
            AST::ExprPtr right = comparison();
            expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    AST::ExprPtr Parser::comparison()
    {
        // term ( ( '>' | '>=' | '<' | '<=' | 'is' ) term )*
        AST::ExprPtr expr = term();
        while (match({TokenType::GT, TokenType::GT_EQ, TokenType::LT, TokenType::LT_EQ, TokenType::IS_KW}))
        {
            Token op = previous();
            AST::ExprPtr right = term();
            expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    AST::ExprPtr Parser::term()
    {
        // factor ( ( '-' | '+' ) factor )*
        AST::ExprPtr expr = factor();
        while (match({TokenType::MINUS, TokenType::PLUS}))
        {
            Token op = previous();
            AST::ExprPtr right = factor();
            expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    AST::ExprPtr Parser::factor()
    {
        // exponentiation ( ( '/' | '*' | '%' | '#' ) exponentiation )*
        AST::ExprPtr expr = exponentiation();
        while (match({TokenType::SLASH, TokenType::STAR, TokenType::PERCENT, TokenType::HASH}))
        {
            Token op = previous();
            AST::ExprPtr right = exponentiation();
            expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    AST::ExprPtr Parser::exponentiation()
    {
        // unary ( '**' unary )?  -- Để right-associative, cần đệ quy bên phải
        // HOẶC unary ( '**' exponentiation )?
        // Hiện tại: unary ('**' unary) -- sẽ là left-associative nếu dùng vòng lặp
        // -> Sửa để hỗ trợ right-associativity cho '**'
        AST::ExprPtr expr = unary();
        if (match({TokenType::STAR_STAR}))
        {
            Token op = previous();
            AST::ExprPtr right = exponentiation();
            expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    AST::ExprPtr Parser::unary()
    {
        // ( '!' | '-' | '++' | '--' | 'not' ) unary
        // | NEW_KW call_or_member_access
        // | call_or_member_access
        if (match({TokenType::NOT, TokenType::MINUS, TokenType::PLUS_PLUS, TokenType::MINUS_MINUS, TokenType::NOT_KW, TokenType::TILDE}))
        {
            Token op = previous();        // Toán tử prefix
            AST::ExprPtr right = unary(); // Parse toán hạng
            return std::make_unique<AST::UnaryExpr>(op, std::move(right));
        }
        if (match({TokenType::NEW_KW}))
        {
            Token keyword_new = previous();
            AST::ExprPtr class_constructor_expr = call_or_member_access();
            if (dynamic_cast<AST::IdentifierExpr *>(class_constructor_expr.get()) &&
                !dynamic_cast<AST::CallExpr *>(class_constructor_expr.get()))
            {
                Token dummy_rparen(TokenType::RPAREN, ")", std::monostate{}, keyword_new.line, keyword_new.column_start + 1);
                class_constructor_expr = std::unique_ptr<AST::Expr>(new AST::CallExpr(std::move(class_constructor_expr), dummy_rparen, std::vector<AST::ExprPtr>{}));
            }
            else if (!dynamic_cast<AST::CallExpr *>(class_constructor_expr.get()))
            {
                throw error(previous(), "Mong đợi một lời gọi constructor (ví dụ: MyClass() hoặc MyClass) sau 'new'.");
            }
            return std::unique_ptr<AST::Expr>(new AST::NewExpr(keyword_new, std::move(class_constructor_expr)));
        }
        return call_or_member_access(); // Nếu không phải unary prefix hoặc new
    }

    AST::ExprPtr Parser::call_or_member_access()
    {
        // primary ( '(' arguments? ')' | '++' | '--' | '[' expression ']' | '.' IDENTIFIER ( '(' arguments? ')' )? )*
        AST::ExprPtr expr = primary(); // Parse phần cơ bản nhất của biểu thức

        while (true)
        { // Lặp để xử lý chuỗi các lời gọi hàm, truy cập postfix, etc.
            if (match({TokenType::LPAREN}))
            { // Lời gọi hàm: expr(...)
                expr = finish_call(std::move(expr));
            }
            else if (match({TokenType::PLUS_PLUS, TokenType::MINUS_MINUS}))
            { // Toán tử postfix: expr++ / expr--
                Token postfix_operator_token = previous();
                expr = std::make_unique<AST::PostfixExpr>(std::move(expr), postfix_operator_token);
            }
            else if (match({TokenType::LBRACKET})) // MỚI: Truy cập phần tử mảng: expr[...]
            {
                Token l_bracket_token = previous();
                AST::ExprPtr index_expr = expression();
                Token r_bracket_token = consume(TokenType::RBRACKET, "Thiếu ']' sau chỉ số truy cập.");
                expr = std::unique_ptr<AST::Expr>(new AST::SubscriptExpr(std::move(expr), l_bracket_token, std::move(index_expr), r_bracket_token));
            }
            else if (match({TokenType::DOT})) // Truy cập thành viên hoặc gọi method: expr.ident hoặc expr.method(...)
            {
                Token dot_token = previous();
#ifdef _DEBUG
                std::cerr << "[DEBUG] Found DOT token, next token: " << peek().lexeme << " (type: " << static_cast<int>(peek().type) << ")" << std::endl;
#endif
                // Accept IDENTIFIER or DELETE_KW as property/method name
                Token property_token;
                if (check(TokenType::IDENTIFIER) || check(TokenType::DELETE_KW)) {
                    property_token = advance();
#ifdef _DEBUG
                    std::cerr << "[DEBUG] MemberExpr created with property: " << property_token.lexeme << std::endl;
#endif
                } else {
                    throw error(peek(), "Thiếu tên thuộc tính sau dấu '.'");
                }
                // Luôn tạo MemberExpr trước
                expr = std::make_unique<AST::MemberExpr>(std::move(expr), dot_token, property_token);
                
                // Nếu sau đó là dấu '(', thì đây là lời gọi hàm: expr.method(...)
                if (match({TokenType::LPAREN}))
                {
                    // Sử dụng finish_call để tạo CallExpr với callee là MemberExpr vừa tạo
                    expr = finish_call(std::move(expr));
                }
            }
            else
            {
                break; // Không có toán tử call/postfix/subscript/member nào nữa
            }
        }
        return expr;
    }

    AST::ExprPtr Parser::finish_call(AST::ExprPtr callee)
    {
        // Hàm này được gọi sau khi LPAREN đã được consume
        std::vector<AST::ExprPtr> arguments;
        if (!check(TokenType::RPAREN))
        { // Nếu không phải ')' ngay, tức là có đối số
            do
            {
                // Kiểm tra giới hạn số lượng đối số (ví dụ 255)
                if (arguments.size() >= 255)
                {
                    error(peek(), "Không thể có nhiều hơn 255 đối số cho một hàm.");
                    // Mặc dù đã báo lỗi, vẫn cố gắng parse để giảm lỗi cascade, nhưng AST có thể không đúng.
                }
                arguments.push_back(expression()); // Parse từng biểu thức đối số
            } while (match({TokenType::COMMA})); // Lặp lại nếu có dấu phẩy
        }
        Token paren_token = consume(TokenType::RPAREN, "Thiếu ')' sau danh sách đối số của hàm.");
        return std::make_unique<AST::CallExpr>(std::move(callee), paren_token, std::move(arguments));
    }

    AST::ExprPtr Parser::primary()
    {
        // Literal (số, chuỗi, true, false, uninit), this, Identifier, ( expression )
        // Sau này có thể thêm: array literal `[]`, map literal `{}`
        // std::cout << "PARSER_EXPR_PRIMARY: primary() called. Peek: " << debug_token_info(peek()) << std::endl;

        // --- Allow str, int, float, bool, uint as function calls if followed by '(' ---
        if (check(TokenType::STR_KW) || check(TokenType::INT_KW) || check(TokenType::FLOAT_KW) ||
            check(TokenType::BOOL_KW) || check(TokenType::UINT_KW))
        {
            // If next token is LPAREN, treat as identifier for function call
            if (check_next(TokenType::LPAREN))
            {
                Token type_token = advance();
                Token id_token(TokenType::IDENTIFIER, type_token.lexeme, type_token.lexeme, type_token.line, type_token.column_start);
                return std::make_unique<AST::IdentifierExpr>(id_token);
            }
            // Otherwise, treat as type keyword (handled in type parser)
        }

        if (match({TokenType::FALSE_KW}))
            return std::make_unique<AST::LiteralExpr>(false);
        if (match({TokenType::TRUE_KW}))
            return std::make_unique<AST::LiteralExpr>(true);
        if (match({TokenType::SOL_KW}))
            return std::unique_ptr<AST::Expr>(new AST::UninitLiteralExpr(previous())); // previous() là SOL_KW
        if (match({TokenType::THIS_KW}))
            return std::unique_ptr<AST::Expr>(new AST::ThisExpr(previous())); // previous() là THIS_KW

        if (match({TokenType::INT, TokenType::UINT, TokenType::FLOAT_NUM, TokenType::STR}))
        {
            // Kiểm tra nếu tiếp theo là INTERP_START thì parse chuỗi nội suy
            if (check(TokenType::INTERP_START))
                return parse_interpolated_string(previous());
            // Đảm bảo truyền đúng variant cho từng loại literal
            const Token &tok = previous();
            if (tok.type == TokenType::UINT)
            {
                uint64_t val = 0;
                if (std::holds_alternative<uint64_t>(tok.literal))
                {
                    val = std::get<uint64_t>(tok.literal);
                }
                else if (std::holds_alternative<int64_t>(tok.literal))
                {
                    val = static_cast<uint64_t>(std::get<int64_t>(tok.literal));
                }
                return std::make_unique<AST::LiteralExpr>(val, tok);
            }
            if (tok.type == TokenType::INT)
            {
                int64_t val = 0;
                if (std::holds_alternative<int64_t>(tok.literal))
                {
                    val = std::get<int64_t>(tok.literal);
                }
                else if (std::holds_alternative<uint64_t>(tok.literal))
                {
                    val = static_cast<int64_t>(std::get<uint64_t>(tok.literal));
                }
                return std::make_unique<AST::LiteralExpr>(val, tok);
            }
            if (tok.type == TokenType::FLOAT_NUM)
            {
                double val = 0.0;
                if (std::holds_alternative<double>(tok.literal))
                {
                    val = std::get<double>(tok.literal);
                }
                return std::make_unique<AST::LiteralExpr>(val, tok);
            }
            if (tok.type == TokenType::STR)
            {
                std::string val;
                if (std::holds_alternative<std::string>(tok.literal))
                {
                    val = std::get<std::string>(tok.literal);
                }
                return std::make_unique<AST::LiteralExpr>(val, tok);
            }
            // Fallback: truyền nguyên như cũ
            return std::make_unique<AST::LiteralExpr>(tok.literal, tok);
        }

        // --- Sửa tại đây: cho phép type(...) là biểu thức ---
        if (match({TokenType::TYPE_KW}))
        {
            // Đối xử như một identifier để cho phép type(a) là CallExpr("type", ...)
            Token type_token = previous();
            return std::make_unique<AST::IdentifierExpr>(type_token);
        }

        if (match({TokenType::IDENTIFIER}))
        {
            return std::make_unique<AST::IdentifierExpr>(previous());
        }

        if (match({TokenType::LPAREN}))
        { // Biểu thức nhóm: ( expression )
            AST::ExprPtr expr_in_group = expression();
            consume(TokenType::RPAREN, "Thiếu ')' sau biểu thức trong dấu ngoặc.");
            return std::unique_ptr<AST::Expr>(new AST::GroupingExpr(std::move(expr_in_group)));
        }

        if (match({TokenType::FUNC_KW})) {
            return parse_function_expression();
        }

        // Thêm hỗ trợ cho array literal `[]`
        if (peek().type == TokenType::LBRACKET)
        {
            return parse_array_literal();
        }

        // Thêm hỗ trợ cho map literal `{}`
        if (peek().type == TokenType::LBRACE)
        {
            bool is_likely_map = false;
            if (check_next(TokenType::RBRACE))
            {
                is_likely_map = true;
            }
            else if (m_current + 2 < m_tokens.size())
            {
                is_likely_map = true;
            }

            if (is_likely_map)
            {
                return parse_map_literal();
            }
        }

        // Nếu không khớp với bất kỳ primary nào, ném lỗi
        throw error(peek(), "Không mong đợi token '" + peek().lexeme + "', cần một biểu thức (primary).");
    }

    AST::ExprPtr Parser::parse_array_literal()
    {
        Token l_bracket = consume(TokenType::LBRACKET, "Mong đợi '[' để bắt đầu array literal.");
        std::vector<AST::ExprPtr> elements;

        if (!check(TokenType::RBRACKET))
        {
            do
            {
                if (peek().type == TokenType::RBRACKET)
                    break;
                elements.push_back(expression());
            } while (match({TokenType::COMMA})); // Lặp lại nếu có dấu phẩy
        }

        Token r_bracket = consume(TokenType::RBRACKET, "Thiếu ']' để kết thúc array literal.");
        // Sửa: trả về ExprPtr
        return std::unique_ptr<AST::Expr>(new AST::ArrayLiteralExpr(l_bracket, std::move(elements), r_bracket));
    }

    AST::ExprPtr Parser::parse_map_literal()
    {
        Token l_brace = consume(TokenType::LBRACE, "Mong đợi '{' để bắt đầu map literal.");
        std::vector<AST::MapEntryNode> entries;

        if (!check(TokenType::RBRACE))
        {
            do
            {
                if (peek().type == TokenType::RBRACE)
                    break;
                AST::ExprPtr key_expr = expression();
                Token colon_token = consume(TokenType::COLON, "Thiếu ':' giữa key và value trong map entry.");
                // --- Sửa ở đây: nếu thiếu value thì tự động chèn UninitLiteralExpr ---
                if (check(TokenType::COMMA) || check(TokenType::RBRACE))
                {
                    Token uninit_token(TokenType::SOL_KW, "sol", std::monostate{}, colon_token.line, colon_token.column_start + 1);
                    AST::ExprPtr value_expr = std::unique_ptr<AST::Expr>(new AST::UninitLiteralExpr(uninit_token));
                    entries.emplace_back(std::move(key_expr), colon_token, std::move(value_expr));
                }
                else
                {
                    AST::ExprPtr value_expr = expression();
                    entries.emplace_back(std::move(key_expr), colon_token, std::move(value_expr));
                }
                // Nếu có dấu phẩy, kiểm tra tiếp theo không phải là RBRACE (không cho phép trailing comma)
                if (check(TokenType::COMMA))
                {
                    advance(); // consume comma
                    if (check(TokenType::RBRACE))
                    {
                        throw error(peek(), "Trailing comma in map literal is not allowed.");
                    }
                }
                else
                {
                    break;
                }
            } while (true);
        }

        Token r_brace = consume(TokenType::RBRACE, "Thiếu '}' để kết thúc map literal.");
        // Sửa: trả về ExprPtr
        return std::unique_ptr<AST::Expr>(new AST::MapLiteralExpr(l_brace, std::move(entries), r_brace));
    }

    AST::ExprPtr Parser::parse_interpolated_string(const Token &first_str_token)
    {
        std::vector<std::variant<std::string, AST::ExprPtr>> parts;
        // Thêm phần đầu tiên
        parts.push_back(std::get<std::string>(first_str_token.literal));
        while (match({TokenType::INTERP_START}))
        {
            // Luôn parse như một biểu thức đầy đủ, không chỉ IDENTIFIER
            AST::ExprPtr expr = expression();
            consume(TokenType::INTERP_END, "Thiếu '}' sau biểu thức nội suy trong chuỗi.");
            parts.push_back(std::move(expr));
            
            // Nếu tiếp tục có STR thì nối tiếp
            if (check(TokenType::STR))
            {
                Token str_token = advance();
                parts.push_back(std::get<std::string>(str_token.literal));
            }
        }
        return std::make_unique<AST::InterpolatedStringExpr>(std::move(parts));
    }

    AST::ExprPtr Parser::parse_function_expression() {
        Token func_kw = previous();
        // Parse parameter list
        consume(TokenType::LPAREN, "Thiếu '(' sau 'func' cho anonymous function.");
        std::vector<AST::FuncParamNode> params;
        if (!check(TokenType::RPAREN)) {
            do {
                bool is_static = false;
                std::optional<AST::TypeNodePtr> param_type = std::nullopt;
                Token param_name;
                if (match({TokenType::VAS_KW})) is_static = true;
                if (match({TokenType::IDENTIFIER})) {
                    param_name = previous();
                } else {
                    throw error(peek(), "Thiếu tên tham số trong anonymous function.");
                }
                if (match({TokenType::COLON})) {
                    param_type = parse_type();
                }
                params.emplace_back(param_name, std::move(param_type), is_static);
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RPAREN, "Thiếu ')' sau danh sách tham số anonymous function.");
        std::optional<AST::TypeNodePtr> return_type = std::nullopt;
        if (match({TokenType::COLON})) {
            return_type = parse_type();
        }
        std::unique_ptr<AST::BlockStmt> body = block();
        return std::make_unique<AST::FunctionExpr>(func_kw, std::move(params), std::move(return_type), std::move(body));
    }

} // namespace Linh