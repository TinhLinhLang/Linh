// --- LinhC/Parsing/Parser/ParseType.cpp ---
#include "Parser.hpp" // Header chính của Parser
#include <iostream>   // Cho std::cout (debug)
#include <string>     // Cho std::string
#include <vector>     // Cho std::vector trong parse_union_type

// Nếu bạn có hàm debug_token_info và muốn sử dụng ở đây,
// bạn có thể include tệp chứa nó hoặc định nghĩa lại ở đây.

namespace Linh
{
    // --- Type Parsing Methods ---

    AST::TypeNodePtr Parser::parse_type()
    {
        // Ưu tiên check array/map generic trước
        if (check(TokenType::ARRAY_KW)) {
            advance();
            Token array_kw_token = previous();
            if (check(TokenType::LT)) {
                consume(TokenType::LT, "Thiếu '<' sau 'array' cho khai báo kiểu array.");
                AST::TypeNodePtr element_type = parse_type();
                consume(TokenType::GT, "Thiếu '>' sau kiểu phần tử array.");
                Token dummy_rbracket(TokenType::RBRACKET, "]", std::monostate{}, array_kw_token.line, array_kw_token.column_start + static_cast<int>(array_kw_token.lexeme.length()));
                return std::make_unique<AST::ArrayTypeNode>(array_kw_token, std::move(element_type), dummy_rbracket);
            }
            // Nếu không có <...>, coi như array<any>
            Token dummy_rbracket(TokenType::RBRACKET, "]", std::monostate{}, array_kw_token.line, array_kw_token.column_start + static_cast<int>(array_kw_token.lexeme.length()));
            return std::make_unique<AST::ArrayTypeNode>(array_kw_token, nullptr, dummy_rbracket);
        }
        if (check(TokenType::MAP_KW)) {
            advance();
            Token map_kw_token = previous();
            if (check(TokenType::LT)) {
                consume(TokenType::LT, "Thiếu '<' sau 'map' cho khai báo kiểu map.");
                AST::TypeNodePtr key_type = parse_type();
                consume(TokenType::COMMA, "Thiếu ',' giữa kiểu khóa và kiểu giá trị trong khai báo map.");
                AST::TypeNodePtr value_type = parse_type();
                consume(TokenType::GT, "Thiếu '>' để kết thúc khai báo kiểu map.");
                return std::make_unique<AST::MapTypeNode>(map_kw_token, std::move(key_type), std::move(value_type));
            }
            // Nếu không có <...>, báo lỗi hoặc trả về map<any,any>
            Token dummy_token(TokenType::IDENTIFIER, "any", std::monostate{}, map_kw_token.line, map_kw_token.column_start);
            return std::make_unique<AST::MapTypeNode>(map_kw_token, std::make_unique<AST::BaseTypeNode>(dummy_token), std::make_unique<AST::BaseTypeNode>(dummy_token));
        }
        // std::cout << "PARSER_TYPE: parse_type() called. Peek: " << debug_token_info(peek()) << std::endl;
        AST::TypeNodePtr type_node;

        // Thứ tự kiểm tra quan trọng để tránh nhập nhằng
        if (peek().type == TokenType::LT)
        { // Bắt đầu của Union Type: <Type1, Type2, ...>
            type_node = parse_union_type();
        }
        else
        {
            // Các trường hợp còn lại: BaseType, SizedType, Identifier (có thể là tên kiểu), 'array'
            type_node = parse_primary_type();
        }

        // Sau khi parse phần đầu của kiểu, kiểm tra xem có phải là kiểu mảng không (suffix `[]`)
        // Có thể có nhiều cặp `[]`, ví dụ: int[][]
        while (check(TokenType::LBRACKET))
        {
            // std::cout << "PARSER_TYPE: Found LBRACKET for array suffix." << std::endl;
            type_node = parse_array_suffix_type(std::move(type_node));
        }

        // std::cout << "PARSER_TYPE: Finished parse_type()." << std::endl;
        return type_node;
    }

