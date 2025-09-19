#ifndef LINH_LEXER_HPP
#define LINH_LEXER_HPP

#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <cstdint>

namespace Linh
{

    using LiteralValue = std::variant<
        std::monostate,
        int64_t,
        uint64_t,
        double,
        std::string,
        bool>;

    enum class TokenType
    {
        // Single-character tokens
        LPAREN,
        RPAREN,
        LBRACE,
        RBRACE,
        LBRACKET,
        RBRACKET,
        COMMA,
        DOT,
        MINUS,
        PLUS,
        SEMICOLON,
        SLASH,
        STAR,
        PERCENT,
        COLON,
        HASH,
        // --- Bitwise operators ---
        AMP,   // &
        PIPE,  // |
        CARET, // ^
        TILDE, // ~
        LT_LT, // <<
        GT_GT, // >>

        // One or two character tokens
        NOT,
        NOT_EQ,
        ASSIGN,
        EQ_EQ,
        GT,
        GT_EQ,
        LT,
        LT_EQ,
        AND_LOGIC,
        OR_LOGIC,

        // Compound assignment & others
        PLUS_PLUS,
        MINUS_MINUS,
        PLUS_ASSIGN,
        MINUS_ASSIGN,
        STAR_ASSIGN,
        SLASH_ASSIGN,
        PERCENT_ASSIGN,
        HASH_ASSIGN,
        STAR_STAR,

        // Literals
        IDENTIFIER,
        STR,
        INT,
        UINT, // <--- Thêm dòng này cho literal uint
        FLOAT_NUM,

        // Keywords
        ARRAY_KW,
        BOOL_KW,
        CONST_KW,
        ELSE_KW,
        FALSE_KW,
        FLOAT_KW,
        FOR_KW,
        FUNC_KW,
        IF_KW,
        INT_KW,
        VAS_KW, // Đổi thành VAS_KW
        MAP_KW,
        PRINT_KW,
        INPUT_KW,
        RETURN_KW,
        STR_KW,
        TRUE_KW,
        UINT_KW,
        VAR_KW,
        VOID_KW,
        WHILE_KW,
        ANY_KW,
        BREAK_KW,
        CONTINUE_KW,
        SKIP_KW,
        SWITCH_KW,
        CASE_KW,
        DEFAULT_KW,
        OTHER_KW,
        TYPE_KW,
        ID_KW,
        SOL_KW,
        IMPORT_KW, // Đảm bảo dòng này nằm ở đây, đúng thứ tự với bảng từ khóa
        FROM_KW,   // Đảm bảo dòng này nằm ở đây, đúng thứ tự với bảng từ khóa

        // New keywords from previous steps
        IS_KW,     // is
        NOT_KW,    // not (keyword version)
        AND_KW,    // and (keyword version)
        OR_KW,     // or (keyword version)
        DO_KW,     // do
        NEW_KW,    // new
        DELETE_KW, // delete
        THIS_KW,   // this

        // Keywords for this update
        THROW_KW,   // throw
        TRY_KW,     // try
        CATCH_KW,   // catch
        FINALLY_KW, // finally
                    // CLASS_KW,   // For future
                    // PUBLIC_KW, PROTECTED_KW, PRIVATE_KW, STATIC_KW, // For future
                    // IMPORT_KW, FROM_KW, // For future

        // Special
        ERROR,
        END_OF_FILE,
        INTERP_START, // &{ mở nội suy
        INTERP_END,    // } đóng nội suy
        BYTE_KW,      // byte
        
        // Specific sized type keywords
        INT8_KW,      // int8
        INT16_KW,     // int16
        INT32_KW,     // int32
        INT64_KW,     // int64
        UINT8_KW,     // uint8
        UINT16_KW,    // uint16
        UINT32_KW,    // uint32
        UINT64_KW,    // uint64
        FLOAT32_KW,   // float32
        FLOAT64_KW,   // float64
    };

    std::string token_type_to_string(TokenType type);

    struct Token
    {
        TokenType type;
        std::string lexeme;
        LiteralValue literal;
        int line;
        int column_start;

        Token(TokenType type, std::string lexeme, LiteralValue literal, int line, int column_start);
        Token() : type(TokenType::ERROR), lexeme(""), literal(std::monostate{}), line(0), column_start(0) {} // <--- Thêm dòng này
        std::string to_string() const;
    };

    class Lexer
    {
    public:
        explicit Lexer(std::string source);
        std::vector<Token> scan_tokens();

    private:
        bool is_at_end() const;
        char advance();
        char peek() const;
        char peek_next() const;
        bool match(char expected);

        void create_and_add_token(TokenType type, int line, int col_start);
        void create_and_add_token(TokenType type, const LiteralValue &literal_val, int line, int col_start);

        void handle_string_literal(char quote_char, int start_line, int start_col);
        void handle_number_literal(int start_line, int start_col);
        void handle_identifier(int start_line, int start_col);
        void handle_block_comment(int start_line, int start_col);

        const std::string m_source;
        std::vector<Token> m_tokens;
        size_t m_start_lexeme = 0;
        size_t m_current_pos = 0;
        int m_current_line = 1;
        int m_current_col_scan = 1;

        static const std::unordered_map<std::string, TokenType> s_keywords;
    };

} // namespace Linh

#endif // LINH_LEXER_HPP