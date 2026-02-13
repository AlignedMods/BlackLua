#pragma once

#include "core.hpp"
#include "allocator.hpp"
#include "internal/compiler/variable_type.hpp"
#include "internal/compiler/core/string_view.hpp"
#include "internal/compiler/core/string_builder.hpp"

namespace BlackLua {
    struct Context;
} // namespace BlackLua

namespace BlackLua::Internal {

    enum class NodeType {
        Bool,
        Char,
        Short,
        Int,
        Long,
        Float,
        Double,
        String,
        InitializerList,
        Constant,
    
        Scope,
    
        VarDecl,
        ParamDecl,
        VarRef,
    
        StructDecl,
        FieldDecl,
        MethodDecl,
    
        FunctionDecl,
    
        While,
        DoWhile,
        For,
    
        If,
    
        Break,
        Continue,
        Return,
    
        StringConstructExpr,
        StringConstructLiteralExpr,
        StringCopyConstructExpr,
        ArrayAccessExpr,
        MemberExpr,
        MethodCallExpr,
        FunctionCallExpr,
        ParenExpr,
        CastExpr,
        UnaryExpr,
        BinExpr
    };
    
    enum class UnaryExprType {
        Invalid,
    
        Not, // "!"
        Negate // "-8.7f"
    };
    
    inline const char* UnaryExprTypeToString(UnaryExprType type) {
        switch (type) {
            case UnaryExprType::Invalid: return "invalid";
            
            case UnaryExprType::Not: return "!";
            case UnaryExprType::Negate: return "-";
        }
    
        BLUA_ASSERT(false, "Unreachable!");
        return "invalid";
    }
    
    enum class BinExprType {
        Invalid,
        Add, AddInPlace, AddOne,
        Sub, SubInPlace, SubOne,
        Mul, MulInPlace,
        Div, DivInPlace,
        Mod, ModInPlace,
    
        Less,
        LessOrEq,
        Greater,
        GreaterOrEq,
    
        Eq,
        IsEq,
        IsNotEq,
    };
    
    inline const char* BinExprTypeToString(BinExprType type) {
        switch (type) {
            case BinExprType::Invalid: return "invalid";
            case BinExprType::Add: return "+";
            case BinExprType::AddInPlace: return "+=";
            case BinExprType::AddOne: return "++";
            case BinExprType::Sub: return "-";
            case BinExprType::SubInPlace: return "-=";
            case BinExprType::SubOne: return "--";
            case BinExprType::Mul: return "*";
            case BinExprType::MulInPlace: return "*=";
            case BinExprType::Div: return "/";
            case BinExprType::DivInPlace: return "/=";
            case BinExprType::Mod: return "%";
            case BinExprType::ModInPlace: return "%=";
    
            case BinExprType::Less: return "<";
            case BinExprType::LessOrEq: return "<=";
            case BinExprType::Greater: return ">";
            case BinExprType::GreaterOrEq: return ">=";
    
            case BinExprType::Eq: return "=";
            case BinExprType::IsEq: return "==";
            case BinExprType::IsNotEq: return "!=";
        }
    
        BLUA_ASSERT(false, "Unreachable!");
        return "invalid";
    }
    
    struct Node; // Forward declaration of Node
    
    struct NodeList {
        Node** Items = nullptr;
        size_t Capacity = 0;
        size_t Size = 0;
    
        void Append(Context* ctx, Node* n);
    };

    // For nodes that have no data tied to them
    struct NodeEmpty {};
    
    struct NodeBool {
        bool Value = false;
    };
    
    struct NodeChar {
        int8_t Char = 0;
    };
    
    struct NodeInt {
        int32_t Int = 0;
        bool Unsigned = false;
    };
    
    struct NodeLong {
        int64_t Long = 0;
        bool Unsigned = false;
    };
    
    struct NodeFloat {
        float Float = 0.0f;
    };
    
    struct NodeDouble {
        double Double = 0.0;
    };
    
    struct NodeString {
        StringView String; // NOTE: This **should** be fine, if there is ever any issues with strings PLEASE check this first! (always make references to tokens!)
    };
    
    struct NodeInitializerList {
        NodeList Nodes{};
    };
    
    struct NodeConstant {
        NodeType Type = NodeType::Bool;
        std::variant<NodeBool*, NodeChar*, NodeInt*, NodeLong*, NodeFloat*, NodeDouble*, NodeString*, NodeInitializerList*> Data;
    
        VariableType* ResolvedType = nullptr;
    };
    
    struct NodeScope {
        NodeList Nodes{};
    };
    
    struct NodeVarDecl {
        StringView Identifier;
        StringBuilder Type;
    
