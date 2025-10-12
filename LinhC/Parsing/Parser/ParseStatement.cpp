// --- LinhC/Parsing/Parser/ParseStatement.cpp ---
#include "Parser.hpp" // Header chính của Parser
#include <iostream>   // Cho std::cout (debug)
#include <string>     // Cho std::string
#include <sstream>    // Thêm dòng này để dùng std::ostringstream

// --- Thêm hàm debug_token_info tại đây ---
#ifdef _DEBUG
static std::string debug_token_info(const Linh::Token &t)
{
    std::ostringstream oss;
    oss << "'" << t.lexeme << "' (Type: " << Linh::token_type_to_string(t.type)
        << ", Line: " << t.line
        << ", Col: " << t.column_start << ")";
    return oss.str();
}
#endif

// Nếu bạn có hàm debug_token_info và muốn sử dụng ở đây,
// bạn có thể include tệp chứa nó hoặc định nghĩa lại ở đây.

namespace Linh
{
    // --- Statement Parsing Methods ---

    AST::StmtPtr Parser::statement()
    {
        // std::cout << "PARSER_STMT: statement() called. Peek: " << debug_token_info(peek()) << std::endl;
        if (match({TokenType::PRINT_KW}))
            return print_statement();

        // Xử lý block statement: '{' ... '}'
        // Quan trọng: Hàm block() kỳ vọng rằng LBRACE đã được consume bởi statement() hoặc một hàm gọi khác.
        if (peek().type == TokenType::LBRACE)
        {
            // std::cout << "PARSER_STMT: Found LBRACE as start of statement. Consuming it." << std::endl;
            consume(TokenType::LBRACE, "Mong đợi '{' để bắt đầu khối lệnh.");
            // previous() bây giờ là LBRACE, được truyền ngầm cho block()
            return std::unique_ptr<AST::Stmt>(block().release());
        }

        if (match({TokenType::IF_KW}))
            return if_statement();
        if (match({TokenType::WHILE_KW}))
            return while_statement();
        if (match({TokenType::FOR_KW}))
            return for_statement();
        if (match({TokenType::SWITCH_KW}))
            return switch_statement();
        if (match({TokenType::DELETE_KW}))
            return delete_statement();
        if (match({TokenType::THROW_KW}))
            return throw_statement();
        if (match({TokenType::TRY_KW}))
            return try_statement();
        if (match({TokenType::RETURN_KW}))
            return return_statement();
        if (match({TokenType::BREAK_KW}))
            return break_statement();
        if (match({TokenType::SKIP_KW}))
            return continue_statement(); // SKIP_KW là từ khóa cho 'continue'

        // std::cout << "PARSER_STMT: Không match statement nào, gọi expression_statement(). Peek: " << debug_token_info(peek()) << std::endl;
        return expression_statement();
    }

    AST::StmtPtr Parser::print_statement()
    {
        Token keyword = previous(); // previous() là PRINT_KW
        consume(TokenType::LPAREN, "Thiếu '(' sau 'print'.");
        
        std::vector<AST::ExprPtr> expressions;
        if (!check(TokenType::RPAREN)) {
            do {
                expressions.push_back(expression());
            } while (match({TokenType::COMMA}));
        }
        
        consume(TokenType::RPAREN, "Thiếu ')' sau đối số của print.");
        if (check(TokenType::SEMICOLON))
            consume(TokenType::SEMICOLON, "");
        return std::make_unique<AST::PrintStmt>(std::move(keyword), std::move(expressions));
    }

    std::unique_ptr<AST::BlockStmt> Parser::block()
    {
        // Hàm này được gọi SAU KHI LBRACE đã được consume.
        // previous() sẽ là LBRACE đó.
        Token opening_brace_for_this_block = previous();

        AST::StmtList statements_in_block;
        while (!check(TokenType::RBRACE) && !is_at_end())
        {
            // std::cout << "PARSER_BLOCK: Vòng lặp trong block, parsing declaration. Peek: " << debug_token_info(peek()) << std::endl;
            statements_in_block.push_back(declaration()); // Trong một block, có thể có khai báo hoặc câu lệnh
        }
        // Nếu đã hết file và token cuối cùng đã parse là RBRACE thì kết thúc block mà không báo lỗi
        if (is_at_end() && !m_tokens.empty() && m_tokens[m_tokens.size()-2].type == TokenType::RBRACE) {
            // Đã parse xong block, return luôn
            return std::make_unique<AST::BlockStmt>(std::move(statements_in_block), opening_brace_for_this_block);
        }
        // Nếu đã hết file mà token cuối cùng là RBRACE thì advance để consume và kết thúc block
        if (is_at_end() && !m_tokens.empty() && m_tokens.back().type == TokenType::RBRACE) {
            m_current = m_tokens.size() - 1; // Đặt con trỏ tới RBRACE cuối cùng
            advance(); // Consume RBRACE
        } else {
            consume(TokenType::RBRACE, "Thiếu '}' để kết thúc khối lệnh.");
        }

        // std::cout << "PARSER_BLOCK: block() finished. Consumed RBRACE. Returning BlockStmt for brace at line "
        //           << opening_brace_for_this_block.line << std::endl;
        return std::make_unique<AST::BlockStmt>(std::move(statements_in_block), opening_brace_for_this_block);
    }

