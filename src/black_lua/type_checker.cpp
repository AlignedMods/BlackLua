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

        if (m_DeclaredIdentifiers.contains(std::string(decl->Identifier))) {
            ErrorRedeclaration(decl->Identifier);
        } else {
            if (decl->Value) {
                CheckNodeExpression(decl->Value, decl->Type);
            }

            m_DeclaredIdentifiers[std::string(decl->Identifier)] = node;
        }
    }

    void TypeChecker::CheckNodeVarSet(Node* node) {
        NodeVarSet* set = std::get<NodeVarSet*>(node->Data);

        if (!m_DeclaredIdentifiers.contains(std::string(set->Identifier))) {
            ErrorUndeclaredIdentifier(set->Identifier);
        } else {
            NodeVarDecl* decl = std::get<NodeVarDecl*>(m_DeclaredIdentifiers.at(std::string(set->Identifier))->Data);

            if (set->Value) {
                CheckNodeExpression(set->Value, decl->Type);
            }
        }
    }

    void TypeChecker::CheckNodeExpression(Node* node, VariableType type) {
        switch (node->Type) {
            case NodeType::Bool: {
                if (type != VariableType::Bool) {
                    ErrorMismatchedTypes(type, "bool");
                }

                break;
            }
            case NodeType::Int: {
                if (type == VariableType::Float || type == VariableType::Double) {
                    WarningMismatchedTypes(type, "integer");
                } else if (type != VariableType::Int && type != VariableType::Long) {
                    ErrorMismatchedTypes(type, "integer");
                }

                break;
            }
            case NodeType::Number: {
                if (type == VariableType::Int || type == VariableType::Long) {
                    WarningMismatchedTypes(type, "number");
                } else if (type != VariableType::Float && type != VariableType::Double) {
                    ErrorMismatchedTypes(type, "number");
                }

                break;
            }
            case NodeType::String: {
                if (type != VariableType::String) {
                    ErrorMismatchedTypes(type, "string");
                }

                break;
            }
            case NodeType::BinExpr: {
                NodeBinExpr* binExpr = std::get<NodeBinExpr*>(node->Data);

                CheckNodeExpression(binExpr->LHS, type);
                CheckNodeExpression(binExpr->RHS, type);

                break;
            }
        }
    }

    void TypeChecker::CheckNodeFunctionDecl(Node* node) {
        NodeFunctionDecl* decl = std::get<NodeFunctionDecl*>(node->Data);

        std::string sig(decl->Name);
        sig += "__BL_";
        sig += std::to_string(decl->Arguments.size());

        if (m_DeclaredIdentifiers.contains(sig)) {
            ErrorRedeclaration(decl->Name);
        }

        m_DeclaredIdentifiers[sig] = node;

        if (decl->HasBody) {
            for (const auto& n : decl->Body) {
                CheckNode(n);
            }
        }

        decl->Signature = sig;
    }

    void TypeChecker::CheckNodeFunctionCall(Node* node) {
        NodeFunctionCall* call = std::get<NodeFunctionCall*>(node->Data);

        std::string sig(call->Name);
        sig += "__BL_";
        sig += std::to_string(call->Parameters.size());

        if (!m_DeclaredIdentifiers.contains(sig)) {
            ErrorUndeclaredIdentifier(call->Name);
        }

        call->Signature = sig;
    }

    void TypeChecker::CheckNode(Node* node) {
        NodeType t = node->Type;

        if (t == NodeType::VarDecl) {
            CheckNodeVarDecl(node);
        } else if (t == NodeType::VarSet) {
            CheckNodeVarSet(node);
        } else if (t == NodeType::FunctionDecl) {
            CheckNodeFunctionDecl(node);
        } else if (t == NodeType::FunctionCall) {
            CheckNodeFunctionCall(node);
        }
    }

    void TypeChecker::WarningMismatchedTypes(VariableType type1, const std::string_view type2) {
        std::cerr << "Type warning: Assigning " << type2 << " to " << VariableTypeToString(type1) << "!\n";
    }

    void TypeChecker::ErrorMismatchedTypes(VariableType type1, const std::string_view type2) {
        std::cerr << "Type error: Cannot assign " << type2 << " to " << VariableTypeToString(type1) << "!\n";

        m_Error = true;
    }

    void TypeChecker::ErrorRedeclaration(const std::string_view msg) {
        std::cerr << "Type error: Redeclaration of \"" << msg << "\"!\n";

        m_Error = true;
    }

    void TypeChecker::ErrorUndeclaredIdentifier(const std::string_view msg) {
        std::cerr << "Type error: Undeclared identifier \"" << msg << "\"!\n";

        m_Error = true;
    }

} // namespace BlackLua::Internal