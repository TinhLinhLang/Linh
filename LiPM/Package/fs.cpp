#include "fs.hpp"
#include <iostream>
#include <sstream>
#include <mutex>

namespace Linh {
namespace LiPM {

    FileManager& FileManager::instance() {
        static FileManager instance;
        return instance;
    }

    int FileManager::open_file(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto file = std::make_unique<std::fstream>(filename, std::ios::in | std::ios::out | std::ios::app);
        if (!file->is_open()) {
            // Nếu không mở được ở chế độ đọc/ghi, thử tạo mới file
            file = std::make_unique<std::fstream>(filename, std::ios::out);
            file->close();
            file = std::make_unique<std::fstream>(filename, std::ios::in | std::ios::out | std::ios::app);
            if (!file->is_open()) return -1;
        }
        int handle = next_handle_++;
        open_files_[handle] = std::move(file);
        handle_to_filename_[handle] = filename;
        return handle;
    }

    std::string FileManager::read_file(int handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return "";
        std::fstream& file = *(it->second);
        if (!file.is_open()) return "";
        
        // Lưu vị trí hiện tại
        std::streampos current_pos = file.tellg();
        
        // Đọc từ vị trí hiện tại đến cuối file
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        return content;
    }

    std::string FileManager::read_line(int handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return "";
        std::fstream& file = *(it->second);
        std::string line;
        if (std::getline(file, line)) return line;
        return "";
    }

    std::vector<std::string> FileManager::read_lines(int handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return {};
        std::fstream& file = *(it->second);
        std::streampos current_pos = file.tellg();
        file.clear();
        file.seekg(0, std::ios::beg);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) lines.push_back(line);
        file.clear();
        file.seekg(current_pos);
        return lines;
    }

