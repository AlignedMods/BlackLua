#include "internal/compiler/semantic_analyzer/type_checker.hpp"

namespace BlackLua::Internal {

    TypeChecker::TypeChecker(Context* ctx, ASTNodes* nodes) {
        m_Context = ctx;
        m_Nodes = nodes;
    }

    void TypeChecker::CheckImpl() {
        // We always want to have at least one map for declarations (global space)
        m_Declarations.resize(1);

        for (Node* node : *m_Nodes) {
            HandleNode(node);
        }
    }

    TypeInfo* TypeChecker::HandleExprConstant(NodeExpr* expr) {
        ExprConstant* constant = GetNode<ExprConstant>(expr);
        expr->Type = ExprValueType::RValue;

        if (ConstantBool* cb = GetNode<ConstantBool>(constant)) {
            constant->ResolvedType = TypeInfo::Create(m_Context, PrimitiveType::Bool);
            return constant->ResolvedType;
        } else if (ConstantChar* cc = GetNode<ConstantChar>(constant)) {
            constant->ResolvedType = TypeInfo::Create(m_Context, PrimitiveType::Char, true);
            return constant->ResolvedType;
        } else if (ConstantInt* ci = GetNode<ConstantInt>(constant)) {
            constant->ResolvedType = TypeInfo::Create(m_Context, PrimitiveType::Int, !ci->Unsigned);
            return constant->ResolvedType;
        } else if (ConstantLong* cl = GetNode<ConstantLong>(constant)) {
            constant->ResolvedType = TypeInfo::Create(m_Context, PrimitiveType::Long, !cl->Unsigned);
            return constant->ResolvedType;
        } else if (ConstantFloat* cf = GetNode<ConstantFloat>(constant)) {
            constant->ResolvedType = TypeInfo::Create(m_Context, PrimitiveType::Float);
            return constant->ResolvedType;
        } else if (ConstantDouble* cd = GetNode<ConstantDouble>(constant)) {
            constant->ResolvedType = TypeInfo::Create(m_Context, PrimitiveType::Double);
            return constant->ResolvedType;
        } else if (ConstantString* cs = GetNode<ConstantString>(constant)) {
            constant->ResolvedType = TypeInfo::Create(m_Context, PrimitiveType::String);
            return constant->ResolvedType;
        }

        BLUA_UNREACHABLE();
    }

    TypeInfo* TypeChecker::HandleExprVarRef(NodeExpr* expr) {
        ExprVarRef* ref = GetNode<ExprVarRef>(expr);
        expr->Type = ExprValueType::LValue;

        std::string ident = fmt::format("{}", ref->Identifier);
        
        for (auto it = m_Declarations.rbegin(); it != m_Declarations.rend(); it++) {
            if (it->contains(ident)) {
                ref->ResolvedType = it->at(ident);
                return ref->ResolvedType;
            }
        }

        m_Context->ReportCompilerError(expr->Loc.Line, expr->Loc.Column, 
                                       expr->Range.Start.Line, expr->Range.Start.Column,
                                       expr->Range.End.Line, expr->Range.End.Column,
                                       fmt::format("Undeclared identifier \"{}\"", ref->Identifier));
        return nullptr;
    }

    TypeInfo* TypeChecker::HandleExprArrayAccess(NodeExpr* expr) { BLUA_ASSERT(false, "todo"); }
    TypeInfo* TypeChecker::HandleExprSelf(NodeExpr* expr) { BLUA_ASSERT(false, "todo"); }
    TypeInfo* TypeChecker::HandleExprMember(NodeExpr* expr) { BLUA_ASSERT(false, "todo"); }
    TypeInfo* TypeChecker::HandleExprMethodCall(NodeExpr* expr) { BLUA_ASSERT(false, "todo"); }
    TypeInfo* TypeChecker::HandleExprCall(NodeExpr* expr) { BLUA_ASSERT(false, "todo"); }
    TypeInfo* TypeChecker::HandleExprParen(NodeExpr* expr) { BLUA_ASSERT(false, "todo"); }
    TypeInfo* TypeChecker::HandleExprCast(NodeExpr* expr) { BLUA_ASSERT(false, "todo"); }
    TypeInfo* TypeChecker::HandleExprUnaryOperator(NodeExpr* expr) { BLUA_ASSERT(false, "todo"); }

