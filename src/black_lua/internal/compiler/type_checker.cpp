#include "internal/compiler/type_checker.hpp"
#include "context.hpp"

#include <string>

namespace BlackLua::Internal {

    TypeChecker TypeChecker::Check(ASTNodes* nodes, Context* ctx) {
        TypeChecker checker;
        checker.m_Nodes = nodes;
        checker.m_Context = ctx;
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

        Scope* newScope = m_Context->GetAllocator()->AllocateNamed<Scope>();
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
        if (symbolMap.contains(fmt::format("{}", decl->Identifier))) {
            m_Context->ReportCompilerError(node->Line, node->Column, fmt::format("Redeclaring identifier {}", decl->Identifier));
            m_Error = true;
        }
        
        VariableType* type = GetVarTypeFromString(StringView(decl->Type.Data(), decl->Type.Size()));
        symbolMap[fmt::format("{}", decl->Identifier)] = { type, node };
        
        if (decl->Value) {
            CheckNodeExpression(decl->Value);
        }
        
        decl->ResolvedType = type;
    }

    void TypeChecker::CheckNodeParamDecl(Node* node) {
        NodeParamDecl* decl = std::get<NodeParamDecl*>(node->Data);
        
        VariableType* type = GetVarTypeFromString(StringView(decl->Type.Data(), decl->Type.Size()));
        m_CurrentScope->DeclaredSymbols[fmt::format("{}", decl->Identifier)] = { type, node };
        
        decl->ResolvedType = type;
    }

    void TypeChecker::CheckNodeFunctionDecl(Node* node) {
        NodeFunctionDecl* decl = std::get<NodeFunctionDecl*>(node->Data);
        
        std::string name = fmt::format("{}", decl->Name);
        
        VariableType* type = GetVarTypeFromString(StringView(decl->ReturnType.Data(), decl->ReturnType.Size()));
        m_DeclaredSymbols[name] = { type, node, decl->Extern };
        
        decl->ResolvedType = type;
    }