    AST::TypeNodePtr Parser::parse_primary_type()
    {
        // Hàm này parse các "đơn vị" cơ bản của một kiểu:
        // - Từ khóa kiểu cơ bản (int, str, bool, void, any, uninit)
        // - Kiểu có kích thước (int<N>, uint<N>, float<N>)
        // - Từ khóa 'array' (cho array<any>)
        // - IDENTIFIER (có thể là tên kiểu do người dùng định nghĩa sau này)
        // - (Type) : kiểu trong dấu ngoặc (chưa hỗ trợ nhưng có thể cần)

        // std::cout << "PARSER_TYPE: parse_primary_type(). Peek: " << debug_token_info(peek()) << std::endl;
        Token type_token = peek(); // Nhìn token hiện tại để quyết định

        // Xử lý các kiểu có khả năng đi kèm kích thước <N>
        if (match({TokenType::INT_KW, TokenType::UINT_KW, TokenType::FLOAT_KW}))
        {
            Token base_keyword_token = previous(); // Token INT_KW, UINT_KW, hoặc FLOAT_KW

            if (check(TokenType::LT))
            { // Có thể là int<N>, uint<N>, float<N>
                consume(TokenType::LT, "Mong đợi '<' sau từ khóa kiểu số để chỉ định kích thước.");
                Token size_val_token = consume(TokenType::INT, "Mong đợi một số nguyên cho kích thước kiểu (ví dụ: int<32>).");
                consume(TokenType::GT, "Mong đợi '>' sau khi chỉ định kích thước kiểu.");

                if (base_keyword_token.type == TokenType::FLOAT_KW)
                {
                    // float<32>, float<64>
                    return std::make_unique<AST::SizedFloatTypeNode>(base_keyword_token, size_val_token);
                }
                else
                { // INT_KW hoặc UINT_KW
                    // int<8>, int<16>, int<32>, int<64>, uint<8>, ...
                    return std::make_unique<AST::SizedIntegerTypeNode>(base_keyword_token, size_val_token);
                }
            }
            // Nếu không có <N>, chỉ là kiểu cơ bản
            return std::make_unique<AST::BaseTypeNode>(base_keyword_token);
        }

        // Xử lý các kiểu cụ thể mới (int8, int16, int32, int64, uint8, uint16, uint32, uint64, float32, float64)
        if (match({TokenType::INT8_KW, TokenType::INT16_KW, TokenType::INT32_KW, TokenType::INT64_KW,
                   TokenType::UINT8_KW, TokenType::UINT16_KW, TokenType::UINT32_KW, TokenType::UINT64_KW,
                   TokenType::FLOAT32_KW, TokenType::FLOAT64_KW}))
        {
            return std::make_unique<AST::BaseTypeNode>(previous());
        }

        // Xử lý các từ khóa kiểu cơ bản khác không có kích thước <N>
        if (match({TokenType::STR_KW, TokenType::BOOL_KW, TokenType::VOID_KW,
                   TokenType::ANY_KW, TokenType::SOL_KW, TokenType::BYTE_KW}))
        {
            Token kw = previous();
            return std::make_unique<AST::BaseTypeNode>(kw);
        }

        // --- Hỗ trợ type aliases ---
        // int -> int32, float -> float32, uint -> uint32
        if (check(TokenType::IDENTIFIER))
        {
            const std::string& lexeme = peek().lexeme;
            if (lexeme == "int")
            {
                Token int_token = advance();
                // Create an int32 token from the int token
                Token int32_token(TokenType::INT32_KW, "int32", std::monostate{}, int_token.line, int_token.column_start);
                return std::make_unique<AST::BaseTypeNode>(int32_token);
            }
            else if (lexeme == "float")
            {
                Token float_token = advance();
                // Create a float32 token from the float token
                Token float32_token(TokenType::FLOAT32_KW, "float32", std::monostate{}, float_token.line, float_token.column_start);
                return std::make_unique<AST::BaseTypeNode>(float32_token);
            }
            else if (lexeme == "uint")
            {
                Token uint_token = advance();
                // Create a uint32 token from the uint token
                Token uint32_token(TokenType::UINT32_KW, "uint32", std::monostate{}, uint_token.line, uint_token.column_start);
                return std::make_unique<AST::BaseTypeNode>(uint32_token);
            }
            else if (lexeme == "bytes")
            {
                // Alias: bytes -> array<byte>
                Token bytes_token = advance();

                // Create synthetic tokens for 'array' and 'byte'
                Token array_kw_token(TokenType::ARRAY_KW, "array", std::monostate{}, bytes_token.line, bytes_token.column_start);
                Token byte_kw_token(TokenType::BYTE_KW, "byte", std::monostate{}, bytes_token.line, bytes_token.column_start);

                // Build element type node 'byte'
                auto byte_type_node = std::make_unique<AST::BaseTypeNode>(byte_kw_token);

                // Create a dummy RBRACKET token to satisfy ArrayTypeNode constructor that relies on a closing bracket
                // We position it right after 'array' keyword for simplicity
                Token dummy_rbracket(TokenType::RBRACKET, "]", std::monostate{}, array_kw_token.line, array_kw_token.column_start + static_cast<int>(array_kw_token.lexeme.length()));

                // Return array<byte> as ArrayTypeNode
                return std::make_unique<AST::ArrayTypeNode>(array_kw_token, std::move(byte_type_node), dummy_rbracket);
            }
        }

        // --- Hỗ trợ string như một alias của str ---
        if (check(TokenType::IDENTIFIER) && peek().lexeme == "string")
        {
            Token string_token = advance();
            // Xử lý các trường hợp string<...> hoặc string[...]
            if (check(TokenType::LT))
            {
                consume(TokenType::LT, "Mong đợi '<' sau 'string' để giới hạn số ký tự.");
                if (check(TokenType::INT))
                {
                    Token index_token = consume(TokenType::INT, "Mong đợi số nguyên cho chỉ số giới hạn string.");
                    consume(TokenType::GT, "Thiếu '>' sau chỉ số string.");
                    Token str_token(TokenType::STR_KW, "str", std::string(""), string_token.line, string_token.column_start);
                    auto node = std::make_unique<AST::BaseTypeNode>(str_token);
                    node->template_arg = std::stoi(index_token.lexeme);
                    return node;
                }
                else if (check(TokenType::ANY_KW))
                {
                    Token any_token = consume(TokenType::ANY_KW, "Mong đợi 'any' hoặc số nguyên trong string<any>.");
                    consume(TokenType::GT, "Thiếu '>' sau 'any' trong string<any>.");
                    Token str_token(TokenType::STR_KW, "str", std::string(""), string_token.line, string_token.column_start);
                    auto node = std::make_unique<AST::BaseTypeNode>(str_token);
                    node->template_arg = std::nullopt;
                    return node;
                }
                else
                {
                    throw error(peek(), "Mong đợi số nguyên hoặc 'any' trong string<...>.");
                }
            }
            if (check(TokenType::LBRACKET))
            {
                consume(TokenType::LBRACKET, "Mong đợi '[' sau 'string' để giới hạn số ký tự.");
                if (check(TokenType::INT))
                {
                    Token index_token = consume(TokenType::INT, "Mong đợi số nguyên cho chỉ số giới hạn string.");
                    consume(TokenType::RBRACKET, "Thiếu ']' sau chỉ số string.");
                    Token str_token(TokenType::STR_KW, "str", std::string(""), string_token.line, string_token.column_start);
                    auto node = std::make_unique<AST::BaseTypeNode>(str_token);
                    node->template_arg = std::stoi(index_token.lexeme);
                    return node;
                }
                else if (check(TokenType::ANY_KW))
                {
                    Token any_token = consume(TokenType::ANY_KW, "Mong đợi 'any' hoặc số nguyên trong string[any].");
                    consume(TokenType::RBRACKET, "Thiếu ']' sau 'any' trong string[any].");
                    Token str_token(TokenType::STR_KW, "str", std::string(""), string_token.line, string_token.column_start);
                    auto node = std::make_unique<AST::BaseTypeNode>(str_token);
                    node->template_arg = std::nullopt;
                    return node;
                }
                else
                {
                    throw error(peek(), "Mong đợi số nguyên hoặc 'any' trong string[...].");
                }
            }
            // Nếu không có <...> hoặc [...], chỉ là string không giới hạn
            Token str_token(TokenType::STR_KW, "str", std::string(""), string_token.line, string_token.column_start);
            return std::make_unique<AST::BaseTypeNode>(str_token);
        }

        // Xử lý từ khóa 'array' cho array<any>
        if (match({TokenType::ARRAY_KW}))
        {
            Token array_kw_token = previous();
            // Đối với 'array', element_type trong AST::ArrayTypeNode sẽ là nullptr (ngầm định là any)
            // Tạo một RBRACKET giả để đảm bảo constructor của ArrayTypeNode (phiên bản cho 'array') được gọi đúng
            Token dummy_rbracket(TokenType::RBRACKET, "]", std::monostate{}, array_kw_token.line, array_kw_token.column_start + static_cast<int>(array_kw_token.lexeme.length()));
            return std::make_unique<AST::ArrayTypeNode>(array_kw_token, nullptr, dummy_rbracket);
        }

        // --- Bổ sung hỗ trợ array<type> ---
        if (match({TokenType::ARRAY_KW}))
        {
            Token array_kw_token = previous();
            if (check(TokenType::LT))
            {
                consume(TokenType::LT, "Thiếu '<' sau 'array' cho khai báo kiểu array.");
                AST::TypeNodePtr element_type = parse_type();
                consume(TokenType::GT, "Thiếu '>' sau kiểu phần tử array.");
                // Tạo một RBRACKET giả để đảm bảo constructor của ArrayTypeNode (phiên bản cho 'array') được gọi đúng
                Token dummy_rbracket(TokenType::RBRACKET, "]", std::monostate{}, array_kw_token.line, array_kw_token.column_start + static_cast<int>(array_kw_token.lexeme.length()));
                return std::make_unique<AST::ArrayTypeNode>(array_kw_token, std::move(element_type), dummy_rbracket);
            }
            // Nếu không có <...>, coi như array<any>
            Token dummy_rbracket(TokenType::RBRACKET, "]", std::monostate{}, array_kw_token.line, array_kw_token.column_start + static_cast<int>(array_kw_token.lexeme.length()));
            return std::make_unique<AST::ArrayTypeNode>(array_kw_token, nullptr, dummy_rbracket);
        }

        // --- Bổ sung hỗ trợ type[] viết tắt cho array<type> ---
        // Nếu là kiểu cơ bản hoặc identifier, kiểm tra tiếp nếu có LBRACKET
        if (match({TokenType::BOOL_KW, TokenType::VOID_KW, TokenType::ANY_KW, TokenType::SOL_KW}))
        {
            Token base_token = previous();
            AST::TypeNodePtr node = std::make_unique<AST::BaseTypeNode>(base_token);
            // Nếu tiếp theo là LBRACKET, parse thành array
            while (check(TokenType::LBRACKET))
            {
                Token lbracket = consume(TokenType::LBRACKET, "Thiếu '[' sau kiểu để tạo mảng.");
                Token rbracket = consume(TokenType::RBRACKET, "Thiếu ']' sau '[' trong khai báo mảng.");
                node = std::make_unique<AST::ArrayTypeNode>(std::move(node), rbracket);
            }
            return node;
        }
        if (type_token.type == TokenType::IDENTIFIER)
        {
            advance(); // Consume IDENTIFIER
            AST::TypeNodePtr node = std::make_unique<AST::BaseTypeNode>(type_token);
            // Nếu tiếp theo là LBRACKET, parse thành array
            if (check(TokenType::LBRACKET))
            {
                Token lbracket = consume(TokenType::LBRACKET, "Thiếu '[' sau kiểu để tạo mảng.");
                if (check(TokenType::INT))
                {
                    Token index_token = consume(TokenType::INT, "Mong đợi số nguyên cho chỉ số giới hạn.");
                    Token rbracket = consume(TokenType::RBRACKET, "Thiếu ']' sau chỉ số.");
                    // Nếu là str, ánh xạ thành str<index>
                    if (type_token.lexeme == "str")
                    {
                        auto str_node = std::make_unique<AST::BaseTypeNode>(type_token);
                        str_node->template_arg = std::stoi(index_token.lexeme);
                        return str_node;
                    }
                    // Nếu muốn hỗ trợ cho các kiểu khác, có thể xử lý tại đây
                    // Nếu không, coi là array với kích thước (bỏ qua index)
                    node = std::make_unique<AST::ArrayTypeNode>(std::move(node), rbracket);
                }
                else
                {
                    Token rbracket = consume(TokenType::RBRACKET, "Thiếu ']' sau '[' trong khai báo mảng.");
                    node = std::make_unique<AST::ArrayTypeNode>(std::move(node), rbracket);
                }
            }
            // Hỗ trợ nhiều dấu [] liên tiếp
            while (check(TokenType::LBRACKET))
            {
                Token lbracket = consume(TokenType::LBRACKET, "Thiếu '[' sau kiểu để tạo mảng.");
                Token rbracket = consume(TokenType::RBRACKET, "Thiếu ']' sau '[' trong khai báo mảng.");
                node = std::make_unique<AST::ArrayTypeNode>(std::move(node), rbracket);
            }
            return node;
        }

        // TODO: Nếu muốn hỗ trợ (Type) để nhóm kiểu, thêm logic ở đây:
        // if (match({TokenType::LPAREN})) {
        //     AST::TypeNodePtr grouped_type = parse_type(); // Đệ quy
        //     consume(TokenType::RPAREN, "Thiếu ')' sau kiểu trong ngoặc đơn.");
        //     return grouped_type; // Trả về kiểu đã được nhóm
        // }

        // Nếu không khớp với bất kỳ trường hợp nào ở trên, đó là lỗi
        throw error(peek(), "Mong đợi một từ khóa kiểu (int, str, bool, array, map, ...), tên kiểu, hoặc '<' cho union type.");
    }