    TypeInfo* TypeChecker::HandleExprBinaryOperator(NodeExpr* expr) {
        ExprBinaryOperator* binop = GetNode<ExprBinaryOperator>(expr);

        TypeInfo* LHSType = HandleNodeExpression(binop->LHS);
        TypeInfo* RHSType = HandleNodeExpression(binop->RHS);

        switch (binop->Type) {
            case BinaryOperatorType::Add:
            case BinaryOperatorType::Sub:
            case BinaryOperatorType::Mul:
            case BinaryOperatorType::Div:
            case BinaryOperatorType::Mod:
            case BinaryOperatorType::Less:
            case BinaryOperatorType::LessOrEq:
            case BinaryOperatorType::Greater:
            case BinaryOperatorType::GreaterOrEq: {
                // See which conversion would be better
                ConversionCost costLHS = GetConversionCost(LHSType, RHSType, binop->RHS->IsLValue());
                ConversionCost costRHS = GetConversionCost(RHSType, LHSType, binop->LHS->IsLValue());

                if (costLHS.CastNeeded || costRHS.CastNeeded) {
                    if (costLHS.ConversionType == ConversionType::LValueToRValue) {
                        RHSType = InsertImplicitCast(LHSType, RHSType, binop->LHS, costLHS.CastType);
                    }
                    
                    if (costRHS.ConversionType == ConversionType::LValueToRValue) {
                        LHSType = InsertImplicitCast(RHSType, LHSType, binop->RHS, costLHS.CastType);
                    }

                    if (costLHS.ConversionType == ConversionType::Promotion) {
                        RHSType = InsertImplicitCast(LHSType, RHSType, binop->LHS, costLHS.CastType);
                    } else if (costRHS.ConversionType == ConversionType::Promotion) {
                        LHSType = InsertImplicitCast(RHSType, LHSType, binop->RHS, costLHS.CastType);
                    } else {
                        m_Context->ReportCompilerError(expr->Loc.Line, expr->Loc.Column, 
                                                       expr->Range.Start.Line, expr->Range.Start.Column,
                                                       expr->Range.End.Line, expr->Range.End.Column,
                                                       fmt::format("Mismatched types '{}' and '{}', no viable implicit cast", TypeInfoToString(LHSType), TypeInfoToString(RHSType)));
                    }
                }

                binop->ResolvedType = LHSType;
                binop->ResolvedSourceType = LHSType;

                return binop->ResolvedType;
            }

            case BinaryOperatorType::AddInPlace:
            case BinaryOperatorType::SubInPlace:
            case BinaryOperatorType::MulInPlace:
            case BinaryOperatorType::DivInPlace:
            case BinaryOperatorType::ModInPlace:
            case BinaryOperatorType::Eq: {
                if (!binop->LHS->IsLValue()) {
                    m_Context->ReportCompilerError(binop->LHS->Loc.Line, binop->LHS->Loc.Column, 
                                                   binop->LHS->Range.Start.Line, binop->LHS->Range.Start.Column,
                                                   binop->LHS->Range.End.Line, binop->LHS->Range.End.Column,
                                                   "Expression must be a modifiable lvalue");
                }

                ConversionCost cost = GetConversionCost(LHSType, RHSType, binop->RHS->IsLValue());

                if (cost.CastNeeded) {
                    if (cost.ImplicitCastPossible) {
                        InsertImplicitCast(LHSType, RHSType, binop->RHS, cost.CastType);
                    } else {
                        m_Context->ReportCompilerError(binop->RHS->Loc.Line, binop->RHS->Loc.Column, 
                                                       binop->RHS->Range.Start.Line, binop->RHS->Range.Start.Column,
                                                       binop->RHS->Range.End.Line, binop->RHS->Range.End.Column,
                                                       fmt::format("Cannot implicitly cast from {} to {}", TypeInfoToString(RHSType), TypeInfoToString(LHSType)));
                    }
                }

                binop->ResolvedType = LHSType;
                binop->ResolvedSourceType = LHSType;

                return binop->ResolvedType;
            }
        }

        BLUA_UNREACHABLE();
    }

