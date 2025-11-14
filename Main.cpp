/*
<----------------Tinh Linh Language - Linh Interpreter-------------->
// Part of the Tinh Linh language ecosystem:
//   - Linh (this project): Interpreter implementation
//   - Tinh: AOT compiler with runtime (similar to Go)
//   - Lithium: Language standard specification
//
// Tinh Linh is a multi-typed language supporting both static (vas) and dynamic (var) typing, as well as const.
// Tinh Linh does not allow the existence of null. Only 'nothing' is used as the primitive 'no-value'.
//
// The id(x) function returns the id of the value, not a unique id for each variable.
// For primitive types (int, float, bool, string), id(x) is the same if the value is the same.
// For array/map, id(x) is the memory address (reference), so it is the same if they refer to the same object.
//
// IMPORTANT: Linh is NOT Python or JavaScript. Not everything is an object.
// Primitive types (int, uint, float, bool, str, nothing) are NOT objects and do not have methods or properties.
// Only array/map are reference types (objects), but they are not class-based objects.
// There is no class, prototype, or inheritance system. Linh is a statically-typed, value-oriented language.
// <------------------------------------------------------------------>
*/

#include "LinhC/Parsing/Lexer/Lexer.hpp"
#include "LinhC/Parsing/Parser/Parser.hpp"
#include "LinhC/Parsing/AST/ASTPrinter.hpp" // For printing AST
#include "LinhC/Parsing/Semantic/SemanticAnalyzer.hpp"
#include "LinhC/Bytecode/BytecodeEmitter.hpp"
#include "LiVM/LiVM.hpp"
#include "REPL/REPL.hpp" // Thêm dòng này
#include <iostream>
#include <fstream>
#include <sstream>
#include <string> // For std::string
// utf 8
#ifdef _WIN32
#include <windows.h>
#else
#include <locale>
#endif
#include "config/config.hpp" // Thêm dòng này để lấy thông tin version

void force_console_utf8()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    std::locale::global(std::locale(""));
    std::cout.imbue(std::locale());
#endif
}

void runSource(const std::string &source_code,
               const std::string &source_file_path = "",
               Linh::Semantic::SemanticAnalyzer *sema_ptr = nullptr,
               Linh::BytecodeEmitter *emitter_ptr = nullptr,
               Linh::LiVM *vm_ptr = nullptr);

void runFile(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file)
    {
#ifdef _DEBUG
        std::cerr << "Could not open file: " << filename << std::endl;
#endif
        return;
    }
    std::string line, source;
    while (std::getline(file, line))
    {
        source += line + "\n";
    }
    runSource(source, filename, nullptr, nullptr, nullptr);
}

// Đặt biến này vào đúng namespace Linh::Semantic để tránh lỗi linker
namespace Linh
{
    namespace Semantic
    {
        Linh::BytecodeEmitter *g_main_emitter = nullptr;
    }
}

void runSource(const std::string &source_code,
               const std::string &source_file_path,
               Linh::Semantic::SemanticAnalyzer *sema_ptr,
               Linh::BytecodeEmitter *emitter_ptr,
               Linh::LiVM *vm_ptr)
{
#ifdef _DEBUG
    std::cout << "--- Source Code Being Parsed ---\n"
              << source_code << "\n--------------------------------\n";
#endif
    Linh::Lexer lexer(source_code);
    std::vector<Linh::Token> tokens = lexer.scan_tokens();
    Linh::Parser parser(tokens);
    Linh::AST::StmtList ast = parser.parse();
    if (parser.had_error())
    {
#ifdef _DEBUG
        std::cerr << "Lỗi cú pháp, dừng thực thi." << std::endl;
#endif
        return;
    }

    Linh::BytecodeEmitter emitter;
    Linh::Semantic::g_main_emitter = &emitter; // Đặt emitter chính trước khi semantic để import có thể merge
    Linh::Semantic::SemanticAnalyzer sema;
    
    // Set the current file path for proper module resolution
    if (!source_file_path.empty())
    {
        sema.set_current_file_path(source_file_path);
    }
    
    sema.analyze(ast);
    if (!sema.errors.empty())
    {
#ifdef _DEBUG
        for (const auto &err : sema.errors)
        {
            std::cerr << "[Line " << err.line << ", Col " << err.column << "] SemanticError : " << err.message << std::endl;
        }
        std::cerr << "Có lỗi semantic, dừng thực thi." << std::endl;
#endif
        return;
    }
    emitter.emit(ast);
    Linh::Semantic::g_main_emitter = nullptr; // Đặt lại sau khi xong

    // --- Debug: In ra danh sách function sau khi merge ---
#ifdef _DEBUG
    std::cout << "\n--- Function Table After Import Merge ---\n";
    for (const auto &kv : emitter.get_functions())
    {
        std::cout << "Function: " << kv.first << ", param_count: " << kv.second.param_names.size() << "\n";
    }
    std::cout << "------------------------------------------\n";
#endif
    // -----------------------------------------------------

    // --- Run VM ---
    Linh::LiVM vm;
    
    // Set the current file path for VM module resolution
    if (!source_file_path.empty())
    {
        vm.current_file_path = source_file_path;
    }

    // --- Chuyển đổi function table ---
    std::unordered_map<std::string, Linh::LiVM::Function> vm_functions;
    for (const auto &kv : emitter.get_functions())
    {
        Linh::LiVM::Function fn;
        fn.code = kv.second.code;
        fn.param_names = kv.second.param_names;
        vm_functions[kv.first] = std::move(fn);
    }
    vm.set_functions(vm_functions);
    // --- Kết thúc chuyển đổi ---

    vm.run(emitter.get_chunk());

    // --- Debug: print VM stack and variables after execution (optional)
#ifdef _DEBUG
    // (You can add methods to LiVM to expose stack/vars for debugging if needed)

    std::cout << "\nParse succeeded!" << std::endl;
#endif
}

void runSource(const std::string &source_code)
{
    runSource(source_code, "", nullptr, nullptr, nullptr);
}

int main(int argc, char **argv)
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    force_console_utf8();
    if (argc > 1)
    {
        std::string arg1 = argv[1];
        if (arg1 == "-v" || arg1 == "--version" || arg1 == "-V")
        {
            std::cout << name << " (" << engine << ") version " << version << " [" << version_number << "]\n";
            std::cout << "Copyright (c) 2025 Sao Tin Developer Team\n";
            std::cout << "Author: " << author << "\n";
            std::cout << "Website: " << web << "\n";
            return 0;
        }
        runFile(argv[1]);
    }
    else
    {
        Linh::run_repl(); // Thay thế runREPL()
    }

    return 0;
}