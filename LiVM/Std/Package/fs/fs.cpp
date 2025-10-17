#include "fs.hpp"
#include <iostream>
#include <sstream>
#include <mutex>
#include <filesystem>
#include <vector>

namespace Linh {
namespace Std {

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

    bool FileManager::bWrite_file(int handle, const Value& data) {
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

        // Convert Array of Values to raw bytes
        std::vector<unsigned char> buffer;
        if (data.is_array()) {
            const auto& arr = data.as_array_ref();
            buffer.reserve(arr.size());
            for (const auto& v : arr) {
                uint64_t iv = 0;
                if (Linh::holds_alternative<int64_t>(v)) iv = static_cast<uint64_t>(Linh::get<int64_t>(v));
                else if (Linh::holds_alternative<uint64_t>(v)) iv = Linh::get<uint64_t>(v);
                else if (Linh::holds_alternative<double>(v)) iv = static_cast<uint64_t>(Linh::get<double>(v));
                else if (Linh::holds_alternative<Byte>(v)) iv = Linh::get<Byte>(v);
                else if (Linh::holds_alternative<bool>(v)) iv = Linh::get<bool>(v) ? 1u : 0u;
                else if (Linh::holds_alternative<uint32_t>(v)) iv = Linh::get<uint32_t>(v);
                else if (Linh::holds_alternative<int32_t>(v)) iv = static_cast<uint64_t>(Linh::get<int32_t>(v));
                // Clamp to 0..255
                if (iv > 255) iv = 255;
                buffer.push_back(static_cast<unsigned char>(iv & 0xFF));
            }
        }

        if (!buffer.empty())
            file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        file.flush();
        file.close();
        
        it->second = std::make_unique<std::fstream>(filename, std::ios::in | std::ios::out | std::ios::app | std::ios::binary);
        return it->second->is_open();
    }

    bool FileManager::bAppend_file(int handle, const Value& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return false;
        
        // Convert Array of Values to raw bytes
        std::vector<unsigned char> buffer;
        if (data.is_array()) {
            const auto& arr = data.as_array_ref();
            buffer.reserve(arr.size());
            for (const auto& v : arr) {
                uint64_t iv = 0;
                if (Linh::holds_alternative<int64_t>(v)) iv = static_cast<uint64_t>(Linh::get<int64_t>(v));
                else if (Linh::holds_alternative<uint64_t>(v)) iv = Linh::get<uint64_t>(v);
                else if (Linh::holds_alternative<double>(v)) iv = static_cast<uint64_t>(Linh::get<double>(v));
                else if (Linh::holds_alternative<Byte>(v)) iv = Linh::get<Byte>(v);
                else if (Linh::holds_alternative<bool>(v)) iv = Linh::get<bool>(v) ? 1u : 0u;
                else if (Linh::holds_alternative<uint32_t>(v)) iv = Linh::get<uint32_t>(v);
                else if (Linh::holds_alternative<int32_t>(v)) iv = static_cast<uint64_t>(Linh::get<int32_t>(v));
                if (iv > 255) iv = 255;
                buffer.push_back(static_cast<unsigned char>(iv & 0xFF));
            }
        }

        it->second->seekp(0, std::ios::end);
        if (!buffer.empty())
            it->second->write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        it->second->flush();
        return true;
    }

    Value FileManager::bRead_file(int handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = open_files_.find(handle);
        if (it == open_files_.end()) return Value::make_array();
        std::fstream& file = *(it->second);
        if (!file.is_open()) return Value::make_array();
    
        std::streampos current_pos = file.tellg();
        file.seekg(0, std::ios::end);
        std::streampos end_pos = file.tellg();
        file.seekg(current_pos);
    
        if (end_pos <= current_pos) return Value::make_array();
    
        size_t size_to_read = static_cast<size_t>(end_pos - current_pos);
        std::vector<unsigned char> buf(size_to_read);
        file.read(reinterpret_cast<char*>(buf.data()), size_to_read);

        Value arr = Value::make_array();
        auto& arr_ref = arr.as_array_ref();
        arr_ref.reserve(buf.size());
        for (unsigned char c : buf) {
            arr_ref.push_back(Value(static_cast<Byte>(c)));
        }
        return arr;
    }

    // Global variables
    std::unordered_map<std::string, FsFunction> fs_functions;
    std::unordered_map<std::string, Value> fs_constants;
    bool fs_functions_initialized = false;
    int current_file_handle = -1;