    AST::StmtPtr Parser::if_statement()
    {
        Token keyword_if = previous(); // IF_KW
        consume(TokenType::LPAREN, "Thiếu '(' sau 'if'.");
        AST::ExprPtr condition = expression();
        consume(TokenType::RPAREN, "Thiếu ')' sau điều kiện của if.");

        // std::cout << "PARSER_IF: Parsing then_branch. Peek: " << debug_token_info(peek()) << std::endl;
        AST::StmtPtr then_branch = statement(); // then_branch có thể là một block hoặc một câu lệnh đơn

        AST::StmtPtr else_branch = nullptr;
        if (peek().type == TokenType::ELSE_KW)
        { // Không dùng match() vì else không bắt buộc phải advance ngay
            // std::cout << "PARSER_IF: Found ELSE_KW. Consuming it. Lexeme: " << peek().lexeme << std::endl;
            consume(TokenType::ELSE_KW, "Mong đợi 'else'."); // Bây giờ mới consume
            // std::cout << "PARSER_IF: Parsing else_branch. Peek: " << debug_token_info(peek()) << std::endl;
            else_branch = statement(); // else_branch cũng có thể là block hoặc câu lệnh đơn
        }
        // std::cout << "PARSER_IF: Returning IfStmt." << std::endl;
        return std::unique_ptr<AST::Stmt>(new AST::IfStmt(std::move(keyword_if), std::move(condition), std::move(then_branch), std::move(else_branch)));
    }

    AST::StmtPtr Parser::while_statement()
    {
        Token keyword_while = previous(); // WHILE_KW
        consume(TokenType::LPAREN, "Thiếu '(' sau 'while'.");
        AST::ExprPtr condition = expression();
        consume(TokenType::RPAREN, "Thiếu ')' sau điều kiện của while.");
        AST::StmtPtr body = statement(); // Body có thể là block hoặc câu lệnh đơn
        return std::make_unique<AST::WhileStmt>(std::move(keyword_while), std::move(condition), std::move(body));
    }

    AST::StmtPtr Parser::for_statement()
    {
        Token keyword_for = previous(); // FOR_KW
        consume(TokenType::LPAREN, "Thiếu '(' sau 'for'.");

        AST::StmtPtr initializer_stmt = nullptr;
        if (check(TokenType::SEMICOLON))
        { // Khởi tạo rỗng
            consume(TokenType::SEMICOLON, "Mong đợi ';' sau initializer rỗng của for.");
        }
        else if (peek().type == TokenType::VAR_KW || peek().type == TokenType::VAS_KW || peek().type == TokenType::CONST_KW)
        {
            // Parse as a full statement (which will consume its own semicolon)
            initializer_stmt = declaration();
        }
        else
        { // Parse as an expression statement (which will consume its own semicolon)
            initializer_stmt = expression_statement();
        }

        AST::ExprPtr condition_expr = nullptr;
        if (!check(TokenType::SEMICOLON))
        { // Nếu không phải là ';' ngay -> có điều kiện
            condition_expr = expression();
        }
        consume(TokenType::SEMICOLON, "Thiếu ';' sau điều kiện for.");
        if (!condition_expr)
        {
            Token true_token(TokenType::TRUE_KW, "true", true, keyword_for.line, keyword_for.column_start);
            condition_expr = std::make_unique<AST::LiteralExpr>(true_token.literal);
        }

        AST::ExprPtr increment_expr = nullptr;
        if (!check(TokenType::RPAREN))
        { // Nếu không phải ')' ngay -> có bước lặp
            increment_expr = expression();
        }
        consume(TokenType::RPAREN, "Thiếu ')' sau mệnh đề for.");

        AST::StmtPtr body_stmt = statement(); // Body có thể là block hoặc câu lệnh đơn

        // Desugar for loop:
        // {
        //   initializer;
        //   while (condition) {
        //     body;
        //     increment;
        //   }
        // }
        if (increment_expr)
        {
            auto increment_as_stmt = std::unique_ptr<AST::Stmt>(new AST::ExpressionStmt(std::move(increment_expr)));
            if (AST::BlockStmt *body_as_block = dynamic_cast<AST::BlockStmt *>(body_stmt.get()))
            {
                body_as_block->statements.push_back(std::move(increment_as_stmt));
            }
            else
            {
                AST::StmtList new_body_stmts;
                new_body_stmts.push_back(std::move(body_stmt));
                new_body_stmts.push_back(std::move(increment_as_stmt));
                Token dummy_brace(TokenType::LBRACE, "{", std::monostate{}, keyword_for.line, keyword_for.column_start);
                body_stmt = std::unique_ptr<AST::Stmt>(new AST::BlockStmt(std::move(new_body_stmts), dummy_brace));
            }
        }

        Token dummy_while_kw(TokenType::WHILE_KW, "while", std::monostate{}, keyword_for.line, keyword_for.column_start);
        auto while_loop_stmt = std::make_unique<AST::WhileStmt>(dummy_while_kw, std::move(condition_expr), std::move(body_stmt));

        if (initializer_stmt)
        {
            AST::StmtList final_block_stmts;
            final_block_stmts.push_back(std::move(initializer_stmt));
            final_block_stmts.push_back(std::move(while_loop_stmt));
            Token dummy_brace(TokenType::LBRACE, "{", std::monostate{}, keyword_for.line, keyword_for.column_start);
            return std::unique_ptr<AST::Stmt>(new AST::BlockStmt(std::move(final_block_stmts), dummy_brace));
        }
        else
        {
            return while_loop_stmt;
        }
    }

