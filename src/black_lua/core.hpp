#pragma once

#include <iostream>
#include <format>

#define BLUA_FORMAT_PRINT(fmt, ...) std::cout << std::format(fmt, __VA_ARGS__) << '\n'
#define BLUA_FORMAT_ERROR(fmt, ...) std::cerr << std::format(fmt, __VA_ARGS__) << '\n'

#ifndef BLUA_ASSERT
    #define BLUA_ASSERT(condition, error) do { if (!(condition)) { BLUA_FORMAT_ERROR("Assertion failed at {}, line: {}!\nError: {}", __FILE__, __LINE__, error); __debugbreak(); abort(); } } while(0)
#endif