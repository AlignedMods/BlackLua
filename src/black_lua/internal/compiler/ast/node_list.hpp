#pragma once

#include "internal/compiler/ast/node.hpp"

namespace BlackLua {
    struct Context;
} // namespace BlackLua

namespace BlackLua::Internal {

    struct NodeList {
        Node** Items = nullptr;
        size_t Capacity = 0;
        size_t Size = 0;
    
        void Append(Context* ctx, Node* n);
    };

} // namespace BlackLua::Internal