    Value fs_open(const Value& v) {
        if (v.get_type() != ValueType::String) return Value(-1);
        std::string filename = Linh::get<std::string>(v);
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
        if (current_file_handle == -1) return Value::make_array();
        FileManager& fm = FileManager::instance();
        if (!fm.is_file_open(current_file_handle)) return Value::make_array();
        std::vector<std::string> lines = fm.read_lines(current_file_handle);
        Value array = Value::make_array();
        auto& arr_ref = array.as_array_ref();
        for (const auto& line : lines) arr_ref.push_back(Value(line));
        return Value(array);
    }

    Value fs_write(const Value& v) {
        if (current_file_handle == -1) return Value(false);
        FileManager& fm = FileManager::instance();
        if (!fm.is_file_open(current_file_handle)) return Value(false);
        if (v.get_type() != ValueType::String) return Value(false);
        std::string s = Linh::get<std::string>(v);
        bool ok = fm.write_file(current_file_handle, s);
        return Value(ok);
    }

    Value fs_append(const Value& v) {
        if (current_file_handle == -1) return Value(false);
        FileManager& fm = FileManager::instance();
        if (!fm.is_file_open(current_file_handle)) return Value(false);
        if (v.get_type() != ValueType::String) return Value(false);
        std::string s = Linh::get<std::string>(v);
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
        std::string encoding = Linh::holds_alternative<std::string>(v) ? Linh::get<std::string>(v) : "utf-8";
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
        if (Linh::holds_alternative<double>(v)) {
            offset = static_cast<long>(Linh::get<double>(v));
        } else if (Linh::holds_alternative<int64_t>(v)) {
            offset = static_cast<long>(Linh::get<int64_t>(v));
        } else if (Linh::holds_alternative<uint64_t>(v)) {
            offset = static_cast<long>(Linh::get<uint64_t>(v));
        }
        bool success = fm.seek_file(current_file_handle, offset);
        return Value(success);
    }

