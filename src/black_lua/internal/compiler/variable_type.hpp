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
        bool Signed = true;
        std::variant<VariableType*, StructDeclaration> Data;

        bool operator==(const VariableType& other) {
            return Type == other.Type;
        }

        bool operator!=(const VariableType& other) {
            return !(operator==(other));
        }
    };

    inline VariableType* CreateVarType(PrimitiveType type, bool _signed = true, decltype(VariableType::Data) data = {}) {
        VariableType* t = GetAllocator()->AllocateNamed<VariableType>();
        t->Type = type;
        t->Signed = _signed;
        t->Data = data;

        return t;
    }

    inline std::string VariableTypeToString(VariableType* type) {
        std::string str;
        if (!type->Signed) {
            str = "u";
        }

        switch (type->Type) {
            case PrimitiveType::Invalid: str += "invalid"; break;
            case PrimitiveType::Void:    str += "void"; break;
            case PrimitiveType::Bool:    str += "bool"; break;
            case PrimitiveType::Char:    str += "char"; break;
            case PrimitiveType::Short:   str += "short"; break;
            case PrimitiveType::Int:     str += "int"; break;
            case PrimitiveType::Long:    str += "long"; break;
            case PrimitiveType::Float:   str += "float"; break;
            case PrimitiveType::Double:  str += "double"; break;
            case PrimitiveType::String:  str += "string"; break;

            case PrimitiveType::Array: {
                str += fmt::format("{}[]", VariableTypeToString(std::get<VariableType*>(type->Data)));
                break;
            }

            case PrimitiveType::Structure: {
                StructDeclaration decl = std::get<StructDeclaration>(type->Data);
                str += decl.Identifier;

                break;
            }
        }

        return str;
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

            case PrimitiveType::String: {
                return sizeof(void*);
            }

            case PrimitiveType::Structure: {
                StructDeclaration decl = std::get<StructDeclaration>(type->Data);
                return decl.Size;
            }

            case PrimitiveType::Array: {
                return sizeof(void*);
            }

            default: BLUA_ASSERT(false, "Unreachable!");
        }

        return 0;
    }

} // namespace BlackLua::Internal