#pragma once

#define FMT_UNICODE 0
#include "fmt/printf.h"
#include "fmt/color.h"

#ifndef ARIA_ASSERT
    #define ARIA_ASSERT(condition, error) do { if (!(condition)) { fmt::print(stderr, "Assertion failed at {}, line: {}!\nError: {}", __FILE__, __LINE__, error); __debugbreak(); abort(); } } while(0)
#endif

#ifndef ARIA_UNREACHABLE
    #define ARIA_UNREACHABLE() ARIA_ASSERT(false, "Unreachable!")
#endif
