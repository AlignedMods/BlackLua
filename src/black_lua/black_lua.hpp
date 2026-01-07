#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

#ifndef BLUA_ASSERT
    #define BLUA_ASSERT(condition, error) do { if (!(condition)) { std::cerr << "Assertion failed at: " << __LINE__ << ", " << __FILE__ << "\nError: " << error; abort(); } } while(0)
#endif