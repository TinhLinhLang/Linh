#include <cstdio>
#include "LiVM/Variable/type.hpp"

namespace LinhIO
{

    void linh_print(const Linh::Value &val)
    {
        puts(Linh::to_str(val).c_str());
        // print là print thông minh nên có xuống hàm tự động
    }

    void linh_printil(const Linh::Value &val)
    {
        fputs(Linh::to_str(val).c_str(), stdout);
        // printil không tự động xuống hàng
    }

    std::string linh_input(const std::string &prompt)
    {
        if (!prompt.empty())
            fputs(prompt.c_str(), stdout);
        std::string input_val;
        std::getline(std::cin, input_val);
        return input_val;
    }

}
