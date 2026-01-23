#include "compiler.hpp"

#include <string>

namespace BlackLua::Internal {

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
            return m_Nodes.at(m_Index + count);
        } else {
            return nullptr;
        }
    }

    Node* TypeChecker::Consume() {
        BLUA_ASSERT(m_Index < m_Nodes.size(), "Consume() of out bounds!");

        return m_Nodes.at(m_Index++);
    }

    void TypeChecker::CheckNodeScope(Node* node) {
        NodeScope* scope = std::get<NodeScope*>(node->Data);

        Scope* newScope = GetAllocator()->AllocateNamed<Scope>();
        newScope->Parent = m_CurrentScope;
        if (m_CurrentScope) {
            newScope->ReturnType = m_CurrentScope->ReturnType;
        }
        m_CurrentScope = newScope;

        for (size_t i = 0; i < scope->Nodes.Size; i++) {
            CheckNode(scope->Nodes.Items[i]);
        }

        m_CurrentScope = m_CurrentScope->Parent;
    }

    void TypeChecker::CheckNodeVarDecl(Node* node) {
        NodeVarDecl* decl = std::get<NodeVarDecl*>(node->Data);

        std::unordered_map<std::string, Declaration>& symbolMap = (m_CurrentScope) ? m_CurrentScope->DeclaredSymbols : m_DeclaredSymbols;
        if (symbolMap.contains(std::string(decl->Identifier))) {
            ErrorRedeclaration(decl->Identifier);
        }

        symbolMap[std::string(decl->Identifier)] = { decl->Type, node };

        if (decl->Value) {
            CheckNodeExpression(decl->Type, decl->Value);
        }

        if (decl->Next) {
            CheckNodeVarDecl(decl->Next);
        }
    }

    void TypeChecker::CheckNodeFunctionImpl(Node* node) {
        NodeFunctionImpl* impl = std::get<NodeFunctionImpl*>(node->Data);

        std::string name(impl->Name);
        if (m_DeclaredSymbols.contains(name)) {
            ErrorRedeclaration(impl->Name);
        }

        m_DeclaredSymbols[name] = { impl->ReturnType, node };

        NodeScope* scope = std::get<NodeScope*>(impl->Body->Data);

        Scope* newScope = GetAllocator()->AllocateNamed<Scope>();
        newScope->Parent = m_CurrentScope;
        newScope->ReturnType = impl->ReturnType;
        m_CurrentScope = newScope;

        for (size_t i = 0; i < impl->Arguments.Size; i++) {
            CheckNode(impl->Arguments.Items[i]);
        }

        for (size_t i = 0; i < scope->Nodes.Size; i++) {
            CheckNode(scope->Nodes.Items[i]);
        }

        m_CurrentScope = m_CurrentScope->Parent;
    }

    void TypeChecker::CheckNodeReturn(Node* node) {
        NodeReturn* ret = std::get<NodeReturn*>(node->Data);

        if (!m_CurrentScope) { ErrorInvalidReturn(); return; }
        if (m_CurrentScope->ReturnType == VariableType::Invalid) { ErrorInvalidReturn(); return; }

        CheckNodeExpression(m_CurrentScope->ReturnType, ret->Value);
    }

    void TypeChecker::CheckNodeExpression(VariableType type, Node* node) {
        if (GetNodeType(node) != VariableType::Invalid) {
            size_t cost = GetConversionCost(type, GetNodeType(node));

            if (cost > 0) {
                ErrorMismatchedTypes(type, GetNodeType(node));
            }
        }
    }

    void TypeChecker::CheckNode(Node* node) {
        NodeType t = node->Type;

        if (t == NodeType::Scope) {
            CheckNodeScope(node);
        } else if (t == NodeType::VarDecl) {
            CheckNodeVarDecl(node);
        } else if (t == NodeType::FunctionImpl) {
            CheckNodeFunctionImpl(node);
        } else if (t == NodeType::Return) {
            CheckNodeReturn(node);
        } else if (t == NodeType::BinExpr) {
            CheckNodeExpression(VariableType::Invalid, node);
        }
    }

    size_t TypeChecker::GetConversionCost(VariableType type1, VariableType type2) {
        // Equal types, best possible scenario
        if (type1 == type2) {
            return 0;
        }

        if (type1 == VariableType::Bool || type1 == VariableType::Int || type1 == VariableType::Long) {
            if (type2 == VariableType::Bool || type2 == VariableType::Int || type2 == VariableType::Long) {
                return 1;
            } else if (type2 == VariableType::Float || type2 == VariableType::Double) {
                return 2;
            } else {
                return 3;
            }
        }

        if (type1 == VariableType::Float || type1 == VariableType::Double) {
            if (type2 == VariableType::Float || type2 == VariableType::Double) {
                return 1;
            } else if (type2 == VariableType::Bool || type2 == VariableType::Int || type2 == VariableType::Long) {
                return 2;
            } else {
                return 3;
            }
        }

        if (type1 == VariableType::String) {
            if (type2 != VariableType::String) {
                return 3;
            }
        }

        return -1;
    }

    VariableType TypeChecker::GetNodeType(Node* node) {
        switch (node->Type) {
            case NodeType::Bool: return VariableType::Bool;
            case NodeType::Int: return VariableType::Int;
            case NodeType::Long: return VariableType::Long;
            case NodeType::Float: return VariableType::Float;
            case NodeType::Double: return VariableType::Double;
            case NodeType::String: return VariableType::String;

            case NodeType::VarRef: {
                NodeVarRef* ref = std::get<NodeVarRef*>(node->Data);

                std::string ident(ref->Identifier);

                // Loop backward through all the scopes
                Scope* currentScope = m_CurrentScope;
                while (currentScope) {
                    if (currentScope->DeclaredSymbols.contains(ident)) {
                        return currentScope->DeclaredSymbols.at(ident).Type;
                    }

                    currentScope = currentScope->Parent;
                }

                // Check global symbols
                if (m_DeclaredSymbols.contains(std::string(ref->Identifier))) {
                    return m_DeclaredSymbols.at(std::string(ref->Identifier)).Type;
                }

                // We haven't found a variable
                ErrorUndeclaredIdentifier(ref->Identifier);
                return VariableType::Invalid;
            }

            case NodeType::FunctionCallExpr: {
                NodeFunctionCallExpr* expr = std::get<NodeFunctionCallExpr*>(node->Data);
                std::string name(expr->Name);
                if (m_DeclaredSymbols.contains(name)) {
                    NodeList args;
                    
                    switch (m_DeclaredSymbols.at(name).Decl->Type) {
                        case NodeType::FunctionImpl: {
                            NodeFunctionImpl* impl = std::get<NodeFunctionImpl*>(m_DeclaredSymbols.at(name).Decl->Data);
                            args = impl->Arguments;

                            break;
                        }
                    }

                    if (args.Size != expr->Parameters.size()) {
                        ErrorNoMatchingFunction(expr->Name);
                        return VariableType::Invalid;
                    }

                    for (size_t i = 0; i < args.Size; i++) {
                        if (std::get<NodeVarDecl*>(args.Items[i]->Data)->Type != GetNodeType(expr->Parameters[i])) {
                            ErrorNoMatchingFunction(expr->Name);
                            return VariableType::Invalid;
                        }
                    }

                    return m_DeclaredSymbols.at(name).Type;
                }

                ErrorUndeclaredIdentifier(expr->Name);
                return VariableType::Invalid;
            }

            case NodeType::ParenExpr: {
                NodeParenExpr* expr = std::get<NodeParenExpr*>(node->Data);
                VariableType type = GetNodeType(expr->Expression);

                return type;
            }

            case NodeType::CastExpr: {
                NodeCastExpr* expr = std::get<NodeCastExpr*>(node->Data);
                VariableType typeToCast = GetNodeType(expr->Expression);
                VariableType casted = expr->Type;

                size_t cost = GetConversionCost(casted, typeToCast);

                if (cost > 2) {
                    ErrorCannotCast(casted, typeToCast);
                }

                return casted;
            }

            case NodeType::UnaryExpr: {
                NodeUnaryExpr* expr = std::get<NodeUnaryExpr*>(node->Data);
                VariableType type = GetNodeType(expr->Expression);

                return type;
            }

            case NodeType::BinExpr: {
                NodeBinExpr* expr = std::get<NodeBinExpr*>(node->Data);
                VariableType typeLHS = GetNodeType(expr->LHS);
                VariableType typeRHS = GetNodeType(expr->RHS);

                if (typeRHS != typeLHS) {
                    ErrorMismatchedTypes(typeLHS, typeRHS);
                }

                return typeLHS;
            }
            default: return VariableType::Invalid;
        }

        BLUA_ASSERT(false, "Unreachable!");
        return VariableType::Invalid;
    }

    void TypeChecker::WarningMismatchedTypes(VariableType type1, VariableType type2) {
        std::cerr << "Type warning: Assigning " << VariableTypeToString(type2) << " to " << VariableTypeToString(type1) << "!\n";
    }

    void TypeChecker::ErrorMismatchedTypes(VariableType type1, VariableType type2) {
        std::cerr << "Type error: Cannot assign " << VariableTypeToString(type2) << " to " << VariableTypeToString(type1) << "!\n";

        m_Error = true;
    }

    void TypeChecker::ErrorCannotCast(VariableType type1, VariableType type2) {
        std::cerr << "Type error: Cannot cast type " << VariableTypeToString(type2) << " to type " << VariableTypeToString(type1) << "!\n";

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

    void TypeChecker::ErrorInvalidReturn() {
        std::cerr << "Type error: Invalid return statement!\n";

        m_Error = true;
    }

    void TypeChecker::ErrorNoMatchingFunction(const std::string_view func) {
        std::cerr << "Type error: No matching function to call: \"" << func << "\"!\n";

        m_Error = true;
    }

} // namespace BlackLua::Internal