#include "REPL.hpp"
#include "LinhC/Parsing/Lexer/Lexer.hpp"
#include "LinhC/Parsing/Parser/Parser.hpp"
#include "LinhC/Parsing/Semantic/SemanticAnalyzer.hpp"
#include "LinhC/Bytecode/BytecodeEmitter.hpp"
#include "LiVM/LiVM.hpp"
#include <iostream>
#include <string>
#include "config.hpp" // Thêm dòng này để lấy thông tin version

namespace Linh
{
    void run_repl()
    {
        std::string line;
        Linh::LiVM vm;
        Linh::BytecodeEmitter emitter;
        std::unordered_map<std::string, Linh::BytecodeEmitter::FunctionInfo> function_table;
        std::unordered_map<int, Linh::Value> global_vars; // Lưu biến toàn cục giữa các lần nhập
        Linh::Semantic::SemanticAnalyzer analyzer;        // Move outside loop to persist state
        std::cout << "Welcome to Tinh Linh v" << version << "\n";
        std::cout << "Linh REPL (type '.exit' or '.quit' to exit)\n";
        while (true)
        {
            std::string full_stmt;
            std::string prompt = ">>> ";
            bool first_line = true;
            int paren = 0, brace = 0, bracket = 0;
            bool in_single_quote = false, in_double_quote = false;
            bool statement_complete = false;
            do
            {
                std::cout << prompt;
                if (!std::getline(std::cin, line))
                    return;
                if (first_line && (line == ".exit" || line == ".quit"))
                    return;
                if (first_line && line.empty())
                    continue;
                if (!full_stmt.empty())
                    full_stmt += "\n";
                full_stmt += line;
                // Smart multi-line detection
                for (size_t i = 0; i < line.size(); ++i)
                {
                    char c = line[i];
                    if (!in_single_quote && !in_double_quote)
                    {
                        if (c == '(')
                            paren++;
                        else if (c == ')')
                            paren--;
                        else if (c == '{')
                            brace++;
                        else if (c == '}')
                            brace--;
                        else if (c == '[')
                            bracket++;
                        else if (c == ']')
                            bracket--;
                        else if (c == '\"')
                            in_double_quote = true;
                        else if (c == '\'')
                            in_single_quote = true;
                    }
                    else
                    {
                        if (c == '\"' && in_double_quote)
                            in_double_quote = false;
                        if (c == '\'' && in_single_quote)
                            in_single_quote = false;
                    }
                }
                // Statement is complete if all are balanced and line ends with ; or block close or not inside quotes
                statement_complete = (paren <= 0 && brace <= 0 && bracket <= 0 && !in_single_quote && !in_double_quote);
                if (!statement_complete)
                    prompt = "... ";
                first_line = false;
            } while (!statement_complete);

            // Chỉ parse và emit cho dòng vừa nhập
            Linh::Lexer lexer(full_stmt);
            auto tokens = lexer.scan_tokens();
            Linh::Parser parser(tokens);
            auto stmts = parser.parse();

            analyzer.analyze(stmts, false); // REPL mode: giữ lại scope

            if (!analyzer.errors.empty())
            {
#ifdef _DEBUG
                for (const auto &err : analyzer.errors)
                {
                    std::cerr << "[" << (err.stage == ErrorStage::Lexer ? "Lexer" : err.stage == ErrorStage::Parser ? "Parser"
                                                                                : err.stage == ErrorStage::Semantic ? "Semantic"
                                                                                : err.stage == ErrorStage::Bytecode ? "Bytecode"
                                                                                                                    : "Unknown")
                              << "] " << err.message << std::endl;
                }
#endif
                continue;
            }

            emitter.emit(stmts);
            // Cập nhật function table (giữ lại các hàm đã khai báo trước đó)
            auto new_funcs = emitter.get_functions();
            for (const auto &kv : new_funcs)
                function_table[kv.first] = kv.second;

            // Chuyển đổi function_table sang Linh::LiVM::Function
            std::unordered_map<std::string, Linh::LiVM::Function> vm_funcs;
            for (const auto &[name, finfo] : function_table)
            {
                Linh::LiVM::Function f;
                f.code = finfo.code;
                f.param_names = finfo.param_names;
                vm_funcs[name] = std::move(f);
            }
            vm.set_functions(vm_funcs);

            const auto &chunk = emitter.get_chunk();
            // Trước khi chạy, khôi phục biến toàn cục
            vm.set_global_variables(global_vars);
            vm.run(chunk);
            // Sau khi chạy, lưu lại biến toàn cục
            global_vars = vm.get_global_variables();
        }
    }
}