        Node* Value = nullptr;
    
        VariableType* ResolvedType = nullptr;
    };

    struct NodeParamDecl {
        StringView Identifier;
        StringBuilder Type;

        VariableType* ResolvedType = nullptr;
    };
    
    struct NodeVarRef {
        StringView Identifier;
    
        VariableType* ResolvedType = nullptr;
    };
    
    struct NodeStructDecl {
        StringView Identifier;
    
        NodeList Fields;
    };
    
    struct NodeFieldDecl {
        StringView Identifier;
        StringBuilder Type;
    };

    struct NodeMethodDecl {
        StringView Name;
        StringBuilder Signature;
    
        NodeList Parameters;
        StringBuilder ReturnType;

        Node* Body = nullptr;
    
        VariableType* ResolvedType = nullptr;
    };

    struct NodeFunctionDecl {
        StringView Name;
        StringBuilder Signature;
    
        NodeList Parameters;
        StringBuilder ReturnType;
    
        bool Extern = false;
    
        VariableType* ResolvedType = nullptr;

        Node* Body = nullptr; // Type is always NodeScope
    };\

    struct NodeWhile {
        Node* Condition = nullptr;
        Node* Body = nullptr; // Type is always NodeScope
    };
    
    struct NodeDoWhile {
        Node* Body = nullptr; // Type is always NodeScope
        Node* Condition = nullptr;
    };
    
    struct NodeFor {
        Node* Prologue = nullptr; // int i = 0;
        Node* Condition = nullptr; // i < 5;
        Node* Epilogue = nullptr; // i += 1;
        Node* Body = nullptr;
    };
    
    struct NodeIf {
        Node* Condition = nullptr;
        Node* Body = nullptr;
        Node* ElseBody = nullptr;
    };
    
    struct NodeReturn {
        Node* Value = nullptr;
    };
    
    struct NodeStringConstructLiteralExpr {
        Node* Literal = nullptr;
    };

    struct NodeStringCopyConstructExpr {
        Node* Source = nullptr;
    };

    struct NodeArrayAccessExpr {
        Node* Parent = nullptr;
        Node* Index = nullptr;
    
        VariableType* ResolvedType = nullptr;
    };
    
    struct NodeMemberExpr {
        Node* Parent = nullptr;
        StringView Member;
    
        VariableType* ResolvedParentType = nullptr;
        VariableType* ResolvedMemberType = nullptr;
    };

    struct NodeMethodCallExpr {
        Node* Parent = nullptr;
        StringView Member;

        NodeList Arguments;

        VariableType* ResolvedParentType = nullptr;
        VariableType* ResolvedMemberType = nullptr;
    };
    
    struct NodeFunctionCallExpr {
        StringView Name;
        NodeList Arguments;
    };
    
    struct NodeParenExpr {
        Node* Expression = nullptr;
    };
    
    struct NodeCastExpr {
        StringView Type;
        Node* Expression = nullptr;
    
        VariableType* ResolvedSrcType = nullptr;
        VariableType* ResolvedDstType = nullptr;
    };
    
    struct NodeUnaryExpr {
        Node* Expression = nullptr;
        UnaryExprType Type = UnaryExprType::Invalid;
    
        VariableType* ResolvedType = nullptr;
    };
    
    struct NodeBinExpr {
        Node* LHS = nullptr;
        Node* RHS = nullptr;
        BinExprType Type = BinExprType::Invalid;
    
        VariableType* ResolvedType = nullptr;
        VariableType* ResolvedSourceType = nullptr;
    };
    
    struct Node {
        NodeType Type = NodeType::Bool;
        std::variant<NodeEmpty*,
                     NodeConstant*, 
                     NodeScope*,
                     NodeVarDecl*, NodeParamDecl*, NodeVarRef*,
                     NodeStructDecl*, NodeFieldDecl*, NodeMethodDecl*,
                     NodeFunctionDecl*,
                     NodeWhile*, NodeDoWhile*, NodeFor*,
                     NodeIf*,
                     NodeReturn*,
                     NodeStringConstructLiteralExpr*, NodeStringCopyConstructExpr*, NodeArrayAccessExpr*, NodeMemberExpr*, NodeMethodCallExpr*, NodeFunctionCallExpr*, NodeParenExpr*, NodeCastExpr*, NodeUnaryExpr*, NodeBinExpr*> Data;

        size_t Line = 0;
        size_t Column = 0;
    };

    using ASTNodes = std::vector<Node*>;

    template <typename T>
    inline T* GetNode(Node* n) {
        return std::get<T*>(n->Data);
    }

} // namespace BlackLua::Internal