    Value fs_size(const Value& v) {
        std::string filename = "";
        if (Linh::holds_alternative<std::string>(v)) {
            filename = Linh::get<std::string>(v);
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
        if (Linh::holds_alternative<std::string>(v)) {
            path = Linh::get<std::string>(v);
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
        if (!v.is_array()) return Value(false);
        const auto& data = v.as_array_ref();
        FileManager& fm = FileManager::instance();
        bool ok = fm.bWrite_file(current_file_handle, v);
        return Value(ok);
    }

    Value fs_bAppend(const Value& v) {
        if (current_file_handle == -1) return Value(false);
        if (!v.is_array()) return Value(false);
        const auto& data = v.as_array_ref();
        FileManager& fm = FileManager::instance();
        bool ok = fm.bAppend_file(current_file_handle, v);
        return Value(ok);
    }

    Value fs_bRead(const Value& v) {
        if (current_file_handle == -1) return Value{}; // Return nil
        FileManager& fm = FileManager::instance();
        Value data = fm.bRead_file(current_file_handle);
        return Value(data);
    }

    Value fs_listdir(const Value& v) {
        if (!Linh::holds_alternative<std::string>(v)) return Value::make_array();
        std::string path = Linh::get<std::string>(v);
        if (path.empty()) return Value::make_array();
        
        Value array = Value::make_array();
        try {
            std::filesystem::path fs_path(path);
            if (!std::filesystem::exists(fs_path) || !std::filesystem::is_directory(fs_path)) {
                return Value(array);
            }
            
            for (const auto& entry : std::filesystem::directory_iterator(fs_path)) {
                array.as_array_ref().push_back(Value(entry.path().filename().string()));
            }
        } catch (const std::exception&) {
            // Return empty array on error
        }
        return Value(array);
    }

    Value fs_mkdir(const Value& v) {
        if (!Linh::holds_alternative<std::string>(v)) return Value(false);
        std::string path = Linh::get<std::string>(v);
        if (path.empty()) return Value(false);
        
        try {
            std::filesystem::path fs_path(path);
            bool success = std::filesystem::create_directories(fs_path);
            return Value(success || std::filesystem::exists(fs_path));
        } catch (const std::exception&) {
            return Value(false);
        }
    }

    Value fs_remove(const Value& v) {
        if (!Linh::holds_alternative<std::string>(v)) return Value(false);
        std::string path = Linh::get<std::string>(v);
        if (path.empty()) return Value(false);
        
        try {
            std::filesystem::path fs_path(path);
            bool success = std::filesystem::remove(fs_path);
            return Value(success);
        } catch (const std::exception&) {
            return Value(false);
        }
    }

    Value fs_rmdir(const Value& v) {
        if (!Linh::holds_alternative<std::string>(v)) return Value(false);
        std::string path = Linh::get<std::string>(v);
        if (path.empty()) return Value(false);
        
        try {
            std::filesystem::path fs_path(path);
            if (!std::filesystem::exists(fs_path) || !std::filesystem::is_directory(fs_path)) {
                return Value(false);
            }
            bool success = std::filesystem::remove_all(fs_path) > 0;
            return Value(success);
        } catch (const std::exception&) {
            return Value(false);
        }
    }

    Value fs_rename(const Value& v) {
        // fs_rename expects the second argument (dst) to be passed as v
        // and the first argument (src) should be on the stack
        // But since VM only passes one argument, we need to handle this differently
        // For now, let's expect an array with 2 elements as a workaround
        if (v.is_array()) {
            const auto& array = v.as_array_ref();
            if (array.size() != 2) return Value(false);
            
            if (!Linh::holds_alternative<std::string>(array[0]) || 
                !Linh::holds_alternative<std::string>(array[1])) {
                return Value(false);
            }
            
            std::string src = Linh::get<std::string>(array[0]);
            std::string dst = Linh::get<std::string>(array[1]);
            
            if (src.empty() || dst.empty()) return Value(false);
            
            try {
                std::filesystem::path src_path(src);
                std::filesystem::path dst_path(dst);
                std::filesystem::rename(src_path, dst_path);
                return Value(true);
            } catch (const std::exception&) {
                return Value(false);
            }
        }
        
        // If not an array, treat as dst and expect src to be handled by VM
        if (!Linh::holds_alternative<std::string>(v)) return Value(false);
        std::string dst = Linh::get<std::string>(v);
        
        // This is a temporary solution - we need VM to handle multiple args properly
        return Value(false);
    }

    Value fs_copy(const Value& v) {
        if (v.is_array()) {
            const auto& array = v.as_array_ref();
            if (array.size() != 2) return Value(false);
            
            if (!Linh::holds_alternative<std::string>(array[0]) || 
                !Linh::holds_alternative<std::string>(array[1])) {
                return Value(false);
            }
            
            std::string src = Linh::get<std::string>(array[0]);
            std::string dst = Linh::get<std::string>(array[1]);
            
            if (src.empty() || dst.empty()) return Value(false);
            
            try {
                std::filesystem::path src_path(src);
                std::filesystem::path dst_path(dst);
                
                if (!std::filesystem::exists(src_path)) return Value(false);
                
                std::filesystem::copy(src_path, dst_path, 
                    std::filesystem::copy_options::recursive | 
                    std::filesystem::copy_options::overwrite_existing);
                return Value(true);
            } catch (const std::exception&) {
                return Value(false);
            }
        }
        return Value(false);
    }

    Value fs_move(const Value& v) {
        if (v.is_array()) {
            const auto& array = v.as_array_ref();
            if (array.size() != 2) return Value(false);
            
            if (!Linh::holds_alternative<std::string>(array[0]) || 
                !Linh::holds_alternative<std::string>(array[1])) {
                return Value(false);
            }
            
            std::string src = Linh::get<std::string>(array[0]);
            std::string dst = Linh::get<std::string>(array[1]);
            
            if (src.empty() || dst.empty()) return Value(false);
            
            try {
                std::filesystem::path src_path(src);
                std::filesystem::path dst_path(dst);
                
                if (!std::filesystem::exists(src_path)) return Value(false);
                
                // Move is essentially copy + remove source
                std::filesystem::rename(src_path, dst_path);
                return Value(true);
            } catch (const std::exception&) {
                return Value(false);
            }
        }
        return Value(false);
    }

    Value fs_stat(const Value& v) {
        if (!Linh::holds_alternative<std::string>(v)) return Value::make_map();
        std::string path = Linh::get<std::string>(v);
        if (path.empty()) return Value::make_map();
        
        Value stat_map = Value::make_map();
        try {
            std::filesystem::path fs_path(path);
            if (!std::filesystem::exists(fs_path)) {
                return Value(stat_map);
            }
            
            auto status = std::filesystem::status(fs_path);
            auto file_size = std::filesystem::is_regular_file(fs_path) ? 
                std::filesystem::file_size(fs_path) : 0;
            auto last_write = std::filesystem::last_write_time(fs_path);
            
            // Convert to time_t for easier handling
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                last_write - std::filesystem::file_time_type::clock::now() + 
                std::chrono::system_clock::now());
            auto time_t_value = std::chrono::system_clock::to_time_t(sctp);
            
            stat_map.as_map_ref()["size"] = Value(static_cast<double>(file_size));
            stat_map.as_map_ref()["is_file"] = Value(std::filesystem::is_regular_file(fs_path));
            stat_map.as_map_ref()["is_directory"] = Value(std::filesystem::is_directory(fs_path));
            stat_map.as_map_ref()["last_modified"] = Value(static_cast<double>(time_t_value));
            
        } catch (const std::exception&) {
            // Return empty map on error
        }
        return Value(stat_map);
    }

    Value fs_isdir(const Value& v) {
        if (!Linh::holds_alternative<std::string>(v)) return Value(false);
        std::string path = Linh::get<std::string>(v);
        if (path.empty()) return Value(false);
        
        try {
            std::filesystem::path fs_path(path);
            bool is_dir = std::filesystem::exists(fs_path) && 
                         std::filesystem::is_directory(fs_path);
            return Value(is_dir);
        } catch (const std::exception&) {
            return Value(false);
        }
    }

    Value fs_isfile(const Value& v) {
        if (!Linh::holds_alternative<std::string>(v)) return Value(false);
        std::string path = Linh::get<std::string>(v);
        if (path.empty()) return Value(false);
        
        try {
            std::filesystem::path fs_path(path);
            bool is_file = std::filesystem::exists(fs_path) && 
                          std::filesystem::is_regular_file(fs_path);
            return Value(is_file);
        } catch (const std::exception&) {
            return Value(false);
        }
    }

    Value fs_isempty(const Value& v) {
        if (!Linh::holds_alternative<std::string>(v)) return Value(false);
        std::string path = Linh::get<std::string>(v);
        if (path.empty()) return Value(false);
        
        try {
            std::filesystem::path fs_path(path);
            if (!std::filesystem::exists(fs_path)) return Value(false);
            
            if (std::filesystem::is_directory(fs_path)) {
                bool is_empty = std::filesystem::is_empty(fs_path);
                return Value(is_empty);
            } else if (std::filesystem::is_regular_file(fs_path)) {
                auto size = std::filesystem::file_size(fs_path);
                return Value(size == 0);
            }
            return Value(false);
        } catch (const std::exception&) {
            return Value(false);
        }
    }

    Value fs_isopen(const Value& v) {
        // Check if the current file handle is open
        if (current_file_handle == -1) return Value(false);
        FileManager& fm = FileManager::instance();
        return Value(fm.is_file_open(current_file_handle));
    }

    Value fs_rmall(const Value& v) {
        if (!Linh::holds_alternative<std::string>(v)) return Value(false);
        std::string path = Linh::get<std::string>(v);
        if (path.empty()) return Value(false);
        
        try {
            std::filesystem::path fs_path(path);
            if (!std::filesystem::exists(fs_path)) return Value(false);
            
            // Remove directory and all its contents recursively
            std::uintmax_t removed_count = std::filesystem::remove_all(fs_path);
            return Value(removed_count > 0);
        } catch (const std::exception&) {
            return Value(false);
        }
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
        // Directory and file management functions
        fs_functions["listdir"] = fs_listdir;
        fs_functions["mkdir"] = fs_mkdir;
        fs_functions["remove"] = fs_remove;
        fs_functions["rmdir"] = fs_rmdir;
        fs_functions["rename"] = fs_rename;
        fs_functions["copy"] = fs_copy;
        fs_functions["move"] = fs_move;
        fs_functions["stat"] = fs_stat;
        fs_functions["isdir"] = fs_isdir;
        fs_functions["isfile"] = fs_isfile;
        fs_functions["isempty"] = fs_isempty;
        fs_functions["isopen"] = fs_isopen;
        fs_functions["rmall"] = fs_rmall;
        fs_functions_initialized = true;
    }

    FsFunction get_fs_function(const std::string& function_name) {
        if (!fs_functions_initialized) initialize_fs_functions();
        auto it = fs_functions.find(function_name);
        if (it != fs_functions.end()) return it->second;
        return nullptr;
    }

    std::unordered_map<std::string, FsFunction> get_fs_function_map() {
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

} // namespace Std
} // namespace Linh
