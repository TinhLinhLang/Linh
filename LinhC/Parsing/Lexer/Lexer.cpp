#include "Lexer.hpp"
#include <iostream>
#include <utility>
#include <stdexcept>
#include <cctype>
#include <string>
#include <limits>

namespace Linh
{
    const std::unordered_map<std::string, TokenType> Lexer::s_keywords = {
        {"var", TokenType::VAR_KW}, {"vas", TokenType::VAS_KW}, {"const", TokenType::CONST_KW}, {"if", TokenType::IF_KW}, {"else", TokenType::ELSE_KW}, {"for", TokenType::FOR_KW}, {"while", TokenType::WHILE_KW}, {"func", TokenType::FUNC_KW}, {"return", TokenType::RETURN_KW}, {"true", TokenType::TRUE_KW}, {"false", TokenType::FALSE_KW}, {"int", TokenType::INT_KW}, {"uint", TokenType::UINT_KW}, {"str", TokenType::STR_KW}, {"bool", TokenType::BOOL_KW}, {"float", TokenType::FLOAT_KW}, {"map", TokenType::MAP_KW}, {"array", TokenType::ARRAY_KW}, {"void", TokenType::VOID_KW}, {"any", TokenType::ANY_KW}, {"print", TokenType::PRINT_KW}, {"break", TokenType::BREAK_KW}, {"continue", TokenType::CONTINUE_KW}, {"skip", TokenType::SKIP_KW}, {"switch", TokenType::SWITCH_KW}, {"case", TokenType::CASE_KW}, {"default", TokenType::DEFAULT_KW}, {"other", TokenType::OTHER_KW}, {"type", TokenType::TYPE_KW}, {"sol", TokenType::SOL_KW}, {"is", TokenType::IS_KW}, {"not", TokenType::NOT_KW}, {"and", TokenType::AND_KW}, {"or", TokenType::OR_KW}, {"do", TokenType::DO_KW}, {"new", TokenType::NEW_KW}, {"delete", TokenType::DELETE_KW}, {"this", TokenType::THIS_KW}, {"throw", TokenType::THROW_KW}, {"try", TokenType::TRY_KW}, {"catch", TokenType::CATCH_KW}, {"finally", TokenType::FINALLY_KW}, {"import", TokenType::IMPORT_KW}, {"from", TokenType::FROM_KW},
        {"byte", TokenType::BYTE_KW},
        {"bytearray", TokenType::BYTEARRAY_KW},
        {"id", TokenType::IDENTIFIER} // Thêm dòng này để id luôn là identifier (không phải keyword, nhưng nhận diện được)
    };

    Token::Token(TokenType type, std::string lexeme, LiteralValue literal, int line, int column_start)
        : type(type), lexeme(std::move(lexeme)), literal(std::move(literal)), line(line), column_start(column_start) {}