    AST::TypeNodePtr Parser::parse_array_suffix_type(AST::TypeNodePtr element_type_node)
    {
        // Hàm này được gọi khi một `element_type_node` đã được parse
        // và token hiện tại là `LBRACKET` (`[`).
        // std::cout << "PARSER_TYPE: parse_array_suffix_type(). Element type parsed. Peek: " << debug_token_info(peek()) << std::endl;

        consume(TokenType::LBRACKET, "Lỗi nội bộ: Mong đợi '[' cho hậu tố kiểu mảng."); // Lỗi này không nên xảy ra nếu check() đúng

        // Tài liệu không đề cập đến kích thước trong `[]` (ví dụ: `int[5]`).
        // Nếu sau này bạn muốn hỗ trợ, bạn sẽ cần parse một biểu thức ở đây.
        // Ví dụ:
        // AST::ExprPtr size_expr = nullptr;
        // if (!check(TokenType::RBRACKET)) { // Nếu không phải là ']' ngay -> có biểu thức kích thước
        //     size_expr = expression(); // Parse biểu thức kích thước
        // }

        Token r_bracket_token = consume(TokenType::RBRACKET, "Thiếu ']' sau khai báo kiểu mảng.");

        // Tạo ArrayTypeNode với element_type đã có và r_bracket_token
        return std::make_unique<AST::ArrayTypeNode>(std::move(element_type_node), r_bracket_token);
    }

