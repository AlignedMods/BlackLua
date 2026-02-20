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

    VariableType* TypeChecker::CheckNodeExpression(NodeExpr* expr) {
        if (ExprConstant* con = GetNode<ExprConstant>(expr)) {
            VariableType* type = nullptr;
        
            if (GetNode<ConstantBool>(con)) {
                type = CreateVarType(m_Context, PrimitiveType::Bool, true);
            } else if (GetNode<ConstantChar>(con)) {
                type = CreateVarType(m_Context, PrimitiveType::Char);
            } else if (ConstantInt* ci = GetNode<ConstantInt>(con)) {
                type = CreateVarType(m_Context, PrimitiveType::Int, !ci->Unsigned);
            } else if (ConstantLong* cl = GetNode<ConstantLong>(con)) {
                type = CreateVarType(m_Context, PrimitiveType::Long, !cl->Unsigned);
            } else if (GetNode<ConstantFloat>(con)) {
                type = CreateVarType(m_Context, PrimitiveType::Float, true);
            } else if (GetNode<ConstantDouble>(con)) {
                type = CreateVarType(m_Context, PrimitiveType::Double, true);
            } else if (GetNode<ConstantString>(con)) {
                type = CreateVarType(m_Context, PrimitiveType::String);
            }
        
            con->ResolvedType = type;
        
            return type;
        }

        if (ExprVarRef* ref = GetNode<ExprVarRef>(expr)) {
            std::string ident = fmt::format("{}", ref->Identifier);
        
            // First check if we are in a struct context
            if (m_ActiveStruct) {
                for (const auto& field : m_ActiveStruct->Fields) {
                    if (field.Identifier == StringView(ident.data(), ident.size())) {
                        ExprMember* mem = m_Context->GetAllocator()->AllocateNamed<ExprMember>();
                        mem->Member = ref->Identifier;
                        mem->Parent = m_Context->GetAllocator()->AllocateNamed<NodeExpr>(m_Context->GetAllocator()->AllocateNamed<ExprSelf>(), expr->Loc);
                        mem->ResolvedParentType = CreateVarType(m_Context, PrimitiveType::Structure, *m_ActiveStruct);
                        mem->ResolvedMemberType = field.ResolvedType;

                        expr->Data = mem;
                        ref->ResolvedType = mem->ResolvedMemberType;
                        return mem->ResolvedMemberType;
                    }
                }
            }

            // Loop backward through all the scopes
            Scope* currentScope = m_CurrentScope;
            while (currentScope) {
                if (currentScope->DeclaredSymbols.contains(ident)) {
                    ref->ResolvedType = currentScope->DeclaredSymbols.at(ident).Type;
                    return currentScope->DeclaredSymbols.at(ident).Type;
                }
            
                currentScope = currentScope->Parent;
            }
            
            // Check global symbols
            if (m_DeclaredSymbols.contains(fmt::format("{}", (ref->Identifier)))) {
                ref->ResolvedType = m_DeclaredSymbols.at(fmt::format("{}", ref->Identifier)).Type;
                return m_DeclaredSymbols.at(fmt::format("{}", ref->Identifier)).Type;
            }
            
            // We haven't found a variable
            // ErrorUndeclaredIdentifier(ref->Identifier, expr->Line, expr->Column);
            return CreateVarType(m_Context, PrimitiveType::Invalid);
        }
        
        if (ExprArrayAccess* arrAccess = GetNode<ExprArrayAccess>(expr)) {
            VariableType* arrType = CheckNodeExpression(arrAccess->Parent);
            if (arrType->Type != PrimitiveType::Array) {
                std::cerr << "Error!\n";
            }
        
            CheckNodeExpression(arrAccess->Index);
            arrAccess->ResolvedType = std::get<VariableType*>(arrType->Data);
        
            return arrAccess->ResolvedType;
        }

        if (ExprSelf* self = GetNode<ExprSelf>(expr)) {
            if (!m_ActiveStruct) {
                // TODO: error
                return CreateVarType(m_Context, PrimitiveType::Invalid);
            }

            return CreateVarType(m_Context, PrimitiveType::Structure, *m_ActiveStruct);
        }

        if (ExprMember* mem = GetNode<ExprMember>(expr)) {
            VariableType* structType = CheckNodeExpression(mem->Parent);
            if (structType->Type != PrimitiveType::Structure) {
                std::cerr << "Error!\n";
            }
        
            mem->ResolvedParentType = structType;
        
            StructDeclaration decl = std::get<StructDeclaration>(structType->Data);
            for (const auto& field : decl.Fields) {
                if (field.Identifier == mem->Member) {
                    mem->ResolvedMemberType = field.ResolvedType;
                    return mem->ResolvedMemberType;
                }
            }

            // m_Context->ReportCompilerError(expr->Line, expr->Column, fmt::format("Unknown field \"{}\"", mem->Member));
            return nullptr;
        }

        if (ExprMethodCall* call = GetNode<ExprMethodCall>(expr)) {
            VariableType* parentType = CheckNodeExpression(call->Parent);
            call->ResolvedParentType = parentType;
            std::string name = fmt::format("{}__{}", std::get<StructDeclaration>(parentType->Data).Identifier, call->Member);
            if (m_DeclaredSymbols.contains(name)) {
                NodeList params = GetNode<StmtMethodDecl>(m_DeclaredSymbols.at(name).Decl)->Parameters;
        
                if (params.Size != call->Arguments.Size) {
                    // ErrorNoMatchingFunction(call->Name, expr->Line, expr->Column);
                    return CreateVarType(m_Context, PrimitiveType::Invalid);
                }
        
                for (size_t i = 0; i < params.Size; i++) {
                    Node* param = params.Items[i];
                    NodeExpr* arg = GetNode<NodeExpr>(call->Arguments.Items[i]);
        
                    VariableType* typeParam = GetNode<StmtParamDecl>(GetNode<NodeStmt>((param)))->ResolvedType;
                    VariableType* typeArg = CheckNodeExpression(arg);
        
                    ConversionCost cost = GetConversionCost(typeParam, typeArg);
                    if (cost.CastNeeded) {
                        // Perform an implicit cast (if possible)
                        if (cost.ImplicitCastPossible) {
                            InsertImplicitCast(arg, typeParam, typeArg);
                        } else {
                            // m_Context->ReportCompilerError(arg->Line, arg->Column, fmt::format("Mismatched function argument types, parameter type is {}, while argument type is {}", VariableTypeToString(typeParam), VariableTypeToString(typeArg)));
                            m_Error = true;
                        }
                    }
                }
        
                // call->ResolvedReturnType = m_DeclaredSymbols.at(name).Type;
                // return call->ResolvedReturnType; 
                return m_DeclaredSymbols.at(name).Type;
            }
        }

        if (ExprCall* call = GetNode<ExprCall>(expr)) {
            std::string name = fmt::format("{}", call->Name);
            if (m_DeclaredSymbols.contains(name)) {
                NodeList params;
            
                if (StmtFunctionDecl* decl = GetNode<StmtFunctionDecl>(m_DeclaredSymbols.at(name).Decl)) {
                    params = decl->Parameters;
                    call->Extern = decl->Extern;
                } else {
                    // ErrorNoMatchingFunction(call->Name, expr->Line, expr->Column);
                    return CreateVarType(m_Context, PrimitiveType::Invalid);
                }
        
                if (params.Size != call->Arguments.Size) {
                    // ErrorNoMatchingFunction(call->Name, expr->Line, expr->Column);
                    return CreateVarType(m_Context, PrimitiveType::Invalid);
                }
        
                for (size_t i = 0; i < params.Size; i++) {
                    Node* param = params.Items[i];
                    NodeExpr* arg = GetNode<NodeExpr>(call->Arguments.Items[i]);
        
                    VariableType* typeParam = GetNode<StmtParamDecl>(GetNode<NodeStmt>((param)))->ResolvedType;
                    VariableType* typeArg = CheckNodeExpression(arg);
        
                    ConversionCost cost = GetConversionCost(typeParam, typeArg);
                    if (cost.CastNeeded) {
                        // Perform an implicit cast (if possible)
                        if (cost.ImplicitCastPossible) {
                            InsertImplicitCast(arg, typeParam, typeArg);
                        } else {
                            // m_Context->ReportCompilerError(arg->Line, arg->Column, fmt::format("Mismatched function argument types, parameter type is {}, while argument type is {}", VariableTypeToString(typeParam), VariableTypeToString(typeArg)));
                            m_Error = true;
                        }
                    }
                }
        
                call->ResolvedReturnType = m_DeclaredSymbols.at(name).Type;
                return call->ResolvedReturnType; 
            }

            // ErrorUndeclaredIdentifier(call->Name, expr->Line, expr->Column);
            return CreateVarType(m_Context, PrimitiveType::Invalid);
        }
        
        if (ExprParen* paren = GetNode<ExprParen>(expr)) {
            VariableType* type = CheckNodeExpression(paren->Expression);
        
            return type; 
        }

        if (ExprCast* cast = GetNode<ExprCast>(expr)) {
            if (!cast->ResolvedSrcType) {
                cast->ResolvedSrcType = CheckNodeExpression(cast->Expression);
            }
            
            if (!cast->ResolvedDstType) {
                cast->ResolvedDstType = GetVarTypeFromString(cast->Type);
            }
            
            ConversionCost cost = GetConversionCost(cast->ResolvedDstType, cast->ResolvedSrcType);
            
            if (!cost.ExplicitCastPossible) {
                // m_Context->ReportCompilerError(expr->Line, expr->Column, fmt::format("Cannot cast from {} to {}", VariableTypeToString(cast->ResolvedSrcType), VariableTypeToString(cast->ResolvedDstType)));
                m_Error = true;
            }

            VariableType* src = cast->ResolvedSrcType;
            VariableType* dest = cast->ResolvedDstType;

            if (src->IsIntegral() && dest->IsIntegral()) {
                cast->ResolvedCastType = CastType::Integral;
            } else if (src->IsFloatingPoint() && dest->IsFloatingPoint()) {
                cast->ResolvedCastType = CastType::Floating;
            } else if (src->IsIntegral() && dest->IsFloatingPoint()) {
                cast->ResolvedCastType = CastType::IntegralToFloating;
            } else if (src->IsFloatingPoint() && dest->IsIntegral()) {
                cast->ResolvedCastType = CastType::FloatingToIntegral;
            } else {
                BLUA_ASSERT(false, "Impossible cast combination");
            }
            
            return dest;
        }
        
        if (ExprUnaryOperator* unOp = GetNode<ExprUnaryOperator>(expr)) {
            VariableType* type = CheckNodeExpression(unOp->Expression);
        
            unOp->ResolvedType = type;
        
            return type;
        }

        if (ExprBinaryOperator* binOp = GetNode<ExprBinaryOperator>(expr)) {
            VariableType* typeLHS = CheckNodeExpression(binOp->LHS);
            VariableType* typeRHS = CheckNodeExpression(binOp->RHS);
        
            ConversionCost cost = GetConversionCost(typeLHS, typeRHS);
            if (cost.CastNeeded) {
                // Perform an implicit cast (if possible)
                if (cost.ImplicitCastPossible) {
                    // We always want to cast to a wider type
                    // This will check which side of the binary expression we should cast
                    // One thing to note though, if we have an assignment we cannot cast the left side
                    if (GetTypeSize(typeLHS) > GetTypeSize(typeRHS) || binOp->Type == BinaryOperatorType::Eq) {
                        InsertImplicitCast(binOp->RHS, typeLHS, typeRHS);
                    } else {
                        InsertImplicitCast(binOp->LHS, typeRHS, typeLHS);
                    }
                } else {
                    // m_Context->ReportCompilerError(expr->Line, expr->Column, fmt::format("Mismatched types, have {} and {}", VariableTypeToString(typeLHS), VariableTypeToString(typeRHS)));
                    m_Error = true;
                }
            }
        
            VariableType* resolved = nullptr;
        
            switch (binOp->Type) {
                case BinaryOperatorType::Eq: {
                    if (!IsLValue(binOp->LHS)) {
                        // m_Context->ReportCompilerError(binOp->LHS->Line, binOp->LHS->Column, "Expression must be a modifiable lvalue");
                        m_Error = true;
                    }

                    resolved = typeLHS; break;
                }

                case BinaryOperatorType::Add:
                case BinaryOperatorType::AddInPlace:
                case BinaryOperatorType::Sub:
                case BinaryOperatorType::SubInPlace:
                case BinaryOperatorType::Mul:
                case BinaryOperatorType::MulInPlace:
                case BinaryOperatorType::Div:
                case BinaryOperatorType::DivInPlace:
                case BinaryOperatorType::Mod:
                case BinaryOperatorType::ModInPlace:
                case BinaryOperatorType::And:
                case BinaryOperatorType::AndInPlace:
                case BinaryOperatorType::Or:
                case BinaryOperatorType::OrInPlace:
                case BinaryOperatorType::Xor:
                case BinaryOperatorType::XorInPlace:
                    resolved = typeLHS; break;
                
                case BinaryOperatorType::IsEq:
                case BinaryOperatorType::IsNotEq:
                case BinaryOperatorType::Less:
                case BinaryOperatorType::LessOrEq:
                case BinaryOperatorType::Greater:
                case BinaryOperatorType::GreaterOrEq:
                case BinaryOperatorType::BitAnd:
                case BinaryOperatorType::BitOr:
                    resolved = CreateVarType(m_Context, PrimitiveType::Bool, true); break;
        
                default: return nullptr;
            }
        
            binOp->ResolvedType = resolved;
            binOp->ResolvedSourceType = typeLHS;
            return resolved;
        }
        
        BLUA_ASSERT(false, "Unreachable!");
        return CreateVarType(m_Context, PrimitiveType::Invalid);
    }

    void TypeChecker::CheckNodeCompound(NodeStmt* stmt) {
        StmtCompound* compound = GetNode<StmtCompound>(stmt);

        for (size_t i = 0; i < compound->Nodes.Size; i++) {
            CheckNode(compound->Nodes.Items[i]);
        }
    }

    void TypeChecker::CheckNodeVarDecl(NodeStmt* stmt) {
        StmtVarDecl* decl = GetNode<StmtVarDecl>(stmt);
        
        std::unordered_map<std::string, Declaration>& symbolMap = (m_CurrentScope) ? m_CurrentScope->DeclaredSymbols : m_DeclaredSymbols;
        if (symbolMap.contains(fmt::format("{}", decl->Identifier))) {
            // m_Context->ReportCompilerError(stmt->Line, stmt->Column, fmt::format("Redeclaring identifier {}", decl->Identifier));
            m_Error = true;
        }
        
        VariableType* type = GetVarTypeFromString(StringView(decl->Type.Data(), decl->Type.Size()));
        symbolMap[fmt::format("{}", decl->Identifier)] = { type, stmt };
        
        if (decl->Value) {
            CheckNodeExpression(decl->Value);
        }

        decl->ResolvedType = type;
    }

    void TypeChecker::CheckNodeParamDecl(NodeStmt* stmt) {
        StmtParamDecl* decl = GetNode<StmtParamDecl>(stmt);
        
        VariableType* type = GetVarTypeFromString(StringView(decl->Type.Data(), decl->Type.Size()));
        if (m_CurrentScope) {
            m_CurrentScope->DeclaredSymbols[fmt::format("{}", decl->Identifier)] = { type, stmt };
        }
        
        decl->ResolvedType = type;
    }

    void TypeChecker::CheckNodeStructDecl(NodeStmt* stmt) {
        StmtStructDecl* decl = GetNode<StmtStructDecl>(stmt);
        
        std::string name = fmt::format("{}", decl->Identifier);
        
        StructDeclaration sd;
        sd.Identifier = decl->Identifier;
        
        std::vector<NodeStmt*> methods;

        for (size_t i = 0; i < decl->Fields.Size; i++) {
            if (StmtFieldDecl* fdecl = GetNode<StmtFieldDecl>(GetNode<NodeStmt>(decl->Fields.Items[i]))) {
                StructFieldDeclaration field;
                
                field.Identifier = fdecl->Identifier;
                field.Offset = sd.Size;
                field.ResolvedType = GetVarTypeFromString(StringView(fdecl->Type.Data(), fdecl->Type.Size()));
        
                sd.Size += GetTypeSize(field.ResolvedType);
        
                sd.Fields.push_back(field);
            } else if (StmtMethodDecl* mdecl = GetNode<StmtMethodDecl>(GetNode<NodeStmt>(decl->Fields.Items[i]))) {
                methods.push_back(GetNode<NodeStmt>(decl->Fields.Items[i]));
            }
        }
        
        m_DeclaredStructs[name] = sd;

        for (NodeStmt* decl : methods) {
            StmtMethodDecl* mdecl = GetNode<StmtMethodDecl>(decl);
            VariableType* type = GetVarTypeFromString(StringView(mdecl->ReturnType.Data(), mdecl->ReturnType.Size()));
            m_DeclaredSymbols[fmt::format("{}__{}", name, mdecl->Name)] = { type, decl };
            
            mdecl->ResolvedType = type;
            m_ActiveStruct = &m_DeclaredStructs.at(name);
            
            PushScope(type);
            
            for (size_t i = 0; i < mdecl->Parameters.Size; i++) {
                CheckNode(mdecl->Parameters.Items[i]);
            }
            
            CheckNodeCompound(mdecl->Body);
            
            PopScope();

            m_ActiveStruct = nullptr;
        }
    }

    void TypeChecker::CheckNodeFunctionDecl(NodeStmt* stmt) {
        StmtFunctionDecl* decl = GetNode<StmtFunctionDecl>(stmt);
        
        std::string name = fmt::format("{}", decl->Name);
        if (m_DeclaredSymbols.contains(name)) {
            if (m_DeclaredSymbols.at(name).Extern) {
                // m_Context->ReportCompilerError(stmt->Line, stmt->Column, fmt::format("Defining function marked extern: {}", decl->Name));
                m_Error = true;
            }

            if (StmtFunctionDecl* fdecl = GetNode<StmtFunctionDecl>(m_DeclaredSymbols.at(name).Decl)) {
                if (fdecl->Body) {
                    // m_Context->ReportCompilerError(stmt->Line, stmt->Column, fmt::format("Redefining function body: {}", fdecl->Name));
                    m_Error = true;
                }
            } else {
                // m_Context->ReportCompilerError(stmt->Line, stmt->Column, fmt::format("Redefining identifier as a different type: {}", decl->Name));
                m_Error = true;
            }
        }
        
        VariableType* type = GetVarTypeFromString(StringView(decl->ReturnType.Data(), decl->ReturnType.Size()));
        m_DeclaredSymbols[name] = { type, stmt };
        decl->ResolvedType = type;

        if (decl->Body) {
            PushScope(type);
            
            for (size_t i = 0; i < decl->Parameters.Size; i++) {
                CheckNode(decl->Parameters.Items[i]);
            }
            CheckNodeCompound(decl->Body);
            
            PopScope();
        } else {
            for (size_t i = 0; i < decl->Parameters.Size; i++) {
                CheckNode(decl->Parameters.Items[i]);
            }
        }
    }

    void TypeChecker::CheckNodeWhile(NodeStmt* stmt) {
        StmtWhile* wh = GetNode<StmtWhile>(stmt);
        
        PushScope();
        CheckNodeExpression(wh->Condition);
        CheckNodeCompound(wh->Body);
        PopScope();
    }

    void TypeChecker::CheckNodeDoWhile(NodeStmt* stmt) {
        StmtDoWhile* dowh = GetNode<StmtDoWhile>(stmt);

        PushScope();

        CheckNodeCompound(dowh->Body);
        CheckNodeExpression(dowh->Condition);

        PopScope();
    }

    void TypeChecker::CheckNodeIf(NodeStmt* stmt) {
        StmtIf* nif = GetNode<StmtIf>(stmt);

        PushScope();

        CheckNodeExpression(nif->Condition);
        CheckNodeCompound(nif->Body);
        if (nif->ElseBody) {
            PushScope();
            CheckNodeCompound(nif->ElseBody);
            PopScope();
        }

        PopScope();
    }

    void TypeChecker::CheckNodeReturn(NodeStmt* stmt) {
        StmtReturn* ret = GetNode<StmtReturn>(stmt);

        bool canReturn = true;
        if (!m_CurrentScope) {
            canReturn = false;
        } else if (m_CurrentScope->ReturnType == CreateVarType(m_Context, PrimitiveType::Invalid)) {
            canReturn = false;
        }

        if (!canReturn) {
            // m_Context->ReportCompilerError(stmt->Line, stmt->Column, "Cannot return from a non-function scope");
            m_Error = true;
            return;
        }

        VariableType* exprType = CheckNodeExpression(ret->Value);
        ConversionCost cost = GetConversionCost(m_CurrentScope->ReturnType, exprType);

        if (cost.CastNeeded) {
            if (cost.ImplicitCastPossible) {
                InsertImplicitCast(ret->Value, m_CurrentScope->ReturnType, exprType);
            } else {
                // m_Context->ReportCompilerError(stmt->Line, stmt->Column, fmt::format("Cannot implicitly cast from {} to {}", VariableTypeToString(exprType), VariableTypeToString(m_CurrentScope->ReturnType)));
                m_Error = true;
            }
        }
    }

    void TypeChecker::CheckNodeStatement(NodeStmt* stmt) {
        if (GetNode<StmtCompound>(stmt)) {
            PushScope();
            CheckNodeCompound(stmt);
            PopScope();
        } else if (GetNode<StmtVarDecl>(stmt)) {
            CheckNodeVarDecl(stmt);
        } else if (GetNode<StmtParamDecl>(stmt)) {
            CheckNodeParamDecl(stmt);
        } else if (GetNode<StmtStructDecl>(stmt)) {
            CheckNodeStructDecl(stmt);
        } else if (GetNode<StmtFunctionDecl>(stmt)) {
            CheckNodeFunctionDecl(stmt);
        } else if (GetNode<StmtWhile>(stmt)) {
            CheckNodeWhile(stmt);
        } else if (GetNode<StmtDoWhile>(stmt)) {
            CheckNodeDoWhile(stmt);
        } else if (GetNode<StmtIf>(stmt)) {
            CheckNodeIf(stmt);
        } else if (GetNode<StmtReturn>(stmt)) {
            CheckNodeReturn(stmt);
        }
    }

    void TypeChecker::CheckNode(Node* node) {
        if (NodeExpr* expr = GetNode<NodeExpr>(node)) {
            CheckNodeExpression(expr);
        } else if (NodeStmt* stmt = GetNode<NodeStmt>(node)) {
            CheckNodeStatement(stmt);
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

        if (type1->IsSigned() != type2->IsSigned()) {
            cost.SignedMismatch = true;
            cost.ImplicitCastPossible = false;
        }

        // Equal types, best possible scenario
        if (type1->Type == type2->Type) {
            cost.Type = ConversionType::None;
            cost.CastNeeded = cost.SignedMismatch;
            return cost;
        }

        if (type1->IsIntegral()) {
            if (type2->IsIntegral()) {
                if (GetTypeSize(type1) > GetTypeSize(type2)) {
                    cost.Type = ConversionType::Narrowing;
                } else {
                    cost.Type = ConversionType::Promotion;
                }
            } else if (type2->IsFloatingPoint()) {
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

        if (type1->IsFloatingPoint()) {
            if (type2->IsFloatingPoint()) {
                if (GetTypeSize(type1) > GetTypeSize(type1)) {
                    cost.Type = ConversionType::Narrowing;
                } else {
                    cost.Type = ConversionType::Promotion;
                }
            } else if (type2->IsIntegral()) {
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

    void TypeChecker::InsertImplicitCast(NodeExpr* expr, VariableType* dest, VariableType* src) {
        ExprCast* cast = m_Context->GetAllocator()->AllocateNamed<ExprCast>();
        NodeExpr* newExpr = m_Context->GetAllocator()->AllocateNamed<NodeExpr>(*expr);
        cast->Expression = newExpr;

        if (src->IsIntegral() && dest->IsIntegral()) {
            cast->ResolvedCastType = CastType::Integral;
        } else if (src->IsFloatingPoint() && dest->IsFloatingPoint()) {
            cast->ResolvedCastType = CastType::Floating;
        } else if (src->IsIntegral() && dest->IsFloatingPoint()) {
            cast->ResolvedCastType = CastType::IntegralToFloating;
        } else if (src->IsFloatingPoint() && dest->IsIntegral()) {
            cast->ResolvedCastType = CastType::FloatingToIntegral;
        } else {
            BLUA_ASSERT(false, "Impossible cast combination");
        }

        cast->ResolvedSrcType = src;
        cast->ResolvedDstType = dest;

        expr->Data = cast;
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
            if (m_DeclaredStructs.contains(isolatedType)) {
                type->Type = PrimitiveType::Structure;
                type->Data = m_DeclaredStructs.at(isolatedType);
            } else {
                ErrorUndeclaredIdentifier(StringView(isolatedType.c_str(), isolatedType.size()), 0, 0);
            }
        }

        if (array) {
            VariableType* arrType = CreateVarType(m_Context, PrimitiveType::Array);
            arrType->Data = type;
            type = arrType;
        }

        return type;
    }

    void TypeChecker::PushScope(VariableType* returnType) {
        Scope* newScope = m_Context->GetAllocator()->AllocateNamed<Scope>();
        newScope->Parent = m_CurrentScope;
        if (returnType) {
            newScope->ReturnType = returnType;
        } else {
            if (m_CurrentScope) {
                newScope->ReturnType = m_CurrentScope->ReturnType;
            }
        }

        m_CurrentScope = newScope;
    }

    void TypeChecker::PopScope() {
        m_CurrentScope = m_CurrentScope->Parent;
    }

    bool TypeChecker::IsLValue(NodeExpr* expr) {
        if (GetNode<ExprVarRef>(expr) || GetNode<ExprMember>(expr) || GetNode<ExprArrayAccess>(expr)) {
            return true;
        }

        return false;
    }

    void TypeChecker::ErrorUndeclaredIdentifier(const StringView ident, size_t line, size_t column) {
        m_Context->ReportCompilerError(line, column, fmt::format("Undeclared identifier {}", ident));
        m_Error = true;
    }

    void TypeChecker::ErrorNoMatchingFunction(const StringView func, size_t line, size_t column) {
        m_Context->ReportCompilerError(line, column, fmt::format("No matching function to call: {}", func));
        m_Error = true;
    }

} // namespace BlackLua::Internal