    void TypeChecker::CheckNodeStructDecl(Node* node) {
        NodeStructDecl* decl = std::get<NodeStructDecl*>(node->Data);
        
        std::string name = fmt::format("{}", decl->Identifier);
        
        StructDeclaration sd;
        sd.Identifier = decl->Identifier;
        
        for (size_t i = 0; i < decl->Fields.Size; i++) {
            switch (decl->Fields.Items[i]->Type) {
                case NodeType::FieldDecl: {
                    NodeFieldDecl* fdecl = std::get<NodeFieldDecl*>(decl->Fields.Items[i]->Data);
                    StructFieldDeclaration field;
                
                    field.Identifier = fdecl->Identifier;
                    field.Offset = sd.Size;
                    field.ResolvedType = GetVarTypeFromString(StringView(fdecl->Type.Data(), fdecl->Type.Size()));
        
                    sd.Size += GetTypeSize(field.ResolvedType);
        
                    sd.Fields.push_back(field);
        
                    break;
                }
        
                case NodeType::MethodDecl: {
                    auto mdecl = GetNode<NodeMethodDecl>(decl->Fields.Items[i]);
                    VariableType* type = GetVarTypeFromString(StringView(mdecl->ReturnType.Data(), mdecl->ReturnType.Size()));
                    m_DeclaredSymbols[fmt::format("{}__{}", name, mdecl->Name)] = { type, decl->Fields.Items[i] };
                    mdecl->ResolvedType = type;
                    
                    NodeScope* scope = std::get<NodeScope*>(mdecl->Body->Data);
                    
                    Scope* newScope = m_Context->GetAllocator()->AllocateNamed<Scope>();
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
        
        std::string name = fmt::format("{}", impl->Name);
        if (m_DeclaredSymbols.contains(name)) {
            if (m_DeclaredSymbols.at(name).Extern) {
                m_Context->ReportCompilerError(node->Line, node->Column, fmt::format("Defining function marked extern: {}", impl->Name));
                m_Error = true;
            }
        }
        
        VariableType* type = GetVarTypeFromString(StringView(impl->ReturnType.Data(), impl->ReturnType.Size()));
        m_DeclaredSymbols[name] = { type, node };
        impl->ResolvedType = type;
        
        NodeScope* scope = std::get<NodeScope*>(impl->Body->Data);
        
        Scope* newScope = m_Context->GetAllocator()->AllocateNamed<Scope>();
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
        
        CheckNodeExpression(wh->Condition);
        CheckNodeScope(wh->Body);
    }

    void TypeChecker::CheckNodeDoWhile(Node* node) {
        NodeDoWhile* dowh = std::get<NodeDoWhile*>(node->Data);

        CheckNodeScope(dowh->Body);
        CheckNodeExpression(dowh->Condition);
    }

    void TypeChecker::CheckNodeIf(Node* node) {
        NodeIf* nif = std::get<NodeIf*>(node->Data);

        CheckNodeExpression(nif->Condition);
        CheckNode(nif->Body);
        if (nif->ElseBody) {
            CheckNode(nif->ElseBody);
        }
    }

    void TypeChecker::CheckNodeReturn(Node* node) {
        NodeReturn* ret = std::get<NodeReturn*>(node->Data);

        bool canReturn = true;
        if (!m_CurrentScope) {
            canReturn = false;
        } else if (m_CurrentScope->ReturnType == CreateVarType(m_Context, PrimitiveType::Invalid)) {
            canReturn = false;
        }

        if (!canReturn) {
            m_Context->ReportCompilerError(node->Line, node->Column, "Cannot return from a non-function scope");
            m_Error = true;
            return;
        }

        VariableType* exprType = CheckNodeExpression(ret->Value);
        ConversionCost cost = GetConversionCost(m_CurrentScope->ReturnType, exprType);

        if (cost.CastNeeded) {
            if (cost.ImplicitCastPossible) {
                InsertImplicitCast(ret->Value, m_CurrentScope->ReturnType, exprType);
            } else {
                m_Context->ReportCompilerError(node->Line, node->Column, fmt::format("Cannot implicitly cast from {} to {}", VariableTypeToString(exprType), VariableTypeToString(m_CurrentScope->ReturnType)));
                m_Error = true;
            }
        }
    }

    VariableType* TypeChecker::CheckNodeExpression(Node* node) {
        switch (node->Type) {
            case NodeType::Constant: {
                auto constant = GetNode<NodeConstant>(node);
                VariableType* type = nullptr;
        
                switch (constant->Type) {
                    case NodeType::Bool:   type = CreateVarType(m_Context, PrimitiveType::Bool); break;
                    case NodeType::Char:   type = CreateVarType(m_Context, PrimitiveType::Char); break;
        
                    case NodeType::Int: {
                        auto i = std::get<NodeInt*>(constant->Data);
                        type = CreateVarType(m_Context, PrimitiveType::Int, !i->Unsigned);
                        break;
                    }
        
                    case NodeType::Long: {
                        auto l = std::get<NodeLong*>(constant->Data);
                        type = CreateVarType(m_Context, PrimitiveType::Long, !l->Unsigned);
                        break;
                    }
        
                    case NodeType::Float:  type = CreateVarType(m_Context, PrimitiveType::Float); break;
                    case NodeType::Double: type = CreateVarType(m_Context, PrimitiveType::Double); break;
                    case NodeType::String: type = CreateVarType(m_Context, PrimitiveType::String); break;
                }
        
                constant->ResolvedType = type;
        
                return type;
            }
        
            case NodeType::VarRef: {
                auto ref = GetNode<NodeVarRef>(node);
        
                std::string ident = fmt::format("{}", ref->Identifier);
        
                // Loop backward through all the scopes
                Scope* currentScope = m_CurrentScope;
                while (currentScope) {
                    if (currentScope->DeclaredSymbols.contains(ident)) {
                        return currentScope->DeclaredSymbols.at(ident).Type;
                    }
        
                    currentScope = currentScope->Parent;
                }
        
                // Check global symbols
                if (m_DeclaredSymbols.contains(fmt::format("{}", (ref->Identifier)))) {
                    return m_DeclaredSymbols.at(fmt::format("{}", ref->Identifier)).Type;
                }
        
                // We haven't found a variable
                ErrorUndeclaredIdentifier(ref->Identifier, node);
                return CreateVarType(m_Context, PrimitiveType::Invalid);
            }
        
            case NodeType::ArrayAccessExpr: {
                auto expr = GetNode<NodeArrayAccessExpr>(node);
                
                VariableType* arrType = CheckNodeExpression(expr->Parent);
                if (arrType->Type != PrimitiveType::Array) {
                    std::cerr << "Error!\n";
                }
        
                CheckNodeExpression(expr->Index);
                expr->ResolvedType = std::get<VariableType*>(arrType->Data);
        
                return expr->ResolvedType;
            }
        
            case NodeType::MemberExpr: {
                auto expr = GetNode<NodeMemberExpr>(node);
        
                VariableType* structType = CheckNodeExpression(expr->Parent);
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
                VariableType* structType = CheckNodeExpression(expr->Parent);
                if (structType->Type != PrimitiveType::Structure) {
                    std::cerr << "Error!\n";
                }
        
                expr->ResolvedParentType = structType;
                
                StructDeclaration decl = std::get<StructDeclaration>(structType->Data);
                std::string sig = fmt::format("{}__{}", decl.Identifier, expr->Member);
        
                if (m_DeclaredSymbols.contains(sig)) {
                    Declaration decl = m_DeclaredSymbols.at(sig);
                    auto mdecl = GetNode<NodeMethodDecl>(decl.Decl);
        
                    if (mdecl->Parameters.Size != expr->Arguments.Size) {
                        ErrorNoMatchingFunction(expr->Member, node);
                        return CreateVarType(m_Context, PrimitiveType::Invalid);
                    }
                
                    for (size_t i = 0; i < mdecl->Parameters.Size; i++) {
                        if (GetNode<NodeParamDecl>(mdecl->Parameters.Items[i])->ResolvedType->Type != CheckNodeExpression(expr->Arguments.Items[i])->Type) {
                            ErrorNoMatchingFunction(expr->Member, node);
                            return CreateVarType(m_Context, PrimitiveType::Invalid);
                        }
                    }
                
                    return mdecl->ResolvedType;
                }
        
                std::cerr << "Unknown method!\n";
                return nullptr;
            }
        
            case NodeType::FunctionCallExpr: {
                auto expr = GetNode<NodeFunctionCallExpr>(node);
        
                std::string name = fmt::format("{}", expr->Name);
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
                        ErrorNoMatchingFunction(expr->Name, node);
                        return CreateVarType(m_Context, PrimitiveType::Invalid);
                    }
                
                    for (size_t i = 0; i < params.Size; i++) {
                        Node* param = params.Items[i];
                        Node* arg = expr->Arguments.Items[i];
        
                        VariableType* typeParam = GetNode<NodeParamDecl>(param)->ResolvedType;
                        VariableType* typeArg = CheckNodeExpression(arg);
        
                        ConversionCost cost = GetConversionCost(typeParam, typeArg);
                        if (cost.CastNeeded) {
                            // Perform an implicit cast (if possible)
                            if (cost.ImplicitCastPossible) {
                                InsertImplicitCast(arg, typeParam, typeArg);
                            } else {
                                m_Context->ReportCompilerError(arg->Line, arg->Column, fmt::format("Mismatched function argument types, parameter type is {}, while argument type is {}", VariableTypeToString(typeParam), VariableTypeToString(typeArg)));
                                m_Error = true;
                            }
                        }
                    }
                
                    return m_DeclaredSymbols.at(name).Type;
                }
                
                ErrorUndeclaredIdentifier(expr->Name, node);
                return CreateVarType(m_Context, PrimitiveType::Invalid);
            }
        
            case NodeType::ParenExpr: {
                auto expr = GetNode<NodeParenExpr>(node);
                VariableType* type = CheckNodeExpression(expr->Expression);
        
                return type;
            }
        
            case NodeType::CastExpr: {
                auto expr = GetNode<NodeCastExpr>(node);
                if (!expr->ResolvedSrcType) {
                    expr->ResolvedSrcType = CheckNodeExpression(expr->Expression);
                }
        
                if (!expr->ResolvedDstType) {
                    expr->ResolvedDstType = GetVarTypeFromString(expr->Type);
                }
                
                ConversionCost cost = GetConversionCost(expr->ResolvedDstType, expr->ResolvedSrcType);
                
                if (!cost.ExplicitCastPossible) {
                    m_Context->ReportCompilerError(node->Line, node->Column, fmt::format("Cannot cast from {} to {}", VariableTypeToString(expr->ResolvedSrcType), VariableTypeToString(expr->ResolvedDstType)));
                    m_Error = true;
                }
                
                return expr->ResolvedDstType;
            }
        
            case NodeType::UnaryExpr: {
                auto expr = GetNode<NodeUnaryExpr>(node);
                VariableType* type = CheckNodeExpression(expr->Expression);
        
                expr->ResolvedType = type;
        
                return type;
            }
        
            case NodeType::BinExpr: {
                auto expr = GetNode<NodeBinExpr>(node);
                VariableType* typeLHS = CheckNodeExpression(expr->LHS);
                VariableType* typeRHS = CheckNodeExpression(expr->RHS);
        
                ConversionCost cost = GetConversionCost(typeLHS, typeRHS);
                if (cost.CastNeeded) {
                    // Perform an implicit cast (if possible)
                    if (cost.ImplicitCastPossible) {
                        // We always want to cast to a wider type
                        // This will check which side of the binary expression we should cast
                        // One thing to note though, if we have an assignment we cannot cast the left side
                        if (GetTypeSize(typeLHS) > GetTypeSize(typeRHS) || expr->Type == BinExprType::Eq) {
                            InsertImplicitCast(expr->RHS, typeLHS, typeRHS);
                        } else {
                            InsertImplicitCast(expr->LHS, typeRHS, typeLHS);
                        }
                    } else {
                        m_Context->ReportCompilerError(node->Line, node->Column, fmt::format("Mismatched types, have {} and {}", VariableTypeToString(typeLHS), VariableTypeToString(typeRHS)));
                        m_Error = true;
                    }
                }
        
                VariableType* resolved = nullptr;
        
                switch (expr->Type) {
                    case BinExprType::Eq: {
                        if (!IsLValue(expr->LHS)) {
                            m_Context->ReportCompilerError(expr->LHS->Line, expr->LHS->Column, "Expression must be a modifiable lvalue");
                            m_Error = true;
                        }
                        resolved = typeLHS; break;
                    }
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
                    case BinExprType::Mod:
                    case BinExprType::ModInPlace:
                        resolved = typeLHS; break;
                    
                    case BinExprType::IsEq:
                    case BinExprType::IsNotEq:
                    case BinExprType::Less:
                    case BinExprType::LessOrEq:
                    case BinExprType::Greater:
                    case BinExprType::GreaterOrEq:
                        resolved = CreateVarType(m_Context, PrimitiveType::Bool); break;
        
                    default: return nullptr;
                }
        
                expr->ResolvedType = resolved;
                expr->ResolvedSourceType = typeLHS;
                return resolved;
            }
        
            default: return nullptr;
        }
        
        BLUA_ASSERT(false, "Unreachable!");
        return CreateVarType(m_Context, PrimitiveType::Invalid);
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
            CheckNodeExpression(node);
        } else if (t == NodeType::MethodCallExpr) {
            CheckNodeExpression(node);
        } else if (t == NodeType::FunctionCallExpr) {
            CheckNodeExpression(node);
        } else if (t == NodeType::BinExpr) {
            CheckNodeExpression(node);
        }
    }

