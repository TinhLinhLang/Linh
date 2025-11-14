// --- LinhC/Parsing/Parser/ParseDeclaration.cpp ---
#include "Parser.hpp"
#include <iostream>
#include <string>

namespace Linh
{
    AST::ExprPtr Parser::create_zero_value_initializer_for_type(const AST::TypeNode *type_node, const Token &reference_token_for_pos)
    {
        if (!type_node)
        {
            Token uninit_token(TokenType::SOL_KW, "nothing", std::monostate{}, reference_token_for_pos.line, reference_token_for_pos.column_start);
            return std::unique_ptr<AST::Expr>(new AST::UninitLiteralExpr(uninit_token));
        }

        if (const AST::BaseTypeNode *base_type = dynamic_cast<const AST::BaseTypeNode *>(type_node))
        {
            switch (base_type->type_keyword_token.type)
            {
            case TokenType::INT_KW:
            {
                Token zero_int_token(TokenType::INT, "0", static_cast<int64_t>(0), reference_token_for_pos.line, reference_token_for_pos.column_start);
                return std::unique_ptr<AST::Expr>(new AST::LiteralExpr(zero_int_token.literal));
            }
            case TokenType::UINT_KW:
            {
                Token zero_uint_token(TokenType::UINT, "0", static_cast<uint64_t>(0), reference_token_for_pos.line, reference_token_for_pos.column_start);
                return std::unique_ptr<AST::Expr>(new AST::LiteralExpr(zero_uint_token.literal));
            }
            case TokenType::FLOAT_KW:
            {
                Token zero_float_token(TokenType::FLOAT_NUM, "0.0", 0.0, reference_token_for_pos.line, reference_token_for_pos.column_start);
                return std::unique_ptr<AST::Expr>(new AST::LiteralExpr(zero_float_token.literal));
            }
            case TokenType::STR_KW:
            {
                Token empty_str_token(TokenType::STR, "\"\"", std::string(""), reference_token_for_pos.line, reference_token_for_pos.column_start);
                return std::unique_ptr<AST::Expr>(new AST::LiteralExpr(empty_str_token.literal));
            }
            case TokenType::BOOL_KW:
            {
                Token false_token(TokenType::FALSE_KW, "false", false, reference_token_for_pos.line, reference_token_for_pos.column_start);
                return std::unique_ptr<AST::Expr>(new AST::LiteralExpr(false_token.literal));
            }
            case TokenType::SOL_KW:
            {
                Token uninit_token(TokenType::SOL_KW, "nothing", std::monostate{}, reference_token_for_pos.line, reference_token_for_pos.column_start);
                return std::unique_ptr<AST::Expr>(new AST::UninitLiteralExpr(uninit_token));
            }
            case TokenType::VOID_KW:
                throw error(base_type->type_keyword_token, "Cannot create zero value for type 'void'.");
            case TokenType::ANY_KW:
            {
                Token uninit_any_token(TokenType::SOL_KW, "nothing", std::monostate{}, reference_token_for_pos.line, reference_token_for_pos.column_start);
                return std::unique_ptr<AST::Expr>(new AST::UninitLiteralExpr(uninit_any_token));
            }
            default:
            { // Bao gồm IDENTIFIER
                std::cerr << "WARNING [Line: 0, Col: 0] ParserWarning: Cannot determine zero value for base type '" << base_type->type_keyword_token.lexeme << "' at parse time. Defaulting to nothing." << std::endl;
                Token uninit_default_token(TokenType::SOL_KW, "nothing", std::monostate{}, reference_token_for_pos.line, reference_token_for_pos.column_start);
                return std::unique_ptr<AST::Expr>(new AST::UninitLiteralExpr(uninit_default_token));
            }
            }
        }
        else if (dynamic_cast<const AST::SizedIntegerTypeNode *>(type_node))
        {
            Token zero_int_token(TokenType::INT, "0", static_cast<int64_t>(0), reference_token_for_pos.line, reference_token_for_pos.column_start);
            return std::unique_ptr<AST::Expr>(new AST::LiteralExpr(zero_int_token.literal));
        }
        else if (dynamic_cast<const AST::SizedFloatTypeNode *>(type_node))
        {
            Token zero_float_token(TokenType::FLOAT_NUM, "0.0", 0.0, reference_token_for_pos.line, reference_token_for_pos.column_start);
            return std::unique_ptr<AST::Expr>(new AST::LiteralExpr(zero_float_token.literal));
        }
        else if (dynamic_cast<const AST::MapTypeNode *>(type_node))
        {
            Token l_brace(TokenType::LBRACE, "{", std::monostate{}, reference_token_for_pos.line, reference_token_for_pos.column_start);
            Token r_brace(TokenType::RBRACE, "}", std::monostate{}, reference_token_for_pos.line, reference_token_for_pos.column_start);
            return std::unique_ptr<AST::Expr>(new AST::MapLiteralExpr(l_brace, std::vector<AST::MapEntryNode>{}, r_brace));
        }
        else if (dynamic_cast<const AST::ArrayTypeNode *>(type_node))
        {
            Token l_bracket(TokenType::LBRACKET, "[", std::monostate{}, reference_token_for_pos.line, reference_token_for_pos.column_start);
            Token r_bracket(TokenType::RBRACKET, "]", std::monostate{}, reference_token_for_pos.line, reference_token_for_pos.column_start);
            return std::unique_ptr<AST::Expr>(new AST::ArrayLiteralExpr(l_bracket, std::vector<AST::ExprPtr>{}, r_bracket));
        }
        else if (const AST::UnionTypeNode *union_type = dynamic_cast<const AST::UnionTypeNode *>(type_node))
        {
            if (!union_type->types.empty() && union_type->types[0] != nullptr)
            {
                return create_zero_value_initializer_for_type(union_type->types[0].get(), reference_token_for_pos);
            }
            std::cerr << "WARNING [Line: 0, Col: 0] ParserWarning: Empty union or first type is null when creating zero value. Defaulting to uninit." << std::endl;
        }

        Token uninit_fallback_token(TokenType::SOL_KW, "nothing", std::monostate{}, reference_token_for_pos.line, reference_token_for_pos.column_start);
        return std::unique_ptr<AST::Expr>(new AST::UninitLiteralExpr(uninit_fallback_token));
    }

