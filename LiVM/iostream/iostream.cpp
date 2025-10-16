#include <cstdio>
#include "LiVM/Variable/type.hpp"

namespace LinhIO
{

    void linh_print(const Linh::Value &val)
    {
        printf("%s\n", Linh::to_str(val).c_str());
        // print là print thông minh nên có xuống hàm tự động
    }

    void linh_printil(const Linh::Value &val)
    {
        puts(Linh::to_str(val).c_str());
        // pust có hiệu suất cao hơn
    }

    std::string linh_input(const std::string &prompt)
    {
        if (!prompt.empty())
            puts(prompt.c_str());
        std::string input_val;
        std::getline(std::cin, input_val);
        return input_val;
    }

}
