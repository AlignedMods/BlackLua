#include "internal/compiler/type_checker.hpp"

#include <string>

namespace BlackLua::Internal {

    TypeChecker TypeChecker::Check(ASTNodes* nodes) {
        TypeChecker checker;
        checker.m_Nodes = nodes;
        checker.CheckImpl();

        return checker;
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
        if (m_Index + count < m_Nodes->size()) {
            return m_Nodes->at(m_Index + count);
        } else {
            return nullptr;
        }
    }

    Node* TypeChecker::Consume() {
        BLUA_ASSERT(m_Index < m_Nodes->size(), "Consume() of out bounds!");

        return m_Nodes->at(m_Index++);
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
        
        VariableType* type = GetVarTypeFromString(decl->Type);
        symbolMap[std::string(decl->Identifier)] = { type, node };
        
        if (decl->Value) {
            CheckNodeExpression(type, decl->Value);
        }

        decl->ResolvedType = type;
    }

    void TypeChecker::CheckNodeParamDecl(Node* node) {
        NodeParamDecl* decl = std::get<NodeParamDecl*>(node->Data);

        VariableType* type = GetVarTypeFromString(decl->Type);
        m_CurrentScope->DeclaredSymbols[std::string(decl->Identifier)] = { type, node };

        decl->ResolvedType = type;
    }

    void TypeChecker::CheckNodeFunctionDecl(Node* node) {
        NodeFunctionDecl* decl = std::get<NodeFunctionDecl*>(node->Data);
        
        std::string name(decl->Name);
        
        VariableType* type = GetVarTypeFromString(decl->ReturnType);
        m_DeclaredSymbols[name] = { type, node, decl->Extern };

        decl->ResolvedType = type;
    }

    void TypeChecker::CheckNodeStructDecl(Node* node) {
        NodeStructDecl* decl = std::get<NodeStructDecl*>(node->Data);

        std::string name(decl->Identifier);
        
        StructDeclaration sd;
        sd.Identifier = decl->Identifier;
        
        for (size_t i = 0; i < decl->Fields.Size; i++) {
            switch (decl->Fields.Items[i]->Type) {
                case NodeType::FieldDecl: {
                    NodeFieldDecl* fdecl = std::get<NodeFieldDecl*>(decl->Fields.Items[i]->Data);
                    StructFieldDeclaration field;
                
                    field.Identifier = fdecl->Identifier;
                    field.Offset = sd.Size;
                    field.ResolvedType = GetVarTypeFromString(fdecl->Type);

                    sd.Size += GetTypeSize(field.ResolvedType);

                    sd.Fields.push_back(field);

                    break;
                }

                case NodeType::MethodDecl: {
                    auto mdecl = GetNode<NodeMethodDecl>(decl->Fields.Items[i]);
                    VariableType* type = GetVarTypeFromString(mdecl->ReturnType);
                    m_DeclaredSymbols[std::format("{}__{}", name, mdecl->Name)] = { type, decl->Fields.Items[i] };
                    mdecl->ResolvedType = type;
                    
                    NodeScope* scope = std::get<NodeScope*>(mdecl->Body->Data);
                    
                    Scope* newScope = GetAllocator()->AllocateNamed<Scope>();
                    newScope->Parent = m_CurrentScope;
                    newScope->ReturnType = type;
                    m_CurrentScope = newScope;
                    
                    for (size_t i = 0; i < mdecl->Parameters.Size; i++) {
                        CheckNode(mdecl->Parameters.Items[i]);
                    }
                    
                    for (size_t i = 0; i < scope->Nodes.Size; i++) {
                        CheckNode(scope->Nodes.Items[i]);
                    }
                    
                    m_CurrentScope = m_CurrentScope->Parent;
                    break;
                }
            }
        }

        m_DeclaredStructs[name] = sd;
    }

