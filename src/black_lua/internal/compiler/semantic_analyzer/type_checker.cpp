#include "internal/compiler/semantic_analyzer/type_checker.hpp"

namespace BlackLua::Internal {

    TypeChecker::TypeChecker(Context* ctx) {
        m_Context = ctx;
    }

    VariableType* TypeChecker::GetVariableTypeFromString(StringView str) {
        size_t bracket = str.Find('[');

        std::string isolatedType;
        bool array = false;

        if (bracket != StringView::npos) {
            isolatedType = fmt::format("{}", str.SubStr(0, bracket));
            array = true;
        } else {
            isolatedType = fmt::format("{}", str);
        }

        #define TYPE(str, t, _signed) if (isolatedType == str) { type->Type = PrimitiveType::t; type->Data = _signed; }

        VariableType* type = CreateVarType(m_Context, PrimitiveType::Invalid);
        TYPE("void",   Void, true);
        TYPE("bool",   Bool, true);
        TYPE("char",   Char, true);
        TYPE("uchar",  Char, false);
        TYPE("short",  Short, true);
        TYPE("ushort", Short, false);
        TYPE("int",    Int, true);
        TYPE("uint",   Int, false);
        TYPE("long",   Long, true);
        TYPE("ulong",  Long, false);
        TYPE("float",  Float, true);
        TYPE("double", Double, true);
        TYPE("string", String, false);

        #undef TYPE

        // Handle user defined type
        if (type->Type == PrimitiveType::Invalid) {
            BLUA_ASSERT(false, "todo");
            // if (m_DeclaredStructs.contains(isolatedType)) {
            //     type->Type = PrimitiveType::Structure;
            //     type->Data = m_DeclaredStructs.at(isolatedType);
            // } else {
            //     ErrorUndeclaredIdentifier(StringView(isolatedType.c_str(), isolatedType.size()), 0, 0);
            // }
        }

        if (array) {
            BLUA_ASSERT(false, "todo");
            // ArrayDeclaration decl;
            // decl.Type = type;
            // 
            // VariableType* arrType = CreateVarType(m_Context, PrimitiveType::Array, decl);
            // type = arrType;
        }

        return type;
    }

    ConversionCost TypeChecker::GetConversionCost(VariableType* type1, VariableType* type2) {
        BLUA_ASSERT(false, "todo");
    }

    void TypeChecker::InsertImplicitCast() {
        BLUA_ASSERT(false, "todo");
    }

    VariableType* TypeChecker::RequireRValue(VariableType* type, NodeExpr* expr) {
        // Perform an implicit cast if needed
        if (type->LValue) {
            NodeExpr* copy = Allocate<NodeExpr>(*expr);
            
            ExprImplicitCast* cast = Allocate<ExprImplicitCast>();
            cast->Expression = copy;
            cast->ResolvedCastType = CastType::LValueToRValue;
            cast->ResolvedSrcType = type;
            cast->ResolvedDstType = CreateVarType(m_Context, type->Type, false, type->Data);

            expr->Data = cast;

            return cast->ResolvedDstType;
        }

        return type;
    }

    VariableType* TypeChecker::RequireLValue(VariableType* type, NodeExpr* expr) {
        if (!type->LValue) {
            m_Context->ReportCompilerError(expr->Range.Start.Line, expr->Range.Start.Column,
                                           expr->Range.End.Line, expr->Range.End.Column,
                                           expr->Loc.Line, expr->Loc.Column,
                                           "Expression must be a modifiable lvalue");

            return nullptr;
        }

        return type;
    }

} // namespace BlackLua::Internal