    TypeInfo* TypeChecker::HandleNodeExpression(NodeExpr* expr) {
        if (GetNode<ExprConstant>(expr)) {
            return HandleExprConstant(expr);
        } else if (GetNode<ExprVarRef>(expr)) {
            return HandleExprVarRef(expr);
        } else if (GetNode<ExprArrayAccess>(expr)) {
            return HandleExprArrayAccess(expr);
        } else if (GetNode<ExprSelf>(expr)) {
            return HandleExprSelf(expr);
        } else if (GetNode<ExprMember>(expr)) {
            return HandleExprMember(expr);
        } else if (GetNode<ExprMethodCall>(expr)) {
            return HandleExprMethodCall(expr);
        } else if (GetNode<ExprCall>(expr)) {
            return HandleExprCall(expr);
        } else if (GetNode<ExprParen>(expr)) {
            return HandleExprParen(expr);
        } else if (GetNode<ExprCast>(expr)) {
            return HandleExprCast(expr);
        } else if (GetNode<ExprUnaryOperator>(expr)) {
            return HandleExprUnaryOperator(expr);
        } else if (GetNode<ExprBinaryOperator>(expr)) {
            return HandleExprBinaryOperator(expr);
        }

        BLUA_UNREACHABLE();
    }

    void TypeChecker::HandleStmtCompound(NodeStmt* stmt) {
        StmtCompound* compound = GetNode<StmtCompound>(stmt);

        for (size_t i = 0; i < compound->Nodes.Size; i++) {
            HandleNode(compound->Nodes.Items[i]);
        }
    }

    void TypeChecker::HandleStmtVarDecl(NodeStmt* stmt) {
        StmtVarDecl* decl = GetNode<StmtVarDecl>(stmt);
        
        TypeInfo* type = GetTypeInfoFromString(StringView(decl->Type.Data(), decl->Type.Size()), true);
        decl->ResolvedType = type;

        m_Declarations.back()[fmt::format("{}", decl->Identifier)] = type;

        if (decl->Value) {
            TypeInfo* valType = HandleNodeExpression(decl->Value);
            ConversionCost cost = GetConversionCost(type, valType, decl->Value->IsLValue());

            if (cost.CastNeeded) {
                if (cost.ImplicitCastPossible) {
                    InsertImplicitCast(type, valType, decl->Value, cost.CastType);
                } else {
                    m_Context->ReportCompilerError(decl->Value->Loc.Line, decl->Value->Loc.Column, 
                                                   decl->Value->Range.Start.Line, decl->Value->Range.Start.Column,
                                                   decl->Value->Range.End.Line, decl->Value->Range.End.Column,
                                                   fmt::format("Cannot implicitly cast from {} to {}", TypeInfoToString(valType), TypeInfoToString(type)));
                    return;
                }
            }
        }
    }

    void TypeChecker::HandleStmtParamDecl(NodeStmt* stmt) {
        StmtParamDecl* decl = GetNode<StmtParamDecl>(stmt);
        
        TypeInfo* type = GetTypeInfoFromString(StringView(decl->Type.Data(), decl->Type.Size()), true);
        decl->ResolvedType = type;

        m_Declarations.back()[fmt::format("{}", decl->Identifier)] = type;
    }

    void TypeChecker::HandleStmtFunctionDecl(NodeStmt* stmt) {
        StmtFunctionDecl* decl = GetNode<StmtFunctionDecl>(stmt);

        TypeInfo* type = GetTypeInfoFromString(StringView(decl->ReturnType.Data(), decl->ReturnType.Size()), false);
        decl->ResolvedType = type;

        m_Declarations.front()[fmt::format("{}", decl->Name)] = type;

        m_Declarations.emplace_back(); // Push a new scope

        for (size_t i = 0; i < decl->Parameters.Size; i++) {
            HandleNode(decl->Parameters.Items[i]);
        }

        if (decl->Body) {
            HandleStmtCompound(decl->Body);
        }

        m_Declarations.pop_back(); // Pop the scope
    }

