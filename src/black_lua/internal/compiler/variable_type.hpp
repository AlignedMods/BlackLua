#pragma once

#include "core.hpp"
#include "allocator.hpp"

#include <variant>
#include <vector>

namespace BlackLua::Internal {

    enum class PrimitiveType {
        Invalid = 0,
        Void,
        Bool,
        Char,
        Short,
        Int,
        Long,
        Float,
        Double,
        String,

        Array,
        Structure
    };

    inline const char* PrimitiveTypeToString(PrimitiveType type) {
        switch (type) {
            case PrimitiveType::Invalid: return "invalid";
            case PrimitiveType::Void: return "void";
            case PrimitiveType::Bool: return "bool";
            case PrimitiveType::Char: return "char";
            case PrimitiveType::Short: return "short";
            case PrimitiveType::Int: return "int";
            case PrimitiveType::Long: return "long";
            case PrimitiveType::Float: return "float";
            case PrimitiveType::Double: return "double";
            case PrimitiveType::String: return "string";

            case PrimitiveType::Array: return "array";
            case PrimitiveType::Structure: return "struct";
        }

        BLUA_ASSERT(false, "Unreachable!");
        return "invalid";
    }

    struct VariableType;

    struct StructFieldDeclaration {
        std::string_view Identifier;
        size_t Offset = 0;

        VariableType* ResolvedType = nullptr;
    };

    struct StructDeclaration {
        std::string_view Identifier;
        std::vector<StructFieldDeclaration> Fields;

        size_t Size = 0;
    };

    struct VariableType {
        PrimitiveType Type = PrimitiveType::Invalid;
        std::variant<VariableType*, StructDeclaration> Data;

        bool operator==(const VariableType& other) {
            return Type == other.Type;
        }

        bool operator!=(const VariableType& other) {
            return !(operator==(other));
        }
    };

    inline VariableType* CreateVarType(PrimitiveType type, decltype(VariableType::Data) data = {}) {
        VariableType* t = GetAllocator()->AllocateNamed<VariableType>();
        t->Type = type;
        t->Data = data;

        return t;
    }

    inline size_t GetTypeSize(VariableType* type) {
        switch (type->Type) {
            case PrimitiveType::Void: return 0;
            case PrimitiveType::Bool: return 1;
            case PrimitiveType::Char: return 1;
            case PrimitiveType::Short: return 2;
            case PrimitiveType::Int: return 4;
            case PrimitiveType::Float: return 4;
            case PrimitiveType::Long: return 8;
            case PrimitiveType::Double: return 8;

            case PrimitiveType::Structure: {
                StructDeclaration decl = std::get<StructDeclaration>(type->Data);
                return decl.Size;
            }

            case PrimitiveType::Array: {
                return sizeof(void*) + 4;
            }

            default: BLUA_ASSERT(false, "Unreachable!");
        }

        return 0;
    }

} // namespace BlackLua::Internal