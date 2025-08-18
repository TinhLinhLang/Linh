#pragma once
#include <string>
#include "../type.hpp"
#include "../Value/Value.hpp"
#include <fmt/format.h>

namespace LinhIO
{
    void linh_print(const Linh::Value &val);
    std::string linh_input(const std::string &prompt);
    void linh_printf(const Linh::Value &val); // Thêm hàm printil không tự động xuống dòng
}
