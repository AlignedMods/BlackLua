#pragma once

#include "core.hpp"
#include "allocator.hpp"
#include "internal/compiler/variable_type.hpp"

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
        FunctionImpl,
    
        While,
        DoWhile,
        For,
    
        If,
    
        Return,
    
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
    
        inline void Append(Node* node) {
            if (Capacity == 0) {
                Items = reinterpret_cast<Node**>(GetAllocator()->Allocate(sizeof(Node*) * 1));
                Capacity = 1;
            }
    
            if (Size >= Capacity) {
                Capacity *= 2;
    
                Node** newItems = reinterpret_cast<Node**>(GetAllocator()->Allocate(sizeof(Node*) * Capacity));
                memcpy(newItems, Items, sizeof(Node*) * Size);
                Items = newItems;
            }
    
            Items[Size] = node;
            Size++;
        }
    };
    
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
        std::string_view String; // NOTE: This **should** be fine, if there is ever any issues with strings PLEASE check this first! (always make references to tokens!)
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
        std::string_view Identifier;
        std::string Type;
    
        Node* Value = nullptr;
    
        VariableType* ResolvedType = nullptr;
    };

    struct NodeParamDecl {
        std::string_view Identifier;
        std::string Type;

        VariableType* ResolvedType = nullptr;
    };
    
    struct NodeVarRef {
        std::string_view Identifier;
    
        VariableType* ResolvedType = nullptr;
    };
    
    struct NodeStructDecl {
        std::string_view Identifier;
    
        NodeList Fields;
    };
    
    struct NodeFieldDecl {
        std::string_view Identifier;
        std::string Type;
    };

    struct NodeMethodDecl {
        std::string_view Name;
        std::string Signature;
    
        NodeList Parameters;
        std::string ReturnType;

        Node* Body = nullptr;
    
        VariableType* ResolvedType = nullptr;
    };

    struct NodeFunctionDecl {
        std::string_view Name;
        std::string Signature;
    
        NodeList Parameters;
        std::string ReturnType;
    
        bool Extern = false;
    
        VariableType* ResolvedType = nullptr;
    };
    
    struct NodeFunctionImpl {
        std::string_view Name;
    
        NodeList Parameters;
        std::string ReturnType;
    
        Node* Body = nullptr; // Type is always NodeScope
    
        VariableType* ResolvedType = nullptr;
    };
    
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
    
    struct NodeArrayAccessExpr {
        Node* Parent = nullptr;
        Node* Index = nullptr;
    
        VariableType* ResolvedType = nullptr;
    };
    
    struct NodeMemberExpr {
        Node* Parent = nullptr;
        std::string_view Member;
    
        VariableType* ResolvedParentType = nullptr;
        VariableType* ResolvedMemberType = nullptr;
    };

    struct NodeMethodCallExpr {
        Node* Parent = nullptr;
        std::string_view Member;

        NodeList Arguments;

        VariableType* ResolvedParentType = nullptr;
        VariableType* ResolvedMemberType = nullptr;
    };
    
    struct NodeFunctionCallExpr {
        std::string_view Name;
        NodeList Arguments;
    };
    
    struct NodeParenExpr {
        Node* Expression = nullptr;
    };
    
    struct NodeCastExpr {
        std::string Type;
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
    };
    
    struct Node {
        NodeType Type = NodeType::Bool;
        std::variant<NodeConstant*, 
                     NodeScope*,
                     NodeVarDecl*, NodeParamDecl*, NodeVarRef*,
                     NodeStructDecl*, NodeFieldDecl*, NodeMethodDecl*,
                     NodeFunctionDecl*, NodeFunctionImpl*,
                     NodeWhile*, NodeDoWhile*, NodeFor*,
                     NodeIf*,
                     NodeReturn*,
                     NodeArrayAccessExpr*, NodeMemberExpr*, NodeMethodCallExpr*, NodeFunctionCallExpr*, NodeParenExpr*, NodeCastExpr*, NodeUnaryExpr*, NodeBinExpr*> Data;

        size_t Line = 0;
        size_t Column = 0;
    };

    using ASTNodes = std::vector<Node*>;

    template <typename T>
    inline T* GetNode(Node* n) {
        return std::get<T*>(n->Data);
    }

} // namespace BlackLua::Internal