    AST::StmtPtr Parser::switch_statement()
    {
        Token keyword_switch = previous(); // SWITCH_KW
        consume(TokenType::LPAREN, "Thiếu '(' sau 'switch'.");
        AST::ExprPtr condition = expression();
        consume(TokenType::RPAREN, "Thiếu ')' sau biểu thức switch.");
        Token opening_brace = consume(TokenType::LBRACE, "Thiếu '{' trước thân switch.");

        std::vector<AST::CaseClause> case_clauses_list;
        bool default_clause_found = false;

        while (!check(TokenType::RBRACE) && !is_at_end())
        {
            Token case_or_default_keyword_token = peek(); // CASE_KW hoặc DEFAULT_KW
            if (match({TokenType::CASE_KW}))
            {
                // Token case_kw đã được consume bởi match()
                if (default_clause_found)
                    throw error(previous(), "'case' không được xuất hiện sau 'default'.");

                AST::ExprPtr value = expression(); // Giá trị của case
                Token colon_token = consume(TokenType::COLON, "Thiếu ':' sau giá trị case.");

                AST::StmtList statements_for_case;
                // Đọc các câu lệnh cho đến khi gặp case, default, hoặc '}' tiếp theo
                while (!check(TokenType::RBRACE) && !check(TokenType::CASE_KW) && !check(TokenType::DEFAULT_KW) && !is_at_end())
                {
                    statements_for_case.push_back(declaration());
                }
                case_clauses_list.emplace_back(std::move(value), std::move(statements_for_case), case_or_default_keyword_token, colon_token);
            }
            else if (match({TokenType::DEFAULT_KW}))
            {
                // Token default_kw đã được consume
                if (default_clause_found)
                    throw error(previous(), "Chỉ được phép có một mệnh đề 'default' trong switch.");
                default_clause_found = true;

                Token colon_token = consume(TokenType::COLON, "Thiếu ':' sau 'default'.");
                AST::StmtList statements_for_default;
                while (!check(TokenType::RBRACE) && !check(TokenType::CASE_KW) && !is_at_end())
                {
                    // Không được có CASE_KW sau DEFAULT_KW (trừ khi default là cuối cùng và không có break)
                    // Logic fall-through sẽ được xử lý bởi interpreter/compiler sau này.
                    // Parser chỉ đảm bảo cú pháp.
                    statements_for_default.push_back(declaration());
                }
                case_clauses_list.emplace_back(std::move(statements_for_default), case_or_default_keyword_token, colon_token);
            }
            else
            {
                throw error(peek(), "Chỉ mong đợi 'case', 'default', hoặc '}' trong khối switch.");
            }
        }
        consume(TokenType::RBRACE, "Thiếu '}' để kết thúc switch.");
        return std::make_unique<AST::SwitchStmt>(keyword_switch, std::move(condition), std::move(case_clauses_list), opening_brace);
    }

    AST::StmtPtr Parser::delete_statement()
    {
        Token keyword_del = previous();             // DELETE_KW
        AST::ExprPtr expr_to_delete = expression(); // Parse biểu thức cần delete
        if (check(TokenType::SEMICOLON))
            consume(TokenType::SEMICOLON, "");
        return std::make_unique<AST::DeleteStmt>(keyword_del, std::move(expr_to_delete));
    }

