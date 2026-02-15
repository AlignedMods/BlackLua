#pragma once

#include "core.hpp"

#include <variant>
#include <vector>

namespace BlackLua::Internal {

    struct NodeExpr;
    struct NodeStmt;

    struct Node {
        std::variant<NodeExpr*, NodeStmt*> Data;
    };

    template <typename T>
    inline T* GetNode(Node* n) {
        T** result = std::get_if<T*>(&n->Data);
        if (result == nullptr) { return nullptr; }
        return *result;
    }

    using ASTNodes = std::vector<Node*>;

} // namespace BlackLua::Internal