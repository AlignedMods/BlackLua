#pragma once

#include "aria/context.hpp"

namespace Aria::Internal {

    void bl__array__init__(Context* ctx);
    void bl__array__copy__(Context* ctx);
    void bl__array__destruct__(Context* ctx);

    void bl__array__append__(Context* ctx);
    void bl__array__index__(Context* ctx);

} // namespace Aria::Internal