    ConversionCost TypeChecker::GetConversionCost(VariableType* type1, VariableType* type2) {
        if (!type1 || !type2) { return {}; }

        ConversionCost cost{};
        cost.CastNeeded = true;
        cost.ExplicitCastPossible = true;
        cost.ImplicitCastPossible = true;

        cost.Source = type2;
        cost.Destination = type1;

        if (type1->Signed != type2->Signed) {
            cost.SignedMismatch = true;
            cost.ImplicitCastPossible = false;
        }

        // Equal types, best possible scenario
        if (type1->Type == type2->Type) {
            cost.Type = ConversionType::None;
            cost.CastNeeded = cost.SignedMismatch;
            return cost;
        }

        if (type1->Type == PrimitiveType::Bool || type1->Type == PrimitiveType::Char || type1->Type == PrimitiveType::Short || type1->Type == PrimitiveType::Int || type1->Type == PrimitiveType::Long) {
            if (type2->Type == PrimitiveType::Bool || type2->Type == PrimitiveType::Char || type2->Type == PrimitiveType::Short || type2->Type == PrimitiveType::Int || type2->Type == PrimitiveType::Long) {
                if (GetTypeSize(type1) > GetTypeSize(type2)) {
                    cost.Type = ConversionType::Narrowing;
                } else {
                    cost.Type = ConversionType::Promotion;
                }
            } else if (type2->Type == PrimitiveType::Float || type2->Type == PrimitiveType::Double) {
                cost.ImplicitCastPossible = false;

                if (GetTypeSize(type1) > GetTypeSize(type2)) {
                    cost.Type = ConversionType::Narrowing;
                } else {
                    cost.Type = ConversionType::Promotion;
                }
            } else {
                cost.ExplicitCastPossible = false;
            }
        }

        if (type1->Type == PrimitiveType::Float || type1->Type == PrimitiveType::Double) {
            if (type2->Type == PrimitiveType::Float || type2->Type == PrimitiveType::Double) {
                if (GetTypeSize(type1) > GetTypeSize(type1)) {
                    cost.Type = ConversionType::Narrowing;
                } else {
                    cost.Type = ConversionType::Promotion;
                }
            } else if (type2->Type == PrimitiveType::Bool || type2->Type == PrimitiveType::Char || type2->Type == PrimitiveType::Short || type2->Type == PrimitiveType::Int || type2->Type == PrimitiveType::Long) {
                cost.ImplicitCastPossible = false;

                if (GetTypeSize(type1) > GetTypeSize(type2)) {
                    cost.Type = ConversionType::Narrowing;
                } else {
                    cost.Type = ConversionType::Promotion;
                }
            } else {
                cost.ExplicitCastPossible = false;
            }
        }

        return cost;
    }

