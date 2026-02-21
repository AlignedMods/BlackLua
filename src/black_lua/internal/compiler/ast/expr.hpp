#pragma once

#include "internal/compiler/ast/node_list.hpp"
#include "internal/compiler/core/string_view.hpp"
#include "internal/compiler/variable_type.hpp"
#include "internal/compiler/core/source_location.hpp"

#include <variant>

namespace BlackLua::Internal {
    
    struct NodeExpr;

#pragma region Constant**

    struct ConstantBool {
        bool Value = false;
    };
    
    struct ConstantChar {
        int8_t Char = 0;
    };
    
    struct ConstantInt {
        int32_t Int = 0;
        bool Unsigned = false;
    };
    
    struct ConstantLong {
        int64_t Long = 0;
        bool Unsigned = false;
    };
    
    struct ConstantFloat {
        float Float = 0.0f;
    };
    
    struct ConstantDouble {
        double Double = 0.0;
    };
    
    struct ConstantString {
        StringView String;
    };

#pragma endregion

#pragma region CastType

    enum class CastType {
        Integral,
        Floating,
        IntegralToFloating,
        FloatingToIntegral
    };

#pragma endregion

#pragma region UnaryOperatorType

    enum class UnaryOperatorType {
        Invalid,
    
        Not, // "!"
        Negate // "-8.7f"
    };
    
    inline const char* UnaryOperatorTypeToString(UnaryOperatorType type) {
        switch (type) {
            case UnaryOperatorType::Invalid: return "invalid";
            
            case UnaryOperatorType::Not: return "!";
            case UnaryOperatorType::Negate: return "-";
        }
    
        BLUA_ASSERT(false, "Unreachable!");
        return "invalid";
    }

#pragma endregion

#pragma region BinaryOperatorType

    enum class BinaryOperatorType {
        Invalid,
        Add, AddInPlace,
        Sub, SubInPlace,
        Mul, MulInPlace,
        Div, DivInPlace,
        Mod, ModInPlace,
    
        Less,
        LessOrEq,
        Greater,
        GreaterOrEq,

        And, AndInPlace, BitAnd,
        Or, OrInPlace, BitOr,
        Xor, XorInPlace, BitXor,
    
        Eq,
        IsEq,
        IsNotEq,
    };
    
    inline const char* BinaryOperatorTypeToString(BinaryOperatorType type) {
        switch (type) {
            case BinaryOperatorType::Invalid: return "invalid";
            case BinaryOperatorType::Add: return "+";
            case BinaryOperatorType::AddInPlace: return "+=";
            case BinaryOperatorType::Sub: return "-";
            case BinaryOperatorType::SubInPlace: return "-=";
            case BinaryOperatorType::Mul: return "*";
            case BinaryOperatorType::MulInPlace: return "*=";
            case BinaryOperatorType::Div: return "/";
            case BinaryOperatorType::DivInPlace: return "/=";
            case BinaryOperatorType::Mod: return "%";
            case BinaryOperatorType::ModInPlace: return "%=";
    
            case BinaryOperatorType::Less: return "<";
            case BinaryOperatorType::LessOrEq: return "<=";
            case BinaryOperatorType::Greater: return ">";
            case BinaryOperatorType::GreaterOrEq: return ">=";

            case BinaryOperatorType::And: return "&";
            case BinaryOperatorType::AndInPlace: return "&=";
            case BinaryOperatorType::BitAnd: return "&&";
            case BinaryOperatorType::Or: return "|";
            case BinaryOperatorType::OrInPlace: return "|=";
            case BinaryOperatorType::BitOr: return "||";
            case BinaryOperatorType::Xor: return "^";
            case BinaryOperatorType::XorInPlace: return "^=";
            case BinaryOperatorType::BitXor: return "^^";
    
            case BinaryOperatorType::Eq: return "=";
            case BinaryOperatorType::IsEq: return "==";
            case BinaryOperatorType::IsNotEq: return "!=";
        }
    
        BLUA_ASSERT(false, "Unreachable!");
        return "invalid";
    }

#pragma endregion
    
    struct ExprConstant {
        std::variant<ConstantBool*, ConstantChar*, ConstantInt*, ConstantLong*, ConstantFloat*, ConstantDouble*, ConstantString*> Data;
        VariableType* ResolvedType = nullptr;
    };

    struct ExprVarRef {
        StringView Identifier;
    
        VariableType* ResolvedType = nullptr;
    };

    struct ExprArrayAccess {
        NodeExpr* Parent = nullptr;
        NodeExpr* Index = nullptr;
    
        VariableType* ResolvedType = nullptr;
    };
    
    struct ExprSelf {};

    struct ExprMember {
        NodeExpr* Parent = nullptr;
        StringView Member;
    
        VariableType* ResolvedParentType = nullptr;
        VariableType* ResolvedMemberType = nullptr;
    };

    struct ExprMethodCall {
        NodeExpr* Parent = nullptr;
        StringView Member;

        NodeList Arguments;

        VariableType* ResolvedParentType = nullptr;
        VariableType* ResolvedMemberType = nullptr;
    };
    
    struct ExprCall {
        StringView Name;
        NodeList Arguments;

        bool Extern = false;

        VariableType* ResolvedReturnType = nullptr;
    };
    
    struct ExprParen {
        NodeExpr* Expression = nullptr;
    };
    
    struct ExprCast {
        StringView Type;
        NodeExpr* Expression = nullptr;

        CastType ResolvedCastType = CastType::Integral;
        VariableType* ResolvedSrcType = nullptr;
        VariableType* ResolvedDstType = nullptr;
    };
    
    struct ExprUnaryOperator {
        NodeExpr* Expression = nullptr;
        UnaryOperatorType Type = UnaryOperatorType::Invalid;
    
        VariableType* ResolvedType = nullptr;
    };
    
    struct ExprBinaryOperator {
        NodeExpr* LHS = nullptr;
        NodeExpr* RHS = nullptr;
        BinaryOperatorType Type = BinaryOperatorType::Invalid;
    
        VariableType* ResolvedType = nullptr;
        VariableType* ResolvedSourceType = nullptr;
    };

    struct NodeExpr {
        std::variant<ExprConstant*,
                     ExprVarRef*,
                     ExprArrayAccess*, ExprSelf*, ExprMember*, 
                     ExprMethodCall*, ExprCall*, 
                     ExprParen*, ExprCast*, 
                     ExprUnaryOperator*, ExprBinaryOperator*> Data;

        SourceRange Range;
        SourceLocation Loc;
    };

    template <typename T>
    inline T* GetNode(ExprConstant* n) {
        T** result = std::get_if<T*>(&n->Data);
        if (result == nullptr) { return nullptr; }
        return *result;
    }

    template <typename T>
    inline T* GetNode(NodeExpr* n) {
        T** result = std::get_if<T*>(&n->Data);
        if (result == nullptr) { return nullptr; }
        return *result;
    }

} // namespace BlackLua::Internal