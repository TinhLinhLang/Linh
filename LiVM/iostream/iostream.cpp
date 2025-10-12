#include <fmt/format.h>
#include "LiVM/Variable/type.hpp"

namespace LinhIO
{

    void linh_print(const Linh::Value &val)
    {
        fmt::print("{}\n", Linh::to_str(val));
        // print là print thông minh nên có xuống hàm tự động
    }

    void linh_printf(const Linh::Value &val)
    {
        fmt::print("{}", Linh::to_str(val));
    }

    std::string linh_input(const std::string &prompt)
    {
        if (!prompt.empty())
            fmt::print("{}\n", prompt);
        std::string input_val;
        std::getline(std::cin, input_val);
        return input_val;
    }

}