    void TypeChecker::InsertImplicitCast(Node* node, VariableType* dest, VariableType* src) {
        NodeCastExpr* cast = m_Context->GetAllocator()->AllocateNamed<NodeCastExpr>();
        Node* newNode = m_Context->GetAllocator()->AllocateNamed<Node>();
        memcpy(newNode, node, sizeof(Node));
        cast->Expression = newNode;
        cast->ResolvedSrcType = src;
        cast->ResolvedDstType = dest;

        node->Data = cast;
        node->Type = NodeType::CastExpr;
    }

    VariableType* TypeChecker::GetVarTypeFromString(StringView str) {
        size_t bracket = str.Find('[');

        std::string isolatedType;
        bool array = false;

        if (bracket != StringView::npos) {
            isolatedType = fmt::format("{}", str.SubStr(0, bracket));
            array = true;
        } else {
            isolatedType = fmt::format("{}", str);
        }

        #define TYPE(str, t, _signed) if (isolatedType == str) { type->Type = PrimitiveType::t; type->Signed = _signed; }

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
            if (m_DeclaredStructs.contains(isolatedType)) {
                type->Type = PrimitiveType::Structure;
                type->Data = m_DeclaredStructs.at(isolatedType);
            } else {
                ErrorUndeclaredIdentifier(StringView(isolatedType.c_str(), isolatedType.size()), nullptr);
            }
        }

        if (array) {
            VariableType* arrType = CreateVarType(m_Context, PrimitiveType::Array);
            arrType->Data = type;
            type = arrType;
        }

        return type;
    }

    bool TypeChecker::IsLValue(Node* node) {
        if (node->Type == NodeType::VarRef || node->Type == NodeType::MemberExpr || node->Type == NodeType::ArrayAccessExpr) {
            return true;
        }

        return false;
    }

    void TypeChecker::ErrorUndeclaredIdentifier(const StringView ident, Node* node) {
        m_Context->ReportCompilerError(node->Line, node->Column, fmt::format("Undeclared identifier {}", ident));
        m_Error = true;
    }

    void TypeChecker::ErrorNoMatchingFunction(const StringView func, Node* node) {
        m_Context->ReportCompilerError(node->Line, node->Column, fmt::format("No matching function to call: {}", func));
        m_Error = true;
    }

} // namespace BlackLua::Internal
