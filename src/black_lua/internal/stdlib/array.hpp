#pragma once

#include "context.hpp"

namespace BlackLua::Internal {

    void bl__array__init__(Context* ctx);
    void bl__array__copy__(Context* ctx);
    void bl__array__destruct__(Context* ctx);
    void bl__array__index__(Context* ctx);

} // namespace BlackLua::Internal