    void TypeChecker::HandleStmtStructDecl(NodeStmt* stmt) { BLUA_ASSERT(false, "todo"); }
    void TypeChecker::HandleStmtFieldDecl(NodeStmt* stmt) { BLUA_ASSERT(false, "todo"); }
    void TypeChecker::HandleStmtMethodDecl(NodeStmt* stmt) { BLUA_ASSERT(false, "todo"); }
    void TypeChecker::HandleStmtWhile(NodeStmt* stmt) { BLUA_ASSERT(false, "todo"); }
    void TypeChecker::HandleStmtDoWhile(NodeStmt* stmt) { BLUA_ASSERT(false, "todo"); }
    void TypeChecker::HandleStmtFor(NodeStmt* stmt) { BLUA_ASSERT(false, "todo"); }
    void TypeChecker::HandleStmtIf(NodeStmt* stmt) { BLUA_ASSERT(false, "todo"); }
    void TypeChecker::HandleStmtReturn(NodeStmt* stmt) { BLUA_ASSERT(false, "todo"); }

    void TypeChecker::HandleNodeStatement(NodeStmt* stmt) {
        if (GetNode<StmtCompound>(stmt)) {
            m_Declarations.emplace_back();
            HandleStmtCompound(stmt);
            m_Declarations.pop_back();
            return;
        } else if (GetNode<StmtVarDecl>(stmt)) {
            HandleStmtVarDecl(stmt);
            return;
        } else if (GetNode<StmtParamDecl>(stmt)) {
            HandleStmtParamDecl(stmt);
            return;
        } else if (GetNode<StmtFunctionDecl>(stmt)) {
            HandleStmtFunctionDecl(stmt);
            return;
        } else if (GetNode<StmtStructDecl>(stmt)) {
            HandleStmtStructDecl(stmt);
            return;
        } else if (GetNode<StmtFieldDecl>(stmt)) {
            HandleStmtFieldDecl(stmt);
            return;
        } else if (GetNode<StmtMethodDecl>(stmt)) {
            HandleStmtMethodDecl(stmt);
            return;
        } else if (GetNode<StmtWhile>(stmt)) {
            HandleStmtWhile(stmt);
            return;
        } else if (GetNode<StmtDoWhile>(stmt)) {
            HandleStmtDoWhile(stmt);
            return;
        } else if (GetNode<StmtFor>(stmt)) {
            HandleStmtFor(stmt);
            return;
        } else if (GetNode<StmtReturn>(stmt)) {
            HandleStmtReturn(stmt);
            return;
        }

        BLUA_UNREACHABLE();
    }

    void TypeChecker::HandleNode(Node* node) {
        if (NodeExpr* expr = GetNode<NodeExpr>(node)) {
            HandleNodeExpression(expr);
        } else if (NodeStmt* stmt = GetNode<NodeStmt>(node)) {
            HandleNodeStatement(stmt);
        }
    }

    TypeInfo* TypeChecker::GetTypeInfoFromString(StringView str, bool lvalue) {
        size_t bracket = str.Find('[');

        std::string isolatedType;
        bool array = false;

        if (bracket != StringView::npos) {
            isolatedType = fmt::format("{}", str.SubStr(0, bracket));
            array = true;
        } else {
            isolatedType = fmt::format("{}", str);
        }

        TypeInfo* type = nullptr;

        if (isolatedType == "void") { type = TypeInfo::Create(m_Context, PrimitiveType::Void); }
        if (isolatedType == "bool") { type = TypeInfo::Create(m_Context, PrimitiveType::Bool, true); }
        if (isolatedType == "char") { type = TypeInfo::Create(m_Context, PrimitiveType::Char, true); }
        if (isolatedType == "uchar") { type = TypeInfo::Create(m_Context, PrimitiveType::Char, false); }
        if (isolatedType == "short") { type = TypeInfo::Create(m_Context, PrimitiveType::Short, true); }
        if (isolatedType == "ushort") { type = TypeInfo::Create(m_Context, PrimitiveType::Short, false); }
        if (isolatedType == "int") { type = TypeInfo::Create(m_Context, PrimitiveType::Int, true); }
        if (isolatedType == "uint") { type = TypeInfo::Create(m_Context, PrimitiveType::Int, false); }
        if (isolatedType == "long") { type = TypeInfo::Create(m_Context, PrimitiveType::Long, true); }
        if (isolatedType == "ulong") { type = TypeInfo::Create(m_Context, PrimitiveType::Long, false); }
        if (isolatedType == "float") { type = TypeInfo::Create(m_Context, PrimitiveType::Float); }
        if (isolatedType == "double") { type = TypeInfo::Create(m_Context, PrimitiveType::Double); }
        if (isolatedType == "string") { type = TypeInfo::Create(m_Context, PrimitiveType::String); }

        #undef TYPE

        // Handle user defined type
        if (type->Type == PrimitiveType::Invalid) {
            BLUA_ASSERT(false, "todo");
            // if (m_DeclaredStructs.contains(isolatedType)) {
            //     type->Type = PrimitiveType::Structure;
            //     type->Data = m_DeclaredStructs.at(isolatedType);
            // } else {
            //     ErrorUndeclaredIdentifier(StringView(isolatedType.c_str(), isolatedType.size()), 0, 0);
            // }
        }

        if (array) {
            BLUA_ASSERT(false, "todo");
            // ArrayDeclaration decl;
            // decl.Type = type;
            // 
            // VariableType* arrType = CreateVarType(m_Context, PrimitiveType::Array, decl);
            // type = arrType;
        }

        return type;
    }

