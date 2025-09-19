#pragma once
#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <memory>
#include "../../Value/Value.hpp"

namespace Linh {
namespace Std {
    using FsFunction = std::function<Value(const Value&)>;
    
    // Khai báo các hàm fs
    Value fs_open(const Value& v);
    Value fs_read(const Value& v);
    Value fs_readline(const Value& v);
    Value fs_readlines(const Value& v);
    Value fs_write(const Value& v);
    Value fs_append(const Value& v);
    Value fs_close(const Value& v);
    Value fs_encoding(const Value& v);
    Value fs_seek(const Value& v);
    Value fs_size(const Value& v);
    Value fs_exists(const Value& v);
    Value fs_bWrite(const Value& v);
    Value fs_bAppend(const Value& v);
    Value fs_bRead(const Value& v);
    Value fs_listdir(const Value& v);
    Value fs_mkdir(const Value& v);
    Value fs_remove(const Value& v);
    Value fs_rmdir(const Value& v);
    Value fs_rename(const Value& v);
    Value fs_copy(const Value& v);
    Value fs_move(const Value& v);
    Value fs_stat(const Value& v);
    Value fs_isdir(const Value& v);
    Value fs_isfile(const Value& v);
    Value fs_isempty(const Value& v);
    Value fs_isopen(const Value& v);
    Value fs_rmall(const Value& v);
    
    // Global variables
    extern std::unordered_map<std::string, FsFunction> fs_functions;
    extern std::unordered_map<std::string, Value> fs_constants;
    extern bool fs_functions_initialized;
    extern int current_file_handle;

    // Function declarations
    FsFunction get_fs_function(const std::string& name);
    std::unordered_map<std::string, FsFunction> get_fs_functions();
    Value get_fs_constant(const std::string& name);
    std::unordered_map<std::string, Value> get_fs_constants();
    void initialize_fs_functions();
    void initialize_fs_constants();
    
    // Quản lý file handles
    class FileManager {
    public:
        static FileManager& instance();
        int open_file(const std::string& filename);
        std::string read_file(int handle);
        std::string read_line(int handle);
        std::vector<std::string> read_lines(int handle);
        bool write_file(int handle, const std::string& s);
        bool append_file(int handle, const std::string& s);
        bool set_encoding(int handle, const std::string& encoding);
        bool seek_file(int handle, long offset);
        long get_file_size(const std::string& filename);
        bool file_exists(const std::string& filename);
        void close_file(int handle);
        bool is_file_open(int handle);

        // Binary file operations (use array<byte>)
        bool bWrite_file(int handle, const Array& data);
        bool bAppend_file(int handle, const Array& data);
        Array bRead_file(int handle);

    private:
        FileManager() = default;
        std::unordered_map<int, std::unique_ptr<std::fstream>> open_files_;
        std::unordered_map<int, std::string> handle_to_filename_;
        int next_handle_ = 1;
        std::mutex mutex_;
    };
}
}
