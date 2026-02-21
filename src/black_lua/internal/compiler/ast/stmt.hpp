#pragma once

#include "internal/compiler/ast/node_list.hpp"
#include "internal/compiler/core/string_view.hpp"
#include "internal/compiler/core/string_builder.hpp"
#include "internal/compiler/variable_type.hpp"
#include "internal/compiler/core/source_location.hpp"

#include <variant>

namespace BlackLua::Internal {

    struct NodeStmt;

    struct StmtCompound {
        NodeList Nodes{};
    };
    
    struct StmtVarDecl {
        StringView Identifier;
        StringBuilder Type;
    
        NodeExpr* Value = nullptr;
    
        VariableType* ResolvedType = nullptr;
    };

    struct StmtParamDecl {
        StringView Identifier;
        StringBuilder Type;

        VariableType* ResolvedType = nullptr;
    };

    struct StmtFunctionDecl {
        StringView Name;
        StringBuilder Signature;
    
        NodeList Parameters;
        StringBuilder ReturnType;
    
        bool Extern = false;
    
        NodeStmt* Body = nullptr;

        VariableType* ResolvedType = nullptr;
    };
    
    struct StmtStructDecl {
        StringView Identifier;
    
        NodeList Fields;
    };
    
    struct StmtFieldDecl {
        StringView Identifier;
        StringBuilder Type;
    };

    struct StmtMethodDecl {
        StringView Name;
        StringBuilder Signature;
    
        NodeList Parameters;
        StringBuilder ReturnType;

        NodeStmt* Body = nullptr;
    
        VariableType* ResolvedType = nullptr;
    };

    struct StmtWhile {
        NodeExpr* Condition = nullptr;
        NodeStmt* Body = nullptr;
    };
    
    struct StmtDoWhile {
        NodeExpr* Condition = nullptr;
        NodeStmt* Body = nullptr;
    };
    
    struct StmtFor {
        NodeStmt* Prologue = nullptr; // int i = 0;
        NodeExpr* Condition = nullptr; // i < 5;
        NodeExpr* Epilogue = nullptr; // i += 1;
        NodeStmt* Body = nullptr;
    };
    
    struct StmtIf {
        NodeExpr* Condition = nullptr;
        NodeStmt* Body = nullptr;
        NodeStmt* ElseBody = nullptr;
    };
    
    struct StmtReturn {
        NodeExpr* Value = nullptr;
    };

    struct NodeStmt {
        std::variant<StmtCompound*,
                     StmtVarDecl*, StmtParamDecl*, StmtFunctionDecl*,
                     StmtStructDecl*, StmtFieldDecl*, StmtMethodDecl*,
                     StmtWhile*, StmtDoWhile*, StmtFor*, StmtIf*,
                     StmtReturn*,
                     std::nullptr_t> Data;

        SourceRange Range;
        SourceLocation Loc;
    };

    template <typename T>
    inline T* GetNode(NodeStmt* n) {
        T** result = std::get_if<T*>(&n->Data);
        if (result == nullptr) { return nullptr; }
        return *result;
    }

} // namespace BlackLua::Internal