    std::string Token::to_string() const
    {
        std::string literal_str_val = "NONE";
        std::visit([&](auto &&arg)
                   {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) literal_str_val = "None";
            else if constexpr (std::is_same_v<T, int64_t>) literal_str_val = "int:" + std::to_string(arg);
            else if constexpr (std::is_same_v<T, double>) literal_str_val = "float:" + std::to_string(arg);
            else if constexpr (std::is_same_v<T, std::string>) literal_str_val = "str:'" + arg + "'";
            else if constexpr (std::is_same_v<T, bool>) literal_str_val = (arg ? "bool:true" : "bool:false"); }, literal);
        return "Token[Type:" + token_type_to_string(type) +
               ", Lexeme:'" + lexeme +
               "', Literal:" + literal_str_val +
               ", Line:" + std::to_string(line) +
               ", Col:" + std::to_string(column_start) + "]";
    }

    std::string token_type_to_string(TokenType type)
    {
        switch (type)
        {
        case TokenType::LPAREN:
            return "LPAREN";
        case TokenType::RPAREN:
            return "RPAREN";
        case TokenType::LBRACE:
            return "LBRACE";
        case TokenType::RBRACE:
            return "RBRACE";
        case TokenType::LBRACKET:
            return "LBRACKET";
        case TokenType::RBRACKET:
            return "RBRACKET";
        case TokenType::COMMA:
            return "COMMA";
        case TokenType::DOT:
            return "DOT";
        case TokenType::MINUS:
            return "MINUS";
        case TokenType::PLUS:
            return "PLUS";
        case TokenType::SEMICOLON:
            return "SEMICOLON";
        case TokenType::SLASH:
            return "SLASH";
        case TokenType::STAR:
            return "STAR";
        case TokenType::PERCENT:
            return "PERCENT";
        case TokenType::COLON:
            return "COLON";
        case TokenType::HASH:
            return "HASH";
        case TokenType::NOT:
            return "NOT";
        case TokenType::NOT_EQ:
            return "NOT_EQ";
        case TokenType::ASSIGN:
            return "ASSIGN";
        case TokenType::EQ_EQ:
            return "EQ_EQ";
        case TokenType::GT:
            return "GT";
        case TokenType::GT_EQ:
            return "GT_EQ";
        case TokenType::LT:
            return "LT";
        case TokenType::LT_EQ:
            return "LT_EQ";
        case TokenType::AND_LOGIC:
            return "AND_LOGIC";
        case TokenType::OR_LOGIC:
            return "OR_LOGIC";
        case TokenType::PLUS_PLUS:
            return "PLUS_PLUS";
        case TokenType::MINUS_MINUS:
            return "MINUS_MINUS";
        case TokenType::PLUS_ASSIGN:
            return "PLUS_ASSIGN";
        case TokenType::MINUS_ASSIGN:
            return "MINUS_ASSIGN";
        case TokenType::STAR_ASSIGN:
            return "STAR_ASSIGN";
        case TokenType::SLASH_ASSIGN:
            return "SLASH_ASSIGN";
        case TokenType::PERCENT_ASSIGN:
            return "PERCENT_ASSIGN";
        case TokenType::HASH_ASSIGN:
            return "HASH_ASSIGN";
        case TokenType::STAR_STAR:
            return "STAR_STAR";
        case TokenType::IDENTIFIER:
            return "IDENTIFIER";
        case TokenType::STR:
            return "STR";
        case TokenType::INT:
            return "INT";
        case TokenType::UINT:
            return "UINT";
        case TokenType::FLOAT_NUM:
            return "FLOAT_NUM";
        case TokenType::ARRAY_KW:
            return "ARRAY_KW";
        case TokenType::BOOL_KW:
            return "BOOL_KW";
        case TokenType::CONST_KW:
            return "CONST_KW";
        case TokenType::VAS_KW:
            return "VAS_KW";
        case TokenType::ELSE_KW:
            return "ELSE_KW";
        case TokenType::FALSE_KW:
            return "FALSE_KW";
        case TokenType::FLOAT_KW:
            return "FLOAT_KW";
        case TokenType::FOR_KW:
            return "FOR_KW";
        case TokenType::FUNC_KW:
            return "FUNC_KW";
        case TokenType::IF_KW:
            return "IF_KW";
        case TokenType::INPUT_KW:
            return "INPUT_KW";
        case TokenType::INT_KW:
            return "INT_KW";
        case TokenType::MAP_KW:
            return "MAP_KW";
        case TokenType::PRINT_KW:
            return "PRINT_KW";
        case TokenType::RETURN_KW:
            return "RETURN_KW";
        case TokenType::STR_KW:
            return "STR_KW";
        case TokenType::TRUE_KW:
            return "TRUE_KW";
        case TokenType::UINT_KW:
            return "UINT_KW";
        case TokenType::VAR_KW:
            return "VAR_KW";
        case TokenType::VOID_KW:
            return "VOID_KW";
        case TokenType::WHILE_KW:
            return "WHILE_KW";
        case TokenType::ANY_KW:
            return "ANY_KW";
        case TokenType::BREAK_KW:
            return "BREAK_KW";
        case TokenType::CONTINUE_KW:
            return "CONTINUE_KW";
        case TokenType::SKIP_KW:
            return "SKIP_KW";
        case TokenType::SWITCH_KW:
            return "SWITCH_KW";
        case TokenType::CASE_KW:
            return "CASE_KW";
        case TokenType::DEFAULT_KW:
            return "DEFAULT_KW";
        case TokenType::OTHER_KW:
            return "OTHER_KW";
        case TokenType::TYPE_KW:
            return "TYPE_KW";
        case TokenType::ID_KW:
            return "ID_KW";
        case TokenType::SOL_KW:
            return "SOL_KW";
        case TokenType::IS_KW:
            return "IS_KW";
        case TokenType::NOT_KW:
            return "NOT_KW";
        case TokenType::AND_KW:
            return "AND_KW";
        case TokenType::OR_KW:
            return "OR_KW";
        case TokenType::DO_KW:
            return "DO_KW";
        case TokenType::NEW_KW:
            return "NEW_KW";
        case TokenType::DELETE_KW:
            return "DELETE_KW";
        case TokenType::THIS_KW:
            return "THIS_KW";
        case TokenType::THROW_KW:
            return "THROW_KW";
        case TokenType::TRY_KW:
            return "TRY_KW";
        case TokenType::CATCH_KW:
            return "CATCH_KW";
        case TokenType::FINALLY_KW:
            return "FINALLY_KW";
        case TokenType::IMPORT_KW:
            return "IMPORT_KW";
        case TokenType::FROM_KW:
            return "FROM_KW";
        // case TokenType::CLASS_KW: return "CLASS_KW";
        case TokenType::ERROR:
            return "ERROR";
        case TokenType::END_OF_FILE:
            return "EOF";
        default:
            return "UNKNOWN_TOKEN_TYPE(" + std::to_string(static_cast<int>(type)) + ")";
        }
    }

    Lexer::Lexer(std::string source) : m_source(std::move(source)) {}
    bool Lexer::is_at_end() const { return m_current_pos >= m_source.length(); }
    char Lexer::advance()
    {
        if (!is_at_end())
            m_current_col_scan++;
        return m_source[m_current_pos++];
    }
    char Lexer::peek() const
    {
        if (is_at_end())
            return '\0';
        return m_source[m_current_pos];
    }
    char Lexer::peek_next() const
    {
        if (m_current_pos + 1 >= m_source.length())
            return '\0';
        return m_source[m_current_pos + 1];
    }
    bool Lexer::match(char expected)
    {
        if (is_at_end() || m_source[m_current_pos] != expected)
            return false;
        m_current_col_scan++;
        m_current_pos++;
        return true;
    }
    void Lexer::create_and_add_token(TokenType type, int line, int col_start)
    {
        create_and_add_token(type, std::monostate{}, line, col_start);
    }
    void Lexer::create_and_add_token(TokenType type, const LiteralValue &literal_val, int line, int col_start)
    {
        std::string text = m_source.substr(m_start_lexeme, m_current_pos - m_start_lexeme);
        m_tokens.emplace_back(type, std::move(text), literal_val, line, col_start);
    }
    void Lexer::handle_string_literal(char quote_char, int start_line, int start_col)
    {
        std::string value_str;
        while (peek() != quote_char && !is_at_end())
        {
            char current_char = peek();
            if (current_char == '\n')
            {
                m_current_line++;
                m_current_col_scan = 0;
            }
            // --- BẮT ĐẦU: HỖ TRỢ CHUỖI NỘI SUY ---
            if (current_char == '&' && peek_next() == '{')
            {
                if (!value_str.empty())
                {
                    create_and_add_token(TokenType::STR, value_str, start_line, start_col);
                    value_str.clear();
                }
                // Đặt lại m_start_lexeme để INTERP_START có lexeme đúng
                m_start_lexeme = m_current_pos;
                advance(); // consume '&'
                advance(); // consume '{'
                create_and_add_token(TokenType::INTERP_START, start_line, start_col);

                // Bỏ qua khoảng trắng nếu có
                while (isspace(peek()) && peek() != '\n' && !is_at_end())
                    advance();

                // Đánh dấu vị trí bắt đầu biểu thức nội suy
                m_start_lexeme = m_current_pos;
                int expr_start_line = m_current_line;
                int expr_start_col = m_current_col_scan;

                // Quét đến dấu đóng '}'
                int brace_count = 1;
                std::string expr_content;
                while (!is_at_end() && brace_count > 0)
                {
                    char ch = peek();
                    if (ch == '{')
                    {
                        brace_count++;
                    }
                    else if (ch == '}')
                    {
                        brace_count--;
                        if (brace_count == 0)
                            break;
                    }
                    expr_content += advance();
                }

                if (is_at_end())
                {
                    m_tokens.emplace_back(TokenType::ERROR, m_source.substr(m_start_lexeme, m_current_pos - m_start_lexeme), "Unterminated interpolation.", expr_start_line, expr_start_col);
                    return;
                }

                // Tokenize nội dung biểu thức nội suy
                if (!expr_content.empty())
                {
                    // Tạo một Lexer tạm thời để tokenize nội dung biểu thức
                    Lexer expr_lexer(expr_content);
                    auto expr_tokens = expr_lexer.scan_tokens();
                    
                    // Thêm các token vào danh sách chính (bỏ qua EOF token cuối)
                    for (size_t i = 0; i < expr_tokens.size() - 1; ++i)
                    {
                        auto& token = expr_tokens[i];
                        // Điều chỉnh vị trí token để phù hợp với vị trí trong chuỗi gốc
                        token.line = expr_start_line;
                        token.column_start = expr_start_col + token.column_start;
                        m_tokens.push_back(token);
                    }
                }

                // Đặt lại m_start_lexeme để INTERP_END có lexeme đúng
                m_start_lexeme = m_current_pos;
                advance(); // consume '}'
                create_and_add_token(TokenType::INTERP_END, m_current_line, m_current_col_scan);

                m_start_lexeme = m_current_pos;
                continue;
            }
            // --- KẾT THÚC: HỖ TRỢ CHUỖI NỘI SUY ---
            // Xử lý escape sequence
            if (current_char == '\\')
            {
                advance(); // consume '\\'
                char next = peek();
                switch (next)
                {
                case 'n':
                    value_str += '\n';
                    break;
                case 't':
                    value_str += '\t';
                    break;
                case 'r':
                    value_str += '\r';
                    break;
                case '\\':
                    value_str += '\\';
                    break;
                case '"':
                    value_str += '"';
                    break;
                case '\'':
                    value_str += '\'';
                    break;
                case '0':
                    value_str += '\0';
                    break;
                default:
                    value_str += next;
                    break;
                }
                advance(); // consume escaped char
            }
            else if (quote_char != '`' && current_char == '\\' && (peek_next() == quote_char || peek_next() == '\\'))
            {
                advance();
                value_str += advance();
            }
            else
            {
                value_str += advance();
            }
        }
        if (is_at_end())
        {
            m_tokens.emplace_back(TokenType::ERROR, m_source.substr(m_start_lexeme, m_current_pos - m_start_lexeme), "Unterminated string.", start_line, start_col);
            return;
        }
        advance();
        create_and_add_token(TokenType::STR, value_str, start_line, start_col);
    }
    void Lexer::handle_number_literal(int start_line, int start_col)
    {
        // Hỗ trợ literal hệ 16: 0x... hoặc 0X... (có thể thêm hậu tố u/U cho uint)
        if (m_source[m_start_lexeme] == '0' && (peek() == 'x' || peek() == 'X'))
        {
            advance(); // consume 'x' or 'X'
            int digits_start = m_current_pos;
            while (std::isxdigit(peek()))
                advance();
            int digits_end = m_current_pos;
            bool is_uint_hex = false;
            if (peek() == 'u' || peek() == 'U')
            {
                is_uint_hex = true;
                advance();
            }
            std::string hex_str = m_source.substr(digits_start, digits_end - digits_start);
            if (hex_str.empty())
            {
                m_tokens.emplace_back(TokenType::ERROR, "0x", "Invalid hexadecimal literal.", start_line, start_col);
                return;
            }
            try
            {
                unsigned long long v = std::stoull(hex_str, nullptr, 16);
                if (is_uint_hex)
                {
                    create_and_add_token(TokenType::UINT, static_cast<uint64_t>(v), start_line, start_col);
                }
                else
                {
                    if (v <= static_cast<unsigned long long>(std::numeric_limits<int64_t>::max()))
                        create_and_add_token(TokenType::INT, static_cast<int64_t>(v), start_line, start_col);
                    else
                        create_and_add_token(TokenType::UINT, static_cast<uint64_t>(v), start_line, start_col);
                }
            }
            catch (const std::exception &e)
            {
                m_tokens.emplace_back(TokenType::ERROR, hex_str, std::string("Invalid hexadecimal literal: ") + e.what(), start_line, start_col);
            }
            return;
        }
        // Hỗ trợ literal hệ 2: 0b... hoặc 0B... (có thể thêm hậu tố u/U cho uint)
        if (m_source[m_start_lexeme] == '0' && (peek() == 'b' || peek() == 'B'))
        {
            advance(); // consume 'b' or 'B'
            int digits_start = m_current_pos;
            while (peek() == '0' || peek() == '1')
                advance();
            int digits_end = m_current_pos;
            bool is_uint_bin = false;
            if (peek() == 'u' || peek() == 'U')
            {
                is_uint_bin = true;
                advance();
            }
            std::string bin_str = m_source.substr(digits_start, digits_end - digits_start);
            if (bin_str.empty())
            {
                m_tokens.emplace_back(TokenType::ERROR, "0b", "Invalid binary literal.", start_line, start_col);
                return;
            }
            try
            {
                unsigned long long v = std::stoull(bin_str, nullptr, 2);
                if (is_uint_bin)
                {
                    create_and_add_token(TokenType::UINT, static_cast<uint64_t>(v), start_line, start_col);
                }
                else
                {
                    if (v <= static_cast<unsigned long long>(std::numeric_limits<int64_t>::max()))
                        create_and_add_token(TokenType::INT, static_cast<int64_t>(v), start_line, start_col);
                    else
                        create_and_add_token(TokenType::UINT, static_cast<uint64_t>(v), start_line, start_col);
                }
            }
            catch (const std::exception &e)
            {
                m_tokens.emplace_back(TokenType::ERROR, bin_str, std::string("Invalid binary literal: ") + e.what(), start_line, start_col);
            }
            return;
        }
        while (isdigit(peek()))
            advance();
        bool is_float = false;
        if (peek() == '.' && isdigit(peek_next()))
        {
            is_float = true;
            advance();
            while (isdigit(peek()))
                advance();
        }
        std::string num_str = m_source.substr(m_start_lexeme, m_current_pos - m_start_lexeme);
        // Kiểm tra hậu tố u/U cho uint
        bool is_uint = false;
        if ((peek() == 'u' || peek() == 'U') && !is_float)
        {
            is_uint = true;
            advance();
        }
        try
        {
            if (is_float)
                create_and_add_token(TokenType::FLOAT_NUM, std::stod(num_str), start_line, start_col);
            else if (is_uint)
                create_and_add_token(TokenType::UINT, static_cast<uint64_t>(std::stoull(num_str)), start_line, start_col);
            else
                create_and_add_token(TokenType::INT, std::stoll(num_str), start_line, start_col);
        }
        catch (const std::out_of_range &oor)
        {
            std::string error_message = (is_float ? "Float" : (is_uint ? "Uint" : "Integer")) +
                                        std::string(" literal '") + num_str +
                                        "' out of range: " + oor.what();
            m_tokens.emplace_back(TokenType::ERROR, num_str, error_message, start_line, start_col);
        }
    }
    void Lexer::handle_identifier(int start_line, int start_col)
    {
        while (isalnum(peek()) || peek() == '_')
            advance();
        std::string text = m_source.substr(m_start_lexeme, m_current_pos - m_start_lexeme);
        auto it = s_keywords.find(text);
        if (it != s_keywords.end())
        {
            TokenType type = it->second;
            if (type == TokenType::TRUE_KW)
                create_and_add_token(type, true, start_line, start_col);
            else if (type == TokenType::FALSE_KW)
                create_and_add_token(type, false, start_line, start_col);
            else
                create_and_add_token(type, start_line, start_col);
        }
        else
        {
            create_and_add_token(TokenType::IDENTIFIER, start_line, start_col);
        }
    }
    void Lexer::handle_block_comment(int start_line, int start_col)
    {
        while (!(peek() == '*' && peek_next() == '/') && !is_at_end())
        {
            if (peek() == '\n')
            {
                m_current_line++;
                m_current_col_scan = 0;
            }
            advance();
        }
        if (is_at_end())
        {
            m_tokens.emplace_back(TokenType::ERROR, m_source.substr(m_start_lexeme, m_current_pos - m_start_lexeme), "Unterminated block comment.", start_line, start_col);
            return;
        }
        advance();
        advance();
    }
    std::vector<Token> Lexer::scan_tokens()
    {
        m_tokens.clear();
        while (!is_at_end())
        {
            m_start_lexeme = m_current_pos;
            int lexeme_start_line = m_current_line;
            int lexeme_start_col = m_current_col_scan;
            char c = advance();
            switch (c)
            {
            case '(':
                create_and_add_token(TokenType::LPAREN, lexeme_start_line, lexeme_start_col);
                break;
            case ')':
                create_and_add_token(TokenType::RPAREN, lexeme_start_line, lexeme_start_col);
                break;
            case '{':
                create_and_add_token(TokenType::LBRACE, lexeme_start_line, lexeme_start_col);
                break;
            case '}':
                create_and_add_token(TokenType::RBRACE, lexeme_start_line, lexeme_start_col);
                break;
            case '[':
                create_and_add_token(TokenType::LBRACKET, lexeme_start_line, lexeme_start_col);
                break;
            case ']':
                create_and_add_token(TokenType::RBRACKET, lexeme_start_line, lexeme_start_col);
                break;
            case ',':
                create_and_add_token(TokenType::COMMA, lexeme_start_line, lexeme_start_col);
                break;
            case '.':
                create_and_add_token(TokenType::DOT, lexeme_start_line, lexeme_start_col);
                break;
            case ';':
                create_and_add_token(TokenType::SEMICOLON, lexeme_start_line, lexeme_start_col);
                break;
            case ':':
                create_and_add_token(TokenType::COLON, lexeme_start_line, lexeme_start_col);
                break;
            case '%':
                create_and_add_token(match('=') ? TokenType::PERCENT_ASSIGN : TokenType::PERCENT, lexeme_start_line, lexeme_start_col);
                break;
            case '#':
                create_and_add_token(match('=') ? TokenType::HASH_ASSIGN : TokenType::HASH, lexeme_start_line, lexeme_start_col);
                break;
            case '!':
                create_and_add_token(match('=') ? TokenType::NOT_EQ : TokenType::NOT, lexeme_start_line, lexeme_start_col);
                break;
            case '=':
                create_and_add_token(match('=') ? TokenType::EQ_EQ : TokenType::ASSIGN, lexeme_start_line, lexeme_start_col);
                break;
            case '&':
                if (match('&'))
                    create_and_add_token(TokenType::AND_LOGIC, lexeme_start_line, lexeme_start_col);
                else
                    create_and_add_token(TokenType::AMP, lexeme_start_line, lexeme_start_col);
                break;
            case '|':
                if (match('|'))
                    create_and_add_token(TokenType::OR_LOGIC, lexeme_start_line, lexeme_start_col);
                else
                    create_and_add_token(TokenType::PIPE, lexeme_start_line, lexeme_start_col);
                break;
            case '^':
                create_and_add_token(TokenType::CARET, lexeme_start_line, lexeme_start_col);
                break;
            case '~':
                create_and_add_token(TokenType::TILDE, lexeme_start_line, lexeme_start_col);
                break;
            case '<':
                if (match('<'))
                    create_and_add_token(TokenType::LT_LT, lexeme_start_line, lexeme_start_col);
                else if (match('='))
                    create_and_add_token(TokenType::LT_EQ, lexeme_start_line, lexeme_start_col);
                else
                    create_and_add_token(TokenType::LT, lexeme_start_line, lexeme_start_col);
                break;
            case '>':
                if (match('>'))
                    create_and_add_token(TokenType::GT_GT, lexeme_start_line, lexeme_start_col);
                else if (match('='))
                    create_and_add_token(TokenType::GT_EQ, lexeme_start_line, lexeme_start_col);
                else
                    create_and_add_token(TokenType::GT, lexeme_start_line, lexeme_start_col);
                break;
            case '-':
                if (match('-'))
                    create_and_add_token(TokenType::MINUS_MINUS, lexeme_start_line, lexeme_start_col);
                else if (match('='))
                    create_and_add_token(TokenType::MINUS_ASSIGN, lexeme_start_line, lexeme_start_col);
                else
                    create_and_add_token(TokenType::MINUS, lexeme_start_line, lexeme_start_col);
                break;
            case '+':
                if (match('+'))
                    create_and_add_token(TokenType::PLUS_PLUS, lexeme_start_line, lexeme_start_col);
                else if (match('='))
                    create_and_add_token(TokenType::PLUS_ASSIGN, lexeme_start_line, lexeme_start_col);
                else
                    create_and_add_token(TokenType::PLUS, lexeme_start_line, lexeme_start_col);
                break;
            case '*':
                if (match('*'))
                    create_and_add_token(TokenType::STAR_STAR, lexeme_start_line, lexeme_start_col);
                else if (match('='))
                    create_and_add_token(TokenType::STAR_ASSIGN, lexeme_start_line, lexeme_start_col);
                else
                    create_and_add_token(TokenType::STAR, lexeme_start_line, lexeme_start_col);
                break;
            case '/':
                if (match('/'))
                {
                    while (peek() != '\n' && !is_at_end())
                        advance();
                }
                else if (match('*'))
                    handle_block_comment(lexeme_start_line, lexeme_start_col);
                else if (match('='))
                    create_and_add_token(TokenType::SLASH_ASSIGN, lexeme_start_line, lexeme_start_col);
                else
                    create_and_add_token(TokenType::SLASH, lexeme_start_line, lexeme_start_col);
                break;
            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n':
                m_current_line++;
                m_current_col_scan = 1;
                break;
            case '"':
            case '\'':
            case '`':
                handle_string_literal(c, lexeme_start_line, lexeme_start_col);
                break;
            default:
                if (isdigit(c))
                    handle_number_literal(lexeme_start_line, lexeme_start_col);
                else if (isalpha(c) || c == '_')
                    handle_identifier(lexeme_start_line, lexeme_start_col);
                else
                    m_tokens.emplace_back(TokenType::ERROR, std::string(1, c), "Unexpected character.", lexeme_start_line, lexeme_start_col);
                break;
            }
        }
        m_current_col_scan = 1;
        m_tokens.emplace_back(TokenType::END_OF_FILE, "", std::monostate{}, m_current_line, m_current_col_scan);
        return m_tokens;
    }
} // namespace Linh