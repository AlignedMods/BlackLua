#pragma once

#include "internal/compiler/variable_type.hpp"
#include "internal/compiler/core/string_builder.hpp"

namespace BlackLua::Internal {
    
    enum class ConversionType {
        None,
        Promotion,
        Narrowing
    };

    struct ConversionCost {
        ConversionType Type = ConversionType::None;

        bool CastNeeded = false;
        bool SignedMismatch = false;
        bool LValueMismatch = false;
        bool ImplicitCastPossible = false;
        bool ExplicitCastPossible = false;

        VariableType* Source = nullptr;
        VariableType* Destination = nullptr;
    };

    class TypeChecker {
    public:
        TypeChecker(Context* ctx);

        VariableType* GetVariableTypeFromString(StringView str);

    private:
        Context* m_Context = nullptr;
    };

} // namespace BlackLua::Internal