    void TypeChecker::CheckNodeFunctionImpl(Node* node) {
        NodeFunctionImpl* impl = std::get<NodeFunctionImpl*>(node->Data);
        
        std::string name(impl->Name);
        if (m_DeclaredSymbols.contains(name)) {
            if (m_DeclaredSymbols.at(name).Extern) {
                ErrorDefiningExternFunction(name);
            }
        }
        
        VariableType* type = GetVarTypeFromString(impl->ReturnType);
        m_DeclaredSymbols[name] = { type, node };
        impl->ResolvedType = type;
        
        NodeScope* scope = std::get<NodeScope*>(impl->Body->Data);
        
        Scope* newScope = GetAllocator()->AllocateNamed<Scope>();
        newScope->Parent = m_CurrentScope;
        newScope->ReturnType = type;
        m_CurrentScope = newScope;
        
        for (size_t i = 0; i < impl->Parameters.Size; i++) {
            CheckNode(impl->Parameters.Items[i]);
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
        CheckNode(nif->Body);
        if (nif->ElseBody) {
            CheckNode(nif->ElseBody);
        }
    }

    void TypeChecker::CheckNodeReturn(Node* node) {
        NodeReturn* ret = std::get<NodeReturn*>(node->Data);

        if (!m_CurrentScope) { ErrorInvalidReturn(); return; }
        if (m_CurrentScope->ReturnType == CreateVarType(PrimitiveType::Invalid)) { ErrorInvalidReturn(); return; }

        CheckNodeExpression(m_CurrentScope->ReturnType, ret->Value);
    }

    void TypeChecker::CheckNodeExpression(VariableType* type, Node* node) {
        if (GetNodeType(node) != CreateVarType(PrimitiveType::Invalid)) {
            size_t cost = GetConversionCost(type, GetNodeType(node));

            if (cost > 0) {
                // ErrorMismatchedTypes(type, GetNodeType(node));
            }
        }
    }

    void TypeChecker::CheckNode(Node* node) {
        NodeType t = node->Type;

        if (t == NodeType::Scope) {
            CheckNodeScope(node);
        } else if (t == NodeType::VarDecl) {
            CheckNodeVarDecl(node);
        } else if (t == NodeType::ParamDecl) {
            CheckNodeParamDecl(node);
        } else if (t == NodeType::FunctionDecl) {
            CheckNodeFunctionDecl(node);
        } else if (t == NodeType::FunctionImpl) {
            CheckNodeFunctionImpl(node);
        } else if (t == NodeType::StructDecl) {
            CheckNodeStructDecl(node);
        } else if (t == NodeType::While) {
            CheckNodeWhile(node);
        } else if (t == NodeType::If) {
            CheckNodeIf(node);
        } else if (t == NodeType::Return) {
            CheckNodeReturn(node);
        } else if (t == NodeType::ArrayAccessExpr) {
            GetNodeType(node);
        } else if (t == NodeType::MethodCallExpr) {
            GetNodeType(node);
        } else if (t == NodeType::FunctionCallExpr) {
            GetNodeType(node);
        } else if (t == NodeType::BinExpr) {
            GetNodeType(node);
        }
    }

    size_t TypeChecker::GetConversionCost(VariableType* type1, VariableType* type2) {
        if (!type1 || !type2) { return -1; }

        // Equal types, best possible scenario
        if (type1 == type2) {
            return 0;
        }

        if (type1->Type == PrimitiveType::Bool || type1->Type == PrimitiveType::Char || type1->Type == PrimitiveType::Short || type1->Type == PrimitiveType::Int || type1->Type == PrimitiveType::Long) {
            if (type2->Type == PrimitiveType::Bool || type2->Type == PrimitiveType::Char || type2->Type == PrimitiveType::Short || type2->Type == PrimitiveType::Int || type2->Type == PrimitiveType::Long) {
                return 1;
            } else if (type2->Type == PrimitiveType::Float || type2->Type == PrimitiveType::Double) {
                return 2;
            } else {
                return 3;
            }
        }

        if (type1->Type == PrimitiveType::Float || type1->Type == PrimitiveType::Double) {
            if (type2->Type == PrimitiveType::Float || type2->Type == PrimitiveType::Double) {
                return 1;
            } else if (type2->Type == PrimitiveType::Bool || type2->Type == PrimitiveType::Char || type2->Type == PrimitiveType::Short || type2->Type == PrimitiveType::Int || type2->Type == PrimitiveType::Long) {
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

    VariableType* TypeChecker::GetNodeType(Node* node) {
        switch (node->Type) {
            case NodeType::Constant: {
                auto constant = GetNode<NodeConstant>(node);
                VariableType* type = nullptr;

                switch (constant->Type) {
                    case NodeType::Bool:   type = CreateVarType(PrimitiveType::Bool); break;
                    case NodeType::Char:   type = CreateVarType(PrimitiveType::Char); break;
                    case NodeType::Int:    type = CreateVarType(PrimitiveType::Int); break;
                    case NodeType::Long:   type = CreateVarType(PrimitiveType::Long); break;
                    case NodeType::Float:  type = CreateVarType(PrimitiveType::Float); break;
                    case NodeType::Double: type = CreateVarType(PrimitiveType::Double); break;
                    case NodeType::String: type = CreateVarType(PrimitiveType::String); break;
                }

                constant->ResolvedType = type;

                return type;
            }

            case NodeType::VarRef: {
                auto ref = GetNode<NodeVarRef>(node);

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
                auto expr = GetNode<NodeArrayAccessExpr>(node);
                
                VariableType* arrType = GetNodeType(expr->Parent);
                if (arrType->Type != PrimitiveType::Array) {
                    std::cerr << "Error!\n";
                }

                CheckNodeExpression(CreateVarType(PrimitiveType::Int), expr->Index);
                expr->ResolvedType = std::get<VariableType*>(arrType->Data);

                return expr->ResolvedType;
            }

            case NodeType::MemberExpr: {
                auto expr = GetNode<NodeMemberExpr>(node);

                VariableType* structType = GetNodeType(expr->Parent);
                if (structType->Type != PrimitiveType::Structure) {
                    std::cerr << "Error!\n";
                }

                expr->ResolvedParentType = structType;

                StructDeclaration decl = std::get<StructDeclaration>(structType->Data);
                for (const auto& field : decl.Fields) {
                    if (field.Identifier == expr->Member) {
                        expr->ResolvedMemberType = field.ResolvedType;
                        return expr->ResolvedMemberType;
                    }
                }

                std::cerr << "Unknown field!\n";

                return nullptr;
            }

            case NodeType::MethodCallExpr: {
                auto expr = GetNode<NodeMethodCallExpr>(node);
                VariableType* structType = GetNodeType(expr->Parent);
                if (structType->Type != PrimitiveType::Structure) {
                    std::cerr << "Error!\n";
                }

                expr->ResolvedParentType = structType;
                
                StructDeclaration decl = std::get<StructDeclaration>(structType->Data);
                std::string sig = std::format("{}__{}", decl.Identifier, expr->Member);

                if (m_DeclaredSymbols.contains(sig)) {
                    Declaration decl = m_DeclaredSymbols.at(sig);
                    auto mdecl = GetNode<NodeMethodDecl>(decl.Decl);

                    if (mdecl->Parameters.Size != expr->Arguments.Size) {
                        ErrorNoMatchingFunction(expr->Member);
                        return CreateVarType(PrimitiveType::Invalid);
                    }
                
                    for (size_t i = 0; i < mdecl->Parameters.Size; i++) {
                        if (GetNode<NodeParamDecl>(mdecl->Parameters.Items[i])->ResolvedType->Type != GetNodeType(expr->Arguments.Items[i])->Type) {
                            ErrorNoMatchingFunction(expr->Member);
                            return CreateVarType(PrimitiveType::Invalid);
                        }
                    }
                
                    return mdecl->ResolvedType;
                }

                std::cerr << "Unknown method!\n";
                return nullptr;
            }

            case NodeType::FunctionCallExpr: {
                auto expr = GetNode<NodeFunctionCallExpr>(node);

                std::string name(expr->Name);
                if (m_DeclaredSymbols.contains(name)) {
                    NodeList params;
                    
                    switch (m_DeclaredSymbols.at(name).Decl->Type) {
                        case NodeType::FunctionDecl: {
                            NodeFunctionDecl* decl = std::get<NodeFunctionDecl*>(m_DeclaredSymbols.at(name).Decl->Data);
                            params = decl->Parameters;
                    
                            break;
                        }
                
                        case NodeType::FunctionImpl: {
                            NodeFunctionImpl* impl = std::get<NodeFunctionImpl*>(m_DeclaredSymbols.at(name).Decl->Data);
                            params = impl->Parameters;
                
                            break;
                        }
                    }
                
                    if (params.Size != expr->Arguments.Size) {
                        ErrorNoMatchingFunction(expr->Name);
                        return CreateVarType(PrimitiveType::Invalid);
                    }
                
                    for (size_t i = 0; i < params.Size; i++) {
                        if (std::get<NodeParamDecl*>(params.Items[i]->Data)->ResolvedType->Type != GetNodeType(expr->Arguments.Items[i])->Type) {
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
                auto expr = GetNode<NodeParenExpr>(node);
                VariableType* type = GetNodeType(expr->Expression);

                return type;
            }

            case NodeType::CastExpr: {
                auto expr = GetNode<NodeCastExpr>(node);
                VariableType* srcType = GetNodeType(expr->Expression);
                VariableType* dstType = GetVarTypeFromString(expr->Type);
                
                expr->ResolvedSrcType = srcType;
                expr->ResolvedDstType = dstType;
                
                size_t cost = GetConversionCost(dstType, srcType);
                
                if (cost > 2) {
                    // ErrorCannotCast(casted, typeToCast);
                }
                
                return dstType;
            }

            case NodeType::UnaryExpr: {
                auto expr = GetNode<NodeUnaryExpr>(node);
                VariableType* type = GetNodeType(expr->Expression);

                expr->ResolvedType = type;

                return type;
            }

            case NodeType::BinExpr: {
                auto expr = GetNode<NodeBinExpr>(node);
                VariableType* typeLHS = GetNodeType(expr->LHS);
                VariableType* typeRHS = GetNodeType(expr->RHS);

                if (typeRHS != typeLHS) {
                    // ErrorMismatchedTypes(typeLHS, typeRHS);
                }

                VariableType* resolved = nullptr;

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
                        resolved = typeLHS; break;
                    
                    case BinExprType::IsEq:
                    case BinExprType::IsNotEq:
                    case BinExprType::Less:
                    case BinExprType::LessOrEq:
                    case BinExprType::Greater:
                    case BinExprType::GreaterOrEq:
                        resolved = CreateVarType(PrimitiveType::Bool); break;

                    default: return nullptr;
                }

                expr->ResolvedType = resolved;
                return resolved;
            }

            default: return nullptr;
        }

        BLUA_ASSERT(false, "Unreachable!");
        return CreateVarType(PrimitiveType::Invalid);
    }

    VariableType* TypeChecker::GetVarTypeFromString(const std::string& str) {
        size_t bracket = str.find('[');

        std::string isolatedType;
        bool array = false;

        if (bracket != std::string::npos) {
            isolatedType = str.substr(0, bracket);
            array = true;
        } else {
            isolatedType = str;
        }

        #define TYPE(str, t) if (isolatedType == str) { type->Type = PrimitiveType::t; }

        VariableType* type = CreateVarType(PrimitiveType::Invalid);
        TYPE("void", Void);
        TYPE("bool", Bool);
        TYPE("char", Char);
        TYPE("short", Short);
        TYPE("int", Int);
        TYPE("long", Long);
        TYPE("float", Float);
        TYPE("double", Double);
        TYPE("string", String);

        #undef TYPE

        // Handle user defined type
        if (type->Type == PrimitiveType::Invalid) {
            if (m_DeclaredStructs.contains(isolatedType)) {
                type->Type = PrimitiveType::Structure;
                type->Data = m_DeclaredStructs.at(isolatedType);
            } else {
                ErrorUndeclaredIdentifier(isolatedType);
            }
        }

        if (array) {
            VariableType* arrType = CreateVarType(PrimitiveType::Array);
            arrType->Data = type;
            type = arrType;
        }

        return type;
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