    AST::StmtPtr Parser::declaration()
    {
        if (match({TokenType::VAR_KW, TokenType::VAS_KW, TokenType::CONST_KW}))
        {
            return std::unique_ptr<AST::Stmt>(var_declaration(previous()).release());
        }
        if (match({TokenType::FUNC_KW}))
        {
            return std::unique_ptr<AST::Stmt>(function_declaration(previous()).release());
        }
        if (match({TokenType::IMPORT_KW})) // <--- Thêm dòng này
        {
            return std::unique_ptr<AST::Stmt>(import_statement().release());
        }
        if (match({TokenType::EXPORT_KW})) // export statement
        {
            return std::unique_ptr<AST::Stmt>(export_statement().release());
        }
        return statement();
    }

    AST::StmtPtr Parser::var_declaration(Token keyword_token)
    {
        Token name_token = consume(TokenType::IDENTIFIER, "Missing variable name after '" + keyword_token.lexeme + "'.");

        std::optional<AST::TypeNodePtr> declared_type_node_opt = std::nullopt;
        if (match({TokenType::COLON}))
        {
            declared_type_node_opt = parse_type();

            if (declared_type_node_opt.has_value() && declared_type_node_opt.value() != nullptr)
            {
                const AST::TypeNode *actual_type_node = declared_type_node_opt.value().get();
                if (const AST::BaseTypeNode *base_type = dynamic_cast<const AST::BaseTypeNode *>(actual_type_node))
                {
                    if (base_type->type_keyword_token.type == TokenType::VOID_KW)
                    {
                        throw error(base_type->type_keyword_token, "Cannot declare variable of type 'void'. 'void' is only allowed as a function return type.");
                    }
                    if (base_type->type_keyword_token.type == TokenType::ANY_KW)
                    {
                        throw error(base_type->type_keyword_token, "Cannot declare variable of type 'any' directly. 'any' should be used as a generic type in map/array or as a return/parameter type when needed.");
                    }
                }
            }
        }

        AST::ExprPtr initializer_expr = nullptr;
        if (match({TokenType::ASSIGN}))
        {
            initializer_expr = expression();
        }
        else
        {
            if (keyword_token.type == TokenType::VAR_KW)
            {
                if (declared_type_node_opt.has_value())
                {
                    initializer_expr = create_zero_value_initializer_for_type(declared_type_node_opt.value().get(), name_token);
                }
                else
                {
                    Token uninit_type_token(TokenType::SOL_KW, "nothing", std::monostate{}, name_token.line, name_token.column_start);
                    declared_type_node_opt = std::make_unique<AST::BaseTypeNode>(uninit_type_token);
                    Token uninit_val_token(TokenType::SOL_KW, "nothing", std::monostate{}, name_token.line, name_token.column_start);
                    initializer_expr = std::make_unique<AST::UninitLiteralExpr>(uninit_val_token);
                }
            }
            else if (keyword_token.type == TokenType::VAS_KW || keyword_token.type == TokenType::CONST_KW)
            {
                if (declared_type_node_opt.has_value())
                {
                    const AST::TypeNode *type_node_ptr = declared_type_node_opt.value().get();
                    if (const AST::BaseTypeNode *base_type = dynamic_cast<const AST::BaseTypeNode *>(type_node_ptr))
                    {
                        if (base_type->type_keyword_token.type == TokenType::SOL_KW)
                        {
                            throw error(base_type->type_keyword_token, "'" + keyword_token.lexeme + "' cannot be explicitly declared with type 'uninit'.");
                        }
                    }
                    initializer_expr = create_zero_value_initializer_for_type(type_node_ptr, name_token);
                }
                else
                {
                    throw error(name_token, "'" + keyword_token.lexeme + "' declaration must have an explicit initializer or an explicit type to infer a zero value.");
                }
            }
        }

        if ((keyword_token.type == TokenType::VAS_KW || keyword_token.type == TokenType::CONST_KW) &&
            !declared_type_node_opt.has_value() && initializer_expr)
        {
            if (AST::UninitLiteralExpr *uninit_init = dynamic_cast<AST::UninitLiteralExpr *>(initializer_expr.get()))
            {
                // Bỏ báo lỗi, cho phép vas/const = uninit mà không cần kiểu
                // Token error_token = uninit_init->keyword;
                // throw error(error_token, "Cannot infer fixed type for '" + keyword_token.lexeme + "' from 'uninit' value. Use 'var' or provide an explicit type (e.g., 'vas myVar: some_type = uninit;').");
            }
        }

        // Cho phép kết thúc khai báo biến bằng ';', '}' hoặc EOF
        if (check(TokenType::SEMICOLON))
        {
            consume(TokenType::SEMICOLON, "");
        }
        // Nếu là '}' hoặc EOF thì hợp lệ, không cần consume gì thêm
        else if (check(TokenType::RBRACE) || check(TokenType::END_OF_FILE))
        {
            // Hợp lệ, không cần consume gì thêm
        }
        // Nếu là token khác, cũng hợp lệ (bắt đầu statement mới), không báo lỗi
        // (Bỏ else báo lỗi ở đây)
        // Nếu là EOF, advance để parser không bị kẹt ở cuối file
        if (check(TokenType::END_OF_FILE) && !is_at_end())
        {
            advance();
        }

        return std::make_unique<AST::VarDeclStmt>(
            std::move(keyword_token),
            std::move(name_token),
            std::move(declared_type_node_opt),
            std::move(initializer_expr));
    }

