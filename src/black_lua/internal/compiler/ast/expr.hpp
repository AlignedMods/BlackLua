#pragma once

#include "internal/compiler/ast/node_list.hpp"
#include "internal/compiler/core/string_view.hpp"
#include "internal/compiler/type_info.hpp"
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
        FloatingToIntegral,

        LValueToRValue
    };

    inline const char* CastTypeToString(CastType type) {
        switch (type) {
            case CastType::Integral: return "Integral";
            case CastType::Floating: return "Floating";
            case CastType::IntegralToFloating: return "IntegralToFloating";
            case CastType::FloatingToIntegral: return "FloatingToIntegral";
            case CastType::LValueToRValue: return "LValueToRValue";
            default: BLUA_ASSERT(false, "Unreachable");
        }
    }

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
        Xor, XorInPlace,
    
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
    
            case BinaryOperatorType::Eq: return "=";
            case BinaryOperatorType::IsEq: return "==";
            case BinaryOperatorType::IsNotEq: return "!=";
        }
    
        BLUA_ASSERT(false, "Unreachable!");
        return "invalid";
    }

#pragma endregion

#pragma region ExprValueType

    enum class ExprValueType {
        LValue,
        RValue
    };

#pragma endregion
    
    struct ExprConstant {
        std::variant<ConstantBool*, ConstantChar*, ConstantInt*, ConstantLong*, ConstantFloat*, ConstantDouble*, ConstantString*> Data;
        TypeInfo* ResolvedType = nullptr;
    };

    struct ExprVarRef {
        StringView Identifier;
    
        TypeInfo* ResolvedType = nullptr;
    };

    struct ExprArrayAccess {
        NodeExpr* Parent = nullptr;
        NodeExpr* Index = nullptr;
    
        TypeInfo* ResolvedType = nullptr;
    };
    
    struct ExprSelf {};

    struct ExprMember {
        NodeExpr* Parent = nullptr;
        StringView Member;
    
        TypeInfo* ResolvedParentType = nullptr;
        TypeInfo* ResolvedMemberType = nullptr;
    };

    struct ExprMethodCall {
        NodeExpr* Parent = nullptr;
        StringView Member;

        NodeList Arguments;

        TypeInfo* ResolvedParentType = nullptr;
        TypeInfo* ResolvedMemberType = nullptr;
    };
    
    struct ExprCall {
        StringView Name;
        NodeList Arguments;

        bool Extern = false;

        TypeInfo* ResolvedReturnType = nullptr;
    };
    
    struct ExprParen {
        NodeExpr* Expression = nullptr;
    };
    
    // A node which represents an explicit cast in the source code, generated by the parser
    // If a cast is implicit it should use "ExprImplicitCast" instead of this
    struct ExprCast {
        StringView Type;
        NodeExpr* Expression = nullptr;

        CastType ResolvedCastType = CastType::Integral;
        TypeInfo* ResolvedSrcType = nullptr;
        TypeInfo* ResolvedDstType = nullptr;
    };

    // A node which is used only for implcit casts inserted by the semantic analyzer/type checker
    // This node is not valid to use outside of that use case
    struct ExprImplicitCast {
        NodeExpr* Expression = nullptr;

        CastType ResolvedCastType = CastType::Integral;
        TypeInfo* ResolvedSrcType = nullptr;
        TypeInfo* ResolvedDstType = nullptr;
    };
    
    struct ExprUnaryOperator {
        NodeExpr* Expression = nullptr;
        UnaryOperatorType Type = UnaryOperatorType::Invalid;
    
        TypeInfo* ResolvedType = nullptr;
    };
    
    struct ExprBinaryOperator {
        NodeExpr* LHS = nullptr;
        NodeExpr* RHS = nullptr;
        BinaryOperatorType Type = BinaryOperatorType::Invalid;
    
        TypeInfo* ResolvedType = nullptr;
        TypeInfo* ResolvedSourceType = nullptr;
    };

    struct NodeExpr {
        std::variant<ExprConstant*,
                     ExprVarRef*,
                     ExprArrayAccess*, ExprSelf*, ExprMember*, 
                     ExprMethodCall*, ExprCall*, 
                     ExprParen*,
                     ExprCast*, ExprImplicitCast*,
                     ExprUnaryOperator*, ExprBinaryOperator*> Data;

        SourceRange Range;
        SourceLocation Loc;

        ExprValueType Type = ExprValueType::RValue;

        bool IsLValue() const { return Type == ExprValueType::LValue; }
        bool IsRValue() const { return Type == ExprValueType::RValue; }
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