    ConversionCost TypeChecker::GetConversionCost(TypeInfo* dst, TypeInfo* src, bool srcLValue) {
        ConversionCost cost{};
        cost.CastNeeded = true;
        cost.ExplicitCastPossible = true;
        cost.ImplicitCastPossible = true;

        if (TypeInfo::IsEqual(src, dst)) {
            if (srcLValue) {
                cost.CastNeeded = true;
                cost.CastType = CastType::LValueToRValue;
            } else {
                cost.CastNeeded = false;
            }

            return cost;
        }

        if (src->IsIntegral()) {
            if (dst->IsIntegral()) { // Int to int
                if (src->GetSize() > dst->GetSize()) {
                    cost.ConversionType = ConversionType::Narrowing;
                    cost.CastType = CastType::Integral;
                } else if (src->GetSize() < dst->GetSize()) {
                    cost.ConversionType = ConversionType::Promotion;
                    cost.CastType = CastType::Integral;
                } else {
                    if (src->IsSigned() == dst->IsSigned()) {
                        cost.ConversionType = ConversionType::None;
                        cost.CastNeeded = false;
                    } else {
                        cost.ConversionType = ConversionType::SignChange;
                        cost.CastNeeded = true;
                    }
                }
            } else if (dst->IsFloatingPoint()) { // Int to float
                cost.ConversionType = ConversionType::Promotion;
                cost.CastType = CastType::IntegralToFloating;
            } else {
                cost.ExplicitCastPossible = false;
            }
        }

        if (src->IsFloatingPoint()) {
            if (dst->IsFloatingPoint()) { // Float to float
                if (src->GetSize() > dst->GetSize()) {
                    cost.ConversionType = ConversionType::Narrowing;
                    cost.CastType = CastType::Floating;
                } else if (src->GetSize() < dst->GetSize()) {
                    cost.ConversionType = ConversionType::Promotion;
                    cost.CastType = CastType::Floating;
                } else {
                    cost.ConversionType = ConversionType::None;
                    cost.CastNeeded = false;
                }
            } else if (dst->IsIntegral()) { // Float to int
                cost.ImplicitCastPossible = false;
                cost.ConversionType = ConversionType::Narrowing;
                cost.CastType = CastType::FloatingToIntegral;
            } else {
                cost.ExplicitCastPossible = false;
            }
        }

        return cost;
    }

    TypeInfo* TypeChecker::InsertImplicitCast(TypeInfo* dstType, TypeInfo* srcType, NodeExpr* srcExpr, CastType castType) {
        NodeExpr* copy = Allocate<NodeExpr>(*srcExpr);

        ExprImplicitCast* cast = Allocate<ExprImplicitCast>();
        cast->Expression = copy;
        cast->ResolvedCastType = castType;
        cast->ResolvedSrcType = srcType;
        cast->ResolvedDstType = dstType;

        srcExpr->Data = cast;
        srcExpr->Type = ExprValueType::RValue;

        return cast->ResolvedDstType;
    }

} // namespace BlackLua::Internal