    AST::StmtPtr Parser::function_declaration(Token func_keyword)
    {
        Token name_token = consume(TokenType::IDENTIFIER, "Missing function name after 'func'.");
        consume(TokenType::LPAREN, "Missing '(' after function name.");
        std::vector<AST::FuncParamNode> parameters_list;
        if (!check(TokenType::RPAREN))
        {
            do
            {
                if (parameters_list.size() >= 255)
                {
                    error(peek(), "Cannot have more than 255 parameters.");
                }
                // --- Xử lý var/vas/const trước tên tham số ---
                bool is_static = false;
                if (match({TokenType::VAR_KW, TokenType::VAS_KW, TokenType::CONST_KW}))
                {
                    Token keyword_token = previous();
                    if (keyword_token.type == TokenType::VAS_KW) {
                        is_static = true;
                    }
                    // var và const hiện tại chưa có ý nghĩa đặc biệt
                }
                Token param_name_token = consume(TokenType::IDENTIFIER, "Missing parameter name.");
                std::optional<AST::TypeNodePtr> param_type_node = std::nullopt;
                if (match({TokenType::COLON}))
                {
                    param_type_node = parse_type();
                    if (param_type_node.has_value() && param_type_node.value())
                    {
                        if (const AST::BaseTypeNode *base_param_type = dynamic_cast<const AST::BaseTypeNode *>(param_type_node.value().get()))
                        {
                            if (base_param_type->type_keyword_token.type == TokenType::VOID_KW)
                            {
                                throw error(base_param_type->type_keyword_token, "Function parameter cannot have type 'void'.");
                            }
                        }
                    }
                }
                parameters_list.emplace_back(param_name_token, std::move(param_type_node), is_static);
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RPAREN, "Missing ')' after parameter list.");

        std::optional<AST::TypeNodePtr> return_type_node = std::nullopt;
        if (match({TokenType::COLON}))
        {
            return_type_node = parse_type();
            if (return_type_node.has_value() && return_type_node.value())
            {
                if (const AST::BaseTypeNode *base_ret_type = dynamic_cast<const AST::BaseTypeNode *>(return_type_node.value().get()))
                {
                    // Cho phép func trả về uninit như một kiểu dữ liệu nguyên thủy
                    // if (base_ret_type->type_keyword_token.type == TokenType::SOL_KW)
                    // {
                    //     throw error(base_ret_type->type_keyword_token, "Function cannot have return type 'uninit' alone. Use 'void' if no value is returned, or a specific type.");
                    // }
                }
            }
        }

        consume(TokenType::LBRACE, "Missing '{' before function body.");
        std::unique_ptr<AST::BlockStmt> body_block_ptr = block();
        return std::unique_ptr<AST::Stmt>(new AST::FunctionDeclStmt(
            std::move(func_keyword),
            std::move(name_token),
            std::move(parameters_list),
            std::move(return_type_node),
            std::move(body_block_ptr)));
    }

    AST::StmtPtr Parser::import_statement()
    {
        Token import_kw = previous(); // IMPORT_KW
        std::vector<Token> names;
        Token from_kw(TokenType::END_OF_FILE, "", std::monostate{}, import_kw.line, import_kw.column_start); // default
        Token module_name(TokenType::END_OF_FILE, "", std::monostate{}, import_kw.line, import_kw.column_start);
        Token semicolon(TokenType::END_OF_FILE, "", std::monostate{}, import_kw.line, import_kw.column_start);

#ifdef _DEBUG
        std::cerr << "[DEBUG] Import statement - current token: " << peek().lexeme << " (type: " << static_cast<int>(peek().type) << ")" << std::endl;
#endif
        // Nếu tiếp theo là IDENTIFIER hoặc STRING, có thể là import name1, name2 from module; hoặc import module; hoặc import "path";
        if (check(TokenType::IDENTIFIER) || check(TokenType::STR))
        {
#ifdef _DEBUG
            std::cerr << "[DEBUG] Found IDENTIFIER after import: " << peek().lexeme << std::endl;
#endif
            Token first = advance();
            if (check(TokenType::COMMA))
            {
                // import name1, name2, ... from module;
                names.push_back(first);
                while (match({TokenType::COMMA}))
                {
                    Token name_token = consume(TokenType::IDENTIFIER, "Missing name in import statement.");
                    names.push_back(name_token);
                }
                from_kw = consume(TokenType::FROM_KW, "Missing 'from' in import statement.");
                // Parse module name (IDENTIFIER (DOT IDENTIFIER)*)
                Token mod_first = consume(TokenType::IDENTIFIER, "Missing module name in import statement.");
                std::string mod_name = mod_first.lexeme;
                while (check(TokenType::DOT))
                {
                    advance(); // consume DOT
                    Token next_part = consume(TokenType::IDENTIFIER, "Expected identifier after '.' in module name.");
                    mod_name += "." + next_part.lexeme;
                }
                module_name = Token(TokenType::IDENTIFIER, mod_name, std::monostate{}, mod_first.line, mod_first.column_start);
                semicolon = consume(TokenType::SEMICOLON, "Missing ';' after import statement.");
            }
            else if (check(TokenType::FROM_KW))
            {
                // import name from module;
                names.push_back(first);
                from_kw = consume(TokenType::FROM_KW, "Missing 'from' in import statement.");
                // Parse module name (IDENTIFIER (DOT IDENTIFIER)*)
                Token mod_first = consume(TokenType::IDENTIFIER, "Missing module name in import statement.");
                std::string mod_name = mod_first.lexeme;
                while (check(TokenType::DOT))
                {
                    advance(); // consume DOT
                    Token next_part = consume(TokenType::IDENTIFIER, "Expected identifier after '.' in module name.");
                    mod_name += "." + next_part.lexeme;
                }
                module_name = Token(TokenType::IDENTIFIER, mod_name, std::monostate{}, mod_first.line, mod_first.column_start);
                semicolon = consume(TokenType::SEMICOLON, "Missing ';' after import statement.");
            }
            else if (first.type == TokenType::STR || check(TokenType::DOT) || check(TokenType::SEMICOLON) || check(TokenType::END_OF_FILE)
                || check(TokenType::PRINT_KW)
                || check(TokenType::VAR_KW)
                || check(TokenType::VAS_KW)
                || check(TokenType::CONST_KW)
                || check(TokenType::FUNC_KW)
                || check(TokenType::IF_KW)
                || check(TokenType::WHILE_KW)
                || check(TokenType::FOR_KW)
                || check(TokenType::SWITCH_KW)
                || check(TokenType::RETURN_KW)
                || check(TokenType::TRY_KW)
                || check(TokenType::THROW_KW)
                || check(TokenType::DELETE_KW)
                || check(TokenType::BREAK_KW)
                || check(TokenType::SKIP_KW))
            {
                // import module; hoặc import module.with.dots; hoặc import "path";
#ifdef _DEBUG
                std::cerr << "[DEBUG] Entering import module branch" << std::endl;
#endif
                std::string mod_name;
                int mod_line = first.line;
                int mod_col = first.column_start;
                
                if (first.type == TokenType::STR)
                {
                    // import "path/to/module.li";
                    mod_name = std::get<std::string>(first.literal);
#ifdef _DEBUG
                    std::cerr << "[DEBUG] Processing import string path: " << mod_name << std::endl;
#endif
                }
                else
                {
                    // import module.with.dots;
                    mod_name = first.lexeme;
#ifdef _DEBUG
                    std::cerr << "[DEBUG] Processing import module: " << mod_name << std::endl;
#endif
                    while (check(TokenType::DOT))
                    {
                        advance(); // consume DOT
                        Token next_part = consume(TokenType::IDENTIFIER, "Expected identifier after '.' in module name.");
                        mod_name += "." + next_part.lexeme;
                    }
                }
                module_name = Token(TokenType::IDENTIFIER, mod_name, std::monostate{}, mod_line, mod_col);
#ifdef _DEBUG
                std::cerr << "[DEBUG] Final module name: " << mod_name << std::endl;
#endif
                // Cho phép không có dấu chấm phẩy trong REPL hoặc khi là EOF hoặc khi tiếp theo là statement khác
                if (check(TokenType::SEMICOLON))
                {
                    semicolon = consume(TokenType::SEMICOLON, "");
#ifdef _DEBUG
                    std::cerr << "[DEBUG] Found semicolon" << std::endl;
#endif
                }
                else if (check(TokenType::END_OF_FILE)
                    || check(TokenType::PRINT_KW)
                    || check(TokenType::VAR_KW)
                    || check(TokenType::VAS_KW)
                    || check(TokenType::CONST_KW)
                    || check(TokenType::FUNC_KW)
                    || check(TokenType::IF_KW)
                    || check(TokenType::WHILE_KW)
                    || check(TokenType::FOR_KW)
                    || check(TokenType::SWITCH_KW)
                    || check(TokenType::RETURN_KW)
                    || check(TokenType::TRY_KW)
                    || check(TokenType::THROW_KW)
                    || check(TokenType::DELETE_KW)
                    || check(TokenType::BREAK_KW)
                    || check(TokenType::SKIP_KW))
                {
                    // Không consume gì thêm, cho phép kết thúc import bằng EOF hoặc bắt đầu statement mới
#ifdef _DEBUG
                    std::cerr << "[DEBUG] Found EOF or next statement, no semicolon needed" << std::endl;
#endif
                }
                else
                {
#ifdef _DEBUG
                    std::cerr << "[DEBUG] Error: unexpected token after module name" << std::endl;
#endif
                    throw error(peek(), "Missing ';' after import statement.");
                }
            }
            else
            {
                throw error(peek(), "Invalid import statement syntax.");
            }
        }
        else
        {
            throw error(peek(), "Missing module name or import names in import statement.");
        }
        return std::make_unique<AST::ImportStmt>(import_kw, std::move(names), from_kw, module_name, semicolon);
    }

    AST::StmtPtr Parser::export_statement()
    {
        Token export_kw = previous(); // EXPORT_KW
        
        // Parse the declaration to export (function, variable, etc.)
        AST::StmtPtr declaration_to_export;
        
        if (match({TokenType::FUNC_KW}))
        {
            declaration_to_export = function_declaration(previous());
        }
        else if (match({TokenType::VAR_KW, TokenType::VAS_KW, TokenType::CONST_KW}))
        {
            declaration_to_export = var_declaration(previous());
        }
        else
        {
            throw error(peek(), "Expected function or variable declaration after 'export'.");
        }
        
        return std::make_unique<AST::ExportStmt>(export_kw, std::move(declaration_to_export));
    }

} // namespace Linh