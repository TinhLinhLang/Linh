#pragma once
#include <string>
#include <array>

// Chỉ khai báo, không khởi tạo!
extern std::string name;
extern std::string engine;
extern std::string version;
extern std::string version_number;
extern std::string language;

extern std::string author;
extern std::string copyright;
extern std::string web;
extern std::string email;
extern std::string github;

extern std::string sao_tin_develop_team;
extern std::string sao_tin_develop_team_web;
extern std::string sao_tin_develop_team_email;

extern int number_bit_min;
extern int number_bit_default;
extern int number_bit_max;
extern const std::array<int, 4> int_and_uint_bit_options;
extern const std::array<int, 2> float_bit_options;
extern std::string default_utf;
extern const std::array<std::string, 5> sp_os;
extern const std::array<std::string, 10> linh_types;
extern const std::array<std::string, 3> linh_packages; // Số 3 cho math, time và package thứ 3