    AST::TypeNodePtr Parser::parse_map_type()
    {
        // std::cout << "PARSER_TYPE: parse_map_type(). Peek: " << debug_token_info(peek()) << std::endl;
        Token map_kw_token = consume(TokenType::MAP_KW, "Mong đợi từ khóa 'map'."); // Đã được check bởi parse_type()
        consume(TokenType::LT, "Thiếu '<' sau 'map' cho khai báo kiểu map.");

        AST::TypeNodePtr key_type = parse_type(); // Đệ quy để parse kiểu của khóa
        consume(TokenType::COMMA, "Thiếu ',' giữa kiểu khóa và kiểu giá trị trong khai báo map.");
        AST::TypeNodePtr value_type = parse_type(); // Đệ quy để parse kiểu của giá trị

        consume(TokenType::GT, "Thiếu '>' để kết thúc khai báo kiểu map.");

        return std::make_unique<AST::MapTypeNode>(map_kw_token, std::move(key_type), std::move(value_type));
    }

    AST::TypeNodePtr Parser::parse_union_type()
    {
        // std::cout << "PARSER_TYPE: parse_union_type(). Peek: " << debug_token_info(peek()) << std::endl;
        Token l_angle_token = consume(TokenType::LT, "Thiếu '<' để bắt đầu union type."); // Đã được check

        std::vector<AST::TypeNodePtr> types_in_union;

        // Union type không được rỗng, ví dụ: `<>` là không hợp lệ
        if (check(TokenType::GT))
        {
            throw error(peek(), "Union type không được rỗng. Phải có ít nhất một kiểu bên trong '<...>'.");
        }

        do
        {
            types_in_union.push_back(parse_type()); // Đệ quy parse mỗi kiểu trong union
        } while (match({TokenType::COMMA})); // Tiếp tục nếu có dấu phẩy

        Token r_angle_token = consume(TokenType::GT, "Thiếu '>' để kết thúc union type.");

        // Kiểm tra lại lần nữa phòng trường hợp logic sai, mặc dù check(GT) ở trên đã xử lý trường hợp `<>`
        if (types_in_union.empty())
        {
            throw error(l_angle_token, "Union type không được rỗng (lỗi logic).");
        }
        return std::make_unique<AST::UnionTypeNode>(l_angle_token, std::move(types_in_union), r_angle_token);
    }

} // namespace Linh