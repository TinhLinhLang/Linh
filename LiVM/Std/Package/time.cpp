#include "time.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <ctime>

namespace Linh {
namespace Std {
    // Map chứa các hằng số thời gian (đơn vị chuẩn: millisecond)
    static std::unordered_map<std::string, double> time_constants = {
        {"nanosecond", 0.000001},    // 1 nanosecond = 0.000001 millisecond
        {"microsecond", 0.001},      // 1 microsecond = 0.001 millisecond  
        {"millisecond", 1.0},        // 1 millisecond = 1 millisecond (đơn vị chuẩn)
        {"second", 1000.0},          // 1 second = 1000 milliseconds
        {"minute", 60000.0},         // 1 minute = 60000 milliseconds
        {"hour", 3600000.0}          // 1 hour = 3600000 milliseconds
    };

    // Map chứa các hàm thời gian
    static std::unordered_map<std::string, TimeFunction> time_functions;

    // Hàm time_time (placeholder - có thể mở rộng sau)
    Value time_time(const Value& v) {
        // TODO: Implement time function
        return Value{};
    }

    // Hàm time_sleep - tạm dừng chương trình trong số millisecond được chỉ định
    Value time_sleep(const Value& v) {
        double milliseconds = 0.0;
        
        // Chuyển đổi giá trị đầu vào thành millisecond
        if (std::holds_alternative<int64_t>(v)) {
            milliseconds = static_cast<double>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            milliseconds = std::get<double>(v);
        } else if (std::holds_alternative<uint64_t>(v)) {
            milliseconds = static_cast<double>(std::get<uint64_t>(v));
        } else {
            return Value{}; // Trả về nothingnếu không phải số
        }
        
        // Kiểm tra giá trị hợp lệ
        if (milliseconds < 0) {
            return Value{}; // Không thể sleep số âm
        }
        
        // Thực hiện sleep
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(milliseconds));
        
        return Value(milliseconds); // Trả về số millisecond đã sleep
    }

    // Hàm time_process_time - đo CPU time của tiến trình hiện tại (milliseconds)
    Value time_process_time(const Value& v) {
        // Sử dụng clock() để đo CPU time
        // clock() trả về số clock ticks từ khi chương trình bắt đầu
        // CLOCKS_PER_SEC là số clock ticks trong 1 giây
        clock_t cpu_time = clock();
        double cpu_time_seconds = static_cast<double>(cpu_time) / CLOCKS_PER_SEC;
        double cpu_time_milliseconds = cpu_time_seconds * 1000.0;
        
        return Value(cpu_time_milliseconds);
    }

    // Khởi tạo các hằng số thời gian
    void initialize_time_constants() {
        // Các hằng số đã được khởi tạo trong map ở trên
        // Không cần thêm logic khởi tạo phức tạp
    }

    // Khởi tạo các hàm thời gian
    void initialize_time_functions() {
        if (!time_functions.empty()) return;
        time_functions["time"] = time_time;
        time_functions["sleep"] = time_sleep;
        time_functions["process_time"] = time_process_time;
    }

    // Lấy giá trị hằng số thời gian theo tên
    double get_time_constant(const std::string& constant_name) {
        auto it = time_constants.find(constant_name);
        if (it != time_constants.end()) {
            return it->second;
        }
        return 0.0; // Trả về 0 nếu không tìm thấy
    }

    // Lấy danh sách tên các hằng số thời gian
    std::vector<std::string> get_time_constants() {
        std::vector<std::string> names;
        for (const auto& pair : time_constants) {
            names.push_back(pair.first);
        }
        return names;
    }

    // Lấy hàm thời gian theo tên
    TimeFunction get_time_function(const std::string& function_name) {
        if (time_functions.empty()) initialize_time_functions();
        auto it = time_functions.find(function_name);
        if (it != time_functions.end()) return it->second;
        return nullptr;
    }

    // Lấy danh sách tên các hàm thời gian
    std::vector<std::string> get_time_functions() {
        if (time_functions.empty()) initialize_time_functions();
        std::vector<std::string> names;
        for (const auto& pair : time_functions) {
            names.push_back(pair.first);
        }
        return names;
    }
}
}