    AST::StmtPtr Parser::throw_statement()
    {
        Token keyword_throw_stmt = previous();      // THROW_KW
        AST::ExprPtr exception_expr = expression(); // Parse biểu thức ném ra
        if (check(TokenType::SEMICOLON))
            consume(TokenType::SEMICOLON, "");
        return std::make_unique<AST::ThrowStmt>(std::move(keyword_throw_stmt), std::move(exception_expr));
    }

    AST::StmtPtr Parser::try_statement()
    {
        Token keyword_try = previous(); // TRY_KW
        consume(TokenType::LBRACE, "Thiếu khối lệnh '{...}' sau 'try'.");
        std::unique_ptr<AST::BlockStmt> try_block_ptr = block(); // Parse khối try

        std::vector<AST::CatchClauseNode> catch_clauses_list;
        // Parse một hoặc nhiều mệnh đề catch
        while (peek().type == TokenType::CATCH_KW)
        {
            Token keyword_catch = consume(TokenType::CATCH_KW, "Mong đợi 'catch'.");
            std::optional<Token> exception_var_name = std::nullopt;
            if (match({TokenType::LPAREN}))
            { // Có khai báo biến exception `catch (e)`
                if (check(TokenType::IDENTIFIER))
                {
                    exception_var_name = consume(TokenType::IDENTIFIER, "Thiếu tên biến exception trong catch.");
                }
                else
                {
                    // Nếu có ( mà không có IDENTIFIER -> lỗi
                    throw error(peek(), "Thiếu tên biến exception sau '(' trong catch.");
                }
                consume(TokenType::RPAREN, "Thiếu ')' sau khai báo biến exception.");
            }
            // Parse khối catch
            consume(TokenType::LBRACE, "Thiếu khối lệnh '{...}' sau 'catch'.");
            std::unique_ptr<AST::BlockStmt> catch_body_ptr = block();

            if (exception_var_name.has_value())
            {
                catch_clauses_list.emplace_back(keyword_catch, exception_var_name.value(), std::move(catch_body_ptr));
            }
            else
            { // catch không có biến
                catch_clauses_list.emplace_back(keyword_catch, std::move(catch_body_ptr));
            }
        }

        // Parse mệnh đề finally (tùy chọn)
        std::optional<std::unique_ptr<AST::BlockStmt>> finally_block_opt = std::nullopt;
        std::optional<Token> keyword_finally_opt = std::nullopt;
        if (peek().type == TokenType::FINALLY_KW)
        {
            keyword_finally_opt = consume(TokenType::FINALLY_KW, "Mong đợi 'finally'.");
            consume(TokenType::LBRACE, "Thiếu khối lệnh '{...}' sau 'finally'.");
            finally_block_opt = block();
        }

        // Phải có ít nhất một catch hoặc một finally
        if (catch_clauses_list.empty() && !finally_block_opt.has_value())
        {
            throw error(keyword_try, "'try' phải có ít nhất một mệnh đề 'catch' hoặc 'finally'.");
        }
        return std::make_unique<AST::TryStmt>(
            keyword_try, std::move(try_block_ptr),
            std::move(catch_clauses_list),
            std::move(finally_block_opt),
            keyword_finally_opt);
    }

    AST::StmtPtr Parser::return_statement()
    {
        Token keyword = previous(); // RETURN_KW
        AST::ExprPtr value = nullptr;
        if (!check(TokenType::SEMICOLON))
        { // Nếu không phải là ';' ngay, thì có giá trị trả về
            value = expression();
        }
        if (check(TokenType::SEMICOLON))
            consume(TokenType::SEMICOLON, "");
        return std::make_unique<AST::ReturnStmt>(std::move(keyword), std::move(value));
    }

    AST::StmtPtr Parser::break_statement()
    {
        Token keyword = previous(); // BREAK_KW
        if (check(TokenType::SEMICOLON))
            consume(TokenType::SEMICOLON, "");
        return std::make_unique<AST::BreakStmt>(std::move(keyword));
    }

    AST::StmtPtr Parser::continue_statement()
    {                               // 'skip' keyword
        Token keyword = previous(); // SKIP_KW
        if (check(TokenType::SEMICOLON))
            consume(TokenType::SEMICOLON, "");
        return std::make_unique<AST::ContinueStmt>(std::move(keyword));
    }

    AST::StmtPtr Parser::expression_statement()
    {
        // std::cout << "PARSER_EXPR_STMT: expression_statement() called. Peek: " << debug_token_info(peek()) << std::endl;
        AST::ExprPtr expr = expression();
        // Cho phép ; tùy chọn: nếu có thì consume, nếu không thì vẫn hợp lệ
        if (check(TokenType::SEMICOLON))
        {
            consume(TokenType::SEMICOLON, "");
        }
        // Không cần else báo lỗi nữa, chỉ cần return luôn
        return std::make_unique<AST::ExpressionStmt>(std::move(expr));
    }

} // namespace Linh