    bool FileManager::write_file(int handle, const std::string& s) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return false;
        auto fn_it = handle_to_filename_.find(handle);
        if (fn_it == handle_to_filename_.end()) return false;
        std::string filename = fn_it->second;
        // Đóng file cũ
        it->second->close();
        // Mở lại file ở chế độ out|trunc để ghi đè
        std::fstream file(filename, std::ios::out | std::ios::trunc);
        if (!file.is_open()) return false;
        file << s;
        file.flush();
        file.close();
        // Mở lại file ở chế độ in|out|app để các thao tác khác không bị ảnh hưởng
        it->second = std::make_unique<std::fstream>(filename, std::ios::in | std::ios::out | std::ios::app);
        return it->second->is_open();
    }

    bool FileManager::append_file(int handle, const std::string& s) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return false;
        it->second->seekp(0, std::ios::end);
        *it->second << s;
        it->second->flush();
        return true;
    }

    bool FileManager::set_encoding(int handle, const std::string& encoding) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return false;
        // Lưu encoding vào một map riêng để sử dụng khi đọc/ghi
        // Tạm thời chỉ hỗ trợ UTF-8, có thể mở rộng sau
        if (encoding == "utf-8" || encoding == "UTF-8") {
            // Không cần làm gì đặc biệt cho UTF-8 trong C++
            return true;
        }
        // Các encoding khác chưa được hỗ trợ
        return false;
    }

    bool FileManager::seek_file(int handle, long offset) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return false;
        it->second->seekg(offset, std::ios::beg);
        it->second->seekp(offset, std::ios::beg);
        return !it->second->fail();
    }

    long FileManager::get_file_size(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return -1;
        long size = file.tellg();
        file.close();
        return size;
    }

    bool FileManager::file_exists(const std::string& filename) {
        std::ifstream file(filename);
        return file.good();
    }

    void FileManager::close_file(int handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it != open_files_.end()) {
            if (it->second->is_open()) it->second->close();
            open_files_.erase(it);
        }
    }

    bool FileManager::is_file_open(int handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        return open_files_.count(handle) > 0;
    }

    bool FileManager::bWrite_file(int handle, const ByteArray& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return false;
        auto fn_it = handle_to_filename_.find(handle);
        if (fn_it == handle_to_filename_.end()) return false;
        std::string filename = fn_it->second;
        
        it->second->close();
        
        std::fstream file(filename, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!file.is_open()) {
            // Try to reopen in standard mode if binary fails
            it->second = std::make_unique<std::fstream>(filename, std::ios::in | std::ios::out | std::ios::app);
            return false;
        }
        file.write(reinterpret_cast<const char*>(data->data()), data->size());
        file.flush();
        file.close();
        
        it->second = std::make_unique<std::fstream>(filename, std::ios::in | std::ios::out | std::ios::app | std::ios::binary);
        return it->second->is_open();
    }

    bool FileManager::bAppend_file(int handle, const ByteArray& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return false;
        
        it->second->seekp(0, std::ios::end);
        it->second->write(reinterpret_cast<const char*>(data->data()), data->size());
        it->second->flush();
        return true;
    }

    ByteArray FileManager::bRead_file(int handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return make_bytearray();
        std::fstream& file = *(it->second);
        if (!file.is_open()) return make_bytearray();

        std::streampos current_pos = file.tellg();
        file.seekg(0, std::ios::end);
        std::streampos end_pos = file.tellg();
        file.seekg(current_pos);

        if (end_pos <= current_pos) return make_bytearray();

        size_t size_to_read = end_pos - current_pos;
        auto buffer = make_bytearray();
        buffer->resize(size_to_read);
        file.read(reinterpret_cast<char*>(buffer->data()), size_to_read);
        return buffer;
    }

    // Global variables
    std::unordered_map<std::string, FsFunction> fs_functions;
    std::unordered_map<std::string, Value> fs_constants;
    bool fs_functions_initialized = false;
    int current_file_handle = -1;

    Value fs_open(const Value& v) {
        if (v.index() != 5) return Value(-1);
        std::string filename = std::get<std::string>(v);
        if (filename.empty()) return Value(-1);
        FileManager& fm = FileManager::instance();
        int handle = fm.open_file(filename);
        if (handle != -1) current_file_handle = handle;
        return Value(handle);
    }

    Value fs_read(const Value& v) {
        if (current_file_handle == -1) return Value("");
        FileManager& fm = FileManager::instance();
        if (!fm.is_file_open(current_file_handle)) return Value("");
        std::string content = fm.read_file(current_file_handle);
        return Value(content);
    }

    Value fs_readline(const Value& v) {
        if (current_file_handle == -1) return Value("");
        FileManager& fm = FileManager::instance();
        if (!fm.is_file_open(current_file_handle)) return Value("");
        std::string line = fm.read_line(current_file_handle);
        return Value(line);
    }

    Value fs_readlines(const Value& v) {
        if (current_file_handle == -1) return Value::new_array();
        FileManager& fm = FileManager::instance();
        if (!fm.is_file_open(current_file_handle)) return Value::new_array();
        std::vector<std::string> lines = fm.read_lines(current_file_handle);
        Array array = make_array();
        for (const auto& line : lines) array->push_back(Value(line));
        return Value(array);
    }

    Value fs_write(const Value& v) {
        if (current_file_handle == -1) return Value(false);
        FileManager& fm = FileManager::instance();
        if (!fm.is_file_open(current_file_handle)) return Value(false);
        if (v.index() != 5) return Value(false);
        std::string s = std::get<std::string>(v);
        bool ok = fm.write_file(current_file_handle, s);
        return Value(ok);
    }

    Value fs_append(const Value& v) {
        if (current_file_handle == -1) return Value(false);
        FileManager& fm = FileManager::instance();
        if (!fm.is_file_open(current_file_handle)) return Value(false);
        if (v.index() != 5) return Value(false);
        std::string s = std::get<std::string>(v);
        bool ok = fm.append_file(current_file_handle, s);
        return Value(ok);
    }

    Value fs_close(const Value& v) {
        if (current_file_handle != -1) {
            FileManager& fm = FileManager::instance();
            fm.close_file(current_file_handle);
            current_file_handle = -1;
        }
        return Value();
    }

    Value fs_encoding(const Value& v) {
        if (current_file_handle == -1) {
            return Value{};
        }
        FileManager& fm = FileManager::instance();
        if (!fm.is_file_open(current_file_handle)) {
            return Value{};
        }
        std::string encoding = std::holds_alternative<std::string>(v) ? std::get<std::string>(v) : "utf-8";
        bool success = fm.set_encoding(current_file_handle, encoding);
        return Value(success);
    }

    Value fs_seek(const Value& v) {
        if (current_file_handle == -1) {
            return Value{};
        }
        FileManager& fm = FileManager::instance();
        if (!fm.is_file_open(current_file_handle)) {
            return Value{};
        }
        long offset = 0;
        if (std::holds_alternative<double>(v)) {
            offset = static_cast<long>(std::get<double>(v));
        } else if (std::holds_alternative<int64_t>(v)) {
            offset = static_cast<long>(std::get<int64_t>(v));
        } else if (std::holds_alternative<uint64_t>(v)) {
            offset = static_cast<long>(std::get<uint64_t>(v));
        }
        bool success = fm.seek_file(current_file_handle, offset);
        return Value(success);
    }

    Value fs_size(const Value& v) {
        std::string filename = "";
        if (std::holds_alternative<std::string>(v)) {
            filename = std::get<std::string>(v);
        }
        if (filename.empty()) {
            return Value{};
        }
        FileManager& fm = FileManager::instance();
        long size = fm.get_file_size(filename);
        return Value(static_cast<double>(size));
    }

    Value fs_exists(const Value& v) {
        std::string path = "";
        if (std::holds_alternative<std::string>(v)) {
            path = std::get<std::string>(v);
        }
        if (path.empty()) {
            return Value(false);
        }
        FileManager& fm = FileManager::instance();
        bool exists = fm.file_exists(path);
        return Value(exists);
    }

    Value fs_bWrite(const Value& v) {
        if (current_file_handle == -1) return Value(false);
        if (!std::holds_alternative<ByteArray>(v)) return Value(false);
        const auto& data = std::get<ByteArray>(v);
        FileManager& fm = FileManager::instance();
        bool ok = fm.bWrite_file(current_file_handle, data);
        return Value(ok);
    }

    Value fs_bAppend(const Value& v) {
        if (current_file_handle == -1) return Value(false);
        if (!std::holds_alternative<ByteArray>(v)) return Value(false);
        const auto& data = std::get<ByteArray>(v);
        FileManager& fm = FileManager::instance();
        bool ok = fm.bAppend_file(current_file_handle, data);
        return Value(ok);
    }

    Value fs_bRead(const Value& v) {
        if (current_file_handle == -1) return Value{}; // Return nil
        FileManager& fm = FileManager::instance();
        ByteArray data = fm.bRead_file(current_file_handle);
        return Value(data);
    }

    void initialize_fs_functions() {
        if (fs_functions_initialized) return;
        fs_functions["open"] = fs_open;
        fs_functions["read"] = fs_read;
        fs_functions["readline"] = fs_readline;
        fs_functions["readlines"] = fs_readlines;
        fs_functions["write"] = fs_write;
        fs_functions["append"] = fs_append;
        fs_functions["close"] = fs_close;
        fs_functions["encoding"] = fs_encoding;
        fs_functions["seek"] = fs_seek;
        fs_functions["size"] = fs_size;
        fs_functions["exists"] = fs_exists;
        // Binary functions
        fs_functions["bWrite"] = fs_bWrite;
        fs_functions["bAppend"] = fs_bAppend;
        fs_functions["bRead"] = fs_bRead;
        fs_functions_initialized = true;
    }

    FsFunction get_fs_function(const std::string& function_name) {
        if (!fs_functions_initialized) initialize_fs_functions();
        auto it = fs_functions.find(function_name);
        if (it != fs_functions.end()) return it->second;
        return nullptr;
    }

    std::unordered_map<std::string, FsFunction> get_fs_functions() {
        if (!fs_functions_initialized) initialize_fs_functions();
        return fs_functions;
    }

    void initialize_fs_constants() {
        fs_constants["SEEK_SET"] = Value(static_cast<double>(0));  // std::ios::beg
        fs_constants["SEEK_CUR"] = Value(static_cast<double>(1));  // std::ios::cur
        fs_constants["SEEK_END"] = Value(static_cast<double>(2));  // std::ios::end
    }

    Value get_fs_constant(const std::string& name) {
        auto it = fs_constants.find(name);
        if (it != fs_constants.end()) {
            return it->second;
        }
        return Value{};
    }

    std::unordered_map<std::string, Value> get_fs_constants() {
        return fs_constants;
    }

} // namespace LiPM
} // namespace Linh
