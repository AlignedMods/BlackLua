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

} // namespace BlackLua::Internal