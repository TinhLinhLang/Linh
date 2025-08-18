#include "config.hpp"

std::string name = "Linh";
std::string engine = "Linh.cpp";
std::string version = "Beta 0.015 - File System Support and Binary Support";
std::string version_number = "0.015";
std::string language = "C++";

std::string author = "Jkar / Sao Tin Developer Team";
std::string copyright = "Copyright (c) 2025 Sao Tin Developer Team";
std::string web = "https://linh.kesug.com";
std::string email = "linhprogramminglanguage@gmail.com";
std::string github = "https://github.com/LinhProgrammingLanguage/Linh.git";

std::string sao_tin_develop_team = "Sao Tin Developer Team";
std::string sao_tin_develop_team_web = "https://saotin.kesug.com";
std::string sao_tin_develop_team_email = "saotindev@gmail.com";

int number_bit_min = 8;
int number_bit_default = 64;
int number_bit_max = 128;
const std::array<int, 4> int_and_uint_bit_options = {8, 16, 32, 64};
const std::array<int, 2> float_bit_options = {32, 64};
std::string default_utf = "utf-8";
const std::array<std::string, 5> sp_os = {
    "windows", "linux", "macos", "android", "ios"
};
const std::array<std::string, 10> linh_types = {
    "int", "uint", "float", "str", "bool",
    "array", "map", "any", "sol", "void"
}; 
const std::array<std::string, 3> linh_packages = {
    "math", "time", "fs"
};