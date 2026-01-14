#include "compiler.hpp"

#include <string>

namespace BlackLua::Internal {

    // There are certain default lua functions that we don't want to mangle
    // For example we really do not want "print("hello world")" to become "f_1print("hello world")"
    static std::unordered_map<std::string_view, bool> s_NoMangle = {
        { "tostring", true },
        { "print", true }
    };

    TypeChecker TypeChecker::Check(const Parser::Nodes& nodes) {
        TypeChecker checker;
        checker.m_Nodes = nodes;
        checker.CheckImpl();

        return checker;
    }

    const Parser::Nodes& TypeChecker::GetCheckedNodes() const {
        return m_Nodes;
    }

    bool TypeChecker::IsValid() const {
        return m_Error == false;
    }

    void TypeChecker::CheckImpl() {
        while (Peek()) {
            CheckNode(Consume());
        }
    }

    Node* TypeChecker::Peek(size_t count) {
        if (m_Index + count < m_Nodes.size()) {
            return m_Nodes.at(m_Index);
        } else {
            return nullptr;
        }
    }

    Node* TypeChecker::Consume() {
        BLUA_ASSERT(m_Index < m_Nodes.size(), "Consume() of out bounds!");

        return m_Nodes.at(m_Index++);
    }

    void TypeChecker::CheckNodeVarDecl(Node* node) {
        NodeVarDecl* decl = std::get<NodeVarDecl*>(node->Data);

        if (decl->Value) {
            CheckNodeExpression(decl->Value, decl->Type);
        }
    }

    void TypeChecker::CheckNodeExpression(Node* node, TokenType type) {
        switch (node->Type) {
            case NodeType::Bool: {
                if (type != TokenType::Bool) {
                    ErrorMismatchedTypes(type, "bool");
                }

                break;
            }
            case NodeType::Int: {
                if (type == TokenType::Float || type == TokenType::Double) {
                    WarningMismatchedTypes(type, "integer");
                } else if (type != TokenType::Int && type != TokenType::Long) {
                    ErrorMismatchedTypes(type, "integer");
                }

                break;
            }
            case NodeType::Number: {
                if (type == TokenType::Int || type == TokenType::Long) {
                    WarningMismatchedTypes(type, "number");
                } else if (type != TokenType::Float && type != TokenType::Double) {
                    ErrorMismatchedTypes(type, "number");
                }

                break;
            }
            case NodeType::String: {
                if (type != TokenType::String) {
                    ErrorMismatchedTypes(type, "string");
                }

                break;
            }
        }
    }

    void TypeChecker::CheckNode(Node* node) {
        NodeType t = node->Type;

        if (t == NodeType::VarDecl) {
            CheckNodeVarDecl(node);
        }
    }

    void TypeChecker::WarningMismatchedTypes(TokenType type1, const std::string_view type2) {
        std::cerr << "Type warning: Assigning " << type2 << " to " << TokenTypeToString(type1) << "!\n";
    }

    void TypeChecker::ErrorMismatchedTypes(TokenType type1, const std::string_view type2) {
        std::cerr << "Type error: Cannot assign " << type2 << " to " << TokenTypeToString(type1) << "!\n";

        m_Error = true;
    }

} // namespace BlackLua::Internal