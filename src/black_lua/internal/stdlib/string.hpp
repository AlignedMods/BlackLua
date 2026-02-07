#pragma once

#include "context.hpp"

namespace BlackLua::Internal {

    void bl__string__init__(Context* ctx);
    void bl__string__copy__(Context* ctx);
    void bl__string__destruct__(Context* ctx);

    void bl__string__construct_from_literal__(Context* ctx);

} // namespace BlackLua::Internal