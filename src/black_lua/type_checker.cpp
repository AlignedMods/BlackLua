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

    void TypeChecker::CheckNodeVarArrayDecl(Node* node) {
        NodeVarArrayDecl* decl = std::get<NodeVarArrayDecl*>(node->Data);

        std::unordered_map<std::string, Declaration>& symbolMap = (m_CurrentScope) ? m_CurrentScope->DeclaredSymbols : m_DeclaredSymbols;
        if (symbolMap.contains(std::string(decl->Identifier))) {
            ErrorRedeclaration(decl->Identifier);
        }

        symbolMap[std::string(decl->Identifier)] = { decl->Type, node };

        CheckNodeExpression(CreateVarType(PrimitiveType::Int), decl->Size);

        switch (decl->Size->Type) {
            case NodeType::Constant: {
                NodeConstant* c = std::get<NodeConstant*>(decl->Size->Data);
                switch (c->Type) {
                    case NodeType::Int: {
                        NodeInt* i = std::get<NodeInt*>(c->Data);
            
                        decl->Type.Data = static_cast<size_t>(i->Int);

                        break;
                    }

                    default: BLUA_ASSERT(false, "Unreachable!");
                }

                break;
            }

            default: BLUA_ASSERT(false, "Unreachable!");
        }

        if (decl->Value) {
            CheckNodeExpression(decl->Type, decl->Value);
        }

        if (decl->Next) {
            CheckNodeVarDecl(decl->Next);
        }
    }

    void TypeChecker::CheckNodeFunctionDecl(Node* node) {
        NodeFunctionDecl* decl = std::get<NodeFunctionDecl*>(node->Data);

        std::string name(decl->Name);

        m_DeclaredSymbols[name] = { decl->ReturnType, node, decl->Extern };
    }

    void TypeChecker::CheckNodeFunctionImpl(Node* node) {
        NodeFunctionImpl* impl = std::get<NodeFunctionImpl*>(node->Data);

        std::string name(impl->Name);
        if (m_DeclaredSymbols.contains(name)) {
            if (m_DeclaredSymbols.at(name).Extern) {
                ErrorDefiningExternFunction(name);
            }
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

    void TypeChecker::CheckNodeWhile(Node* node) {
        NodeWhile* wh = std::get<NodeWhile*>(node->Data);
        
        CheckNodeExpression(CreateVarType(PrimitiveType::Bool), wh->Condition);
        CheckNodeScope(wh->Body);
    }

    void TypeChecker::CheckNodeDoWhile(Node* node) {
        NodeDoWhile* dowh = std::get<NodeDoWhile*>(node->Data);

        CheckNodeScope(dowh->Body);
        CheckNodeExpression(CreateVarType(PrimitiveType::Bool), dowh->Condition);
    }

    void TypeChecker::CheckNodeIf(Node* node) {
        NodeIf* nif = std::get<NodeIf*>(node->Data);

        CheckNodeExpression(CreateVarType(PrimitiveType::Bool), nif->Condition);
        CheckNodeScope(nif->Body);
    }

    void TypeChecker::CheckNodeReturn(Node* node) {
        NodeReturn* ret = std::get<NodeReturn*>(node->Data);

        if (!m_CurrentScope) { ErrorInvalidReturn(); return; }
        if (m_CurrentScope->ReturnType == CreateVarType(PrimitiveType::Invalid)) { ErrorInvalidReturn(); return; }

        CheckNodeExpression(m_CurrentScope->ReturnType, ret->Value);
    }

    void TypeChecker::CheckNodeExpression(VariableType type, Node* node) {
        if (GetNodeType(node) != CreateVarType(PrimitiveType::Invalid)) {
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
        } else if (t == NodeType::VarArrayDecl) {
            CheckNodeVarArrayDecl(node);
        } else if (t == NodeType::FunctionDecl) {
            CheckNodeFunctionDecl(node);
        } else if (t == NodeType::FunctionImpl) {
            CheckNodeFunctionImpl(node);
        } else if (t == NodeType::While) {
            CheckNodeWhile(node);
        } else if (t == NodeType::If) {
            CheckNodeIf(node);
        } else if (t == NodeType::Return) {
            CheckNodeReturn(node);
        } else if (t == NodeType::FunctionCallExpr) {
            GetNodeType(node);
        } else if (t == NodeType::BinExpr) {
            GetNodeType(node);
        }
    }

    size_t TypeChecker::GetConversionCost(VariableType type1, VariableType type2) {
        // Equal types, best possible scenario
        if (type1 == type2) {
            return 0;
        }

        if (type1 == CreateVarType(PrimitiveType::Bool) || type1 == CreateVarType(PrimitiveType::Char) || type1 == CreateVarType(PrimitiveType::Short) || type1 == CreateVarType(PrimitiveType::Int) || type1 == CreateVarType(PrimitiveType::Long)) {
            if (type2 == CreateVarType(PrimitiveType::Bool) || type2 == CreateVarType(PrimitiveType::Char) || type2 == CreateVarType(PrimitiveType::Short) || type2 == CreateVarType(PrimitiveType::Int) || type2 == CreateVarType(PrimitiveType::Long)) {
                return 1;
            } else if (type2 == CreateVarType(PrimitiveType::Float) || type2 == CreateVarType(PrimitiveType::Double)) {
                return 2;
            } else {
                return 3;
            }
        }

        if (type1 == CreateVarType(PrimitiveType::Float) || type1 == CreateVarType(PrimitiveType::Double)) {
            if (type2 == CreateVarType(PrimitiveType::Float) || type2 == CreateVarType(PrimitiveType::Double)) {
                return 1;
            } else if (type2 == CreateVarType(PrimitiveType::Bool) || type2 == CreateVarType(PrimitiveType::Char) || type2 == CreateVarType(PrimitiveType::Short) || type2 == CreateVarType(PrimitiveType::Int) || type2 == CreateVarType(PrimitiveType::Long)) {
                return 2;
            } else {
                return 3;
            }
        }

        if (type1 == CreateVarType(PrimitiveType::String)) {
            if (type2 != CreateVarType(PrimitiveType::String)) {
                return 3;
            }
        }

        return -1;
    }

    VariableType TypeChecker::GetNodeType(Node* node) {
        switch (node->Type) {
            case NodeType::Constant: {
                NodeConstant* constant = std::get<NodeConstant*>(node->Data);
                VariableType type = CreateVarType(PrimitiveType::Invalid);

                switch (constant->Type) {
                    case NodeType::Bool:   type = CreateVarType(PrimitiveType::Bool); break;
                    case NodeType::Char:   type = CreateVarType(PrimitiveType::Char); break;
                    case NodeType::Int:    type = CreateVarType(PrimitiveType::Int); break;
                    case NodeType::Long:   type = CreateVarType(PrimitiveType::Long); break;
                    case NodeType::Float:  type = CreateVarType(PrimitiveType::Float); break;
                    case NodeType::Double: type = CreateVarType(PrimitiveType::Double); break;
                    case NodeType::String: type = CreateVarType(PrimitiveType::String); break;
                }

                constant->VarType = type;

                return type;
            }

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
                return CreateVarType(PrimitiveType::Invalid);
            }

            case NodeType::ArrayAccessExpr: {
                NodeArrayAccessExpr* expr = std::get<NodeArrayAccessExpr*>(node->Data);
                std::string ident(expr->Name);

                GetNodeType(expr->Index);

                // Loop backward through all the scopes
                Scope* currentScope = m_CurrentScope;
                while (currentScope) {
                    if (currentScope->DeclaredSymbols.contains(ident)) {
                        return CreateVarType(currentScope->DeclaredSymbols.at(ident).Type.Type);
                    }

                    currentScope = currentScope->Parent;
                }

                // Check global symbols
                if (m_DeclaredSymbols.contains(ident)) {
                    return CreateVarType(m_DeclaredSymbols.at(ident).Type.Type);
                }

                ErrorUndeclaredIdentifier(expr->Name);
                return CreateVarType(PrimitiveType::Invalid);
            }

            case NodeType::FunctionCallExpr: {
                NodeFunctionCallExpr* expr = std::get<NodeFunctionCallExpr*>(node->Data);
                std::string name(expr->Name);
                if (m_DeclaredSymbols.contains(name)) {
                    NodeList args;
                    
                    switch (m_DeclaredSymbols.at(name).Decl->Type) {
                        case NodeType::FunctionDecl: {
                            NodeFunctionDecl* decl = std::get<NodeFunctionDecl*>(m_DeclaredSymbols.at(name).Decl->Data);
                            args = decl->Arguments;

                            break;
                        }

                        case NodeType::FunctionImpl: {
                            NodeFunctionImpl* impl = std::get<NodeFunctionImpl*>(m_DeclaredSymbols.at(name).Decl->Data);
                            args = impl->Arguments;

                            break;
                        }
                    }

                    if (args.Size != expr->Parameters.size()) {
                        ErrorNoMatchingFunction(expr->Name);
                        return CreateVarType(PrimitiveType::Invalid);
                    }

                    for (size_t i = 0; i < args.Size; i++) {
                        if (std::get<NodeVarDecl*>(args.Items[i]->Data)->Type != GetNodeType(expr->Parameters[i])) {
                            ErrorNoMatchingFunction(expr->Name);
                            return CreateVarType(PrimitiveType::Invalid);
                        }
                    }

                    return m_DeclaredSymbols.at(name).Type;
                }

                ErrorUndeclaredIdentifier(expr->Name);
                return CreateVarType(PrimitiveType::Invalid);
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

                expr->VarType = type;

                return type;
            }

            case NodeType::BinExpr: {
                NodeBinExpr* expr = std::get<NodeBinExpr*>(node->Data);
                VariableType typeLHS = GetNodeType(expr->LHS);
                VariableType typeRHS = GetNodeType(expr->RHS);

                if (typeRHS != typeLHS) {
                    ErrorMismatchedTypes(typeLHS, typeRHS);
                }

                expr->VarType = typeLHS;

                switch (expr->Type) {
                    case BinExprType::Eq:
                    case BinExprType::Add:
                    case BinExprType::AddInPlace:
                    case BinExprType::AddOne:
                    case BinExprType::Sub:
                    case BinExprType::SubInPlace:
                    case BinExprType::SubOne:
                    case BinExprType::Mul:
                    case BinExprType::MulInPlace:
                    case BinExprType::Div:
                    case BinExprType::DivInPlace:
                        return typeLHS;
                    
                     case BinExprType::IsEq:
                     case BinExprType::IsNotEq:
                     case BinExprType::Less:
                     case BinExprType::LessOrEq:
                     case BinExprType::Greater:
                     case BinExprType::GreaterOrEq:
                        return CreateVarType(PrimitiveType::Bool);

                    default: return CreateVarType(PrimitiveType::Invalid);
                }
            }

            default: return CreateVarType(PrimitiveType::Invalid);
        }

        BLUA_ASSERT(false, "Unreachable!");
        return CreateVarType(PrimitiveType::Invalid);
    }

    void TypeChecker::WarningMismatchedTypes(VariableType type1, VariableType type2) {
        std::cerr << "Type warning: Assigning " << PrimitiveTypeToString(type2.Type) << " to " << PrimitiveTypeToString(type1.Type) << "!\n";
    }

    void TypeChecker::ErrorMismatchedTypes(VariableType type1, VariableType type2) {
        std::cerr << "Type error: Cannot assign " << PrimitiveTypeToString(type2.Type) << " to " << PrimitiveTypeToString(type1.Type) << "!\n";

        m_Error = true;
    }

    void TypeChecker::ErrorCannotCast(VariableType type1, VariableType type2) {
        std::cerr << "Type error: Cannot cast type " << PrimitiveTypeToString(type2.Type) << " to type " << PrimitiveTypeToString(type1.Type) << "!\n";

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

    void TypeChecker::ErrorDefiningExternFunction(const std::string_view func) {
        std::cerr << "Type error: Defining extern function: \"" << func << "\"!\n";

        m_Error = true;
    }

} // namespace BlackLua::Internal
