#pragma once
#include <string>
#include "LiVM/Variable/type.hpp"
#include "LiVM/Variable/Value.hpp"
#include <cstdio>

namespace LinhIO
{
    void linh_print(const Linh::Value &val);
    std::string linh_input(const std::string &prompt);
    void linh_printil(const Linh::Value &val); // Thêm hàm printil không tự động xuống dòng
}
