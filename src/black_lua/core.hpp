#pragma once

#define FMT_UNICODE 0
#include "fmt/printf.h"
#include "fmt/color.h"

#ifndef BLUA_ASSERT
    #define BLUA_ASSERT(condition, error) do { if (!(condition)) { fmt::print(stderr, "Assertion failed at {}, line: {}!\nError: {}", __FILE__, __LINE__, error); __debugbreak(); abort(); } } while(0)
#endif