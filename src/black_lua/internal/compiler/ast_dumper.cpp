#include "internal/compiler/ast_dumper.hpp"

namespace BlackLua::Internal {

    ASTDumper ASTDumper::DumpAST(ASTNodes* nodes) {
        ASTDumper d;
        d.m_ASTNodes = nodes;
        d.DumpASTImpl();
        return d;
    }

    std::string& ASTDumper::GetOutput() {
        return m_Output;
    }

    void ASTDumper::DumpASTImpl() {
        for (Node* node : *m_ASTNodes) {
            DumpASTNode(node, 0);
        }
    }

    void ASTDumper::DumpNodeList(NodeList list, size_t indentation) {
        for (size_t i = 0; i < list.Size; i++) {
            DumpASTNode(list.Items[i], indentation);
        }
    }

    void ASTDumper::DumpNodeExpr(NodeExpr* expr, size_t indentation) {
        std::string ident;
        ident.append(indentation, ' ');
        m_Output += fmt::format("{}<{}:{}> ", ident, expr->Line, expr->Column);
        
        if (ExprConstant* con = GetNode<ExprConstant>(expr)) {
            if (ConstantBool* cb = GetNode<ConstantBool>(con)) {
                m_Output += fmt::format("BooleanConstantExpr {} '{}'\n", cb->Value, VariableTypeToString(con->ResolvedType));
                return;
            }
        
            if (ConstantChar* cc = GetNode<ConstantChar>(con)) {
                m_Output += fmt::format("CharacterConstantExpr {} '{}'\n", cc->Char, VariableTypeToString(con->ResolvedType));
                return;
            }
        
            if (ConstantInt* ci = GetNode<ConstantInt>(con)) {
                m_Output += fmt::format("IntegerConstantExpr {} '{}'\n", ci->Int, VariableTypeToString(con->ResolvedType));
                return;
            }
        
            if (ConstantLong* cl = GetNode<ConstantLong>(con)) {
                m_Output += fmt::format("IntegerConstantExpr {} '{}'\n", cl->Long, VariableTypeToString(con->ResolvedType));
                return;
            }
        
            if (ConstantFloat* cf = GetNode<ConstantFloat>(con)) {
                m_Output += fmt::format("FloatingPointConstantExpr {} '{}'\n", cf->Float, VariableTypeToString(con->ResolvedType));
                return;
            }
        
            if (ConstantDouble* cd = GetNode<ConstantDouble>(con)) {
                m_Output += fmt::format("FloatingPointConstantExpr {} '{}'\n", cd->Double, VariableTypeToString(con->ResolvedType));
                return;
            }
        
            if (ConstantString* cs = GetNode<ConstantString>(con)) {
                m_Output += fmt::format("StringConstantExpr \"{}\" '{}'\n", cs->String, VariableTypeToString(con->ResolvedType));
                return;
            }
        }
        
        if (ExprVarRef* varRef = GetNode<ExprVarRef>(expr)) {
            m_Output += fmt::format("VarRefExpr \"{}\" '{}'\n", varRef->Identifier, VariableTypeToString(varRef->ResolvedType));
            return;
        }
        
        if (ExprArrayAccess* arrAccess = GetNode<ExprArrayAccess>(expr)) {
            m_Output += fmt::format("ArrayAccessExpr '{}'\n", VariableTypeToString(arrAccess->ResolvedType));
            DumpNodeExpr(arrAccess->Index, indentation + 4);
            DumpNodeExpr(arrAccess->Parent, indentation + 4);
            return;
        }
        
        if (ExprMember* mem = GetNode<ExprMember>(expr)) {
            m_Output += fmt::format("MemberExpr {}\n", mem->Member);
            DumpNodeExpr(mem->Parent, indentation + 4);
            return;
        }
        
        if (ExprMethodCall* call = GetNode<ExprMethodCall>(expr)) {
            m_Output += fmt::format("MethodCallExpr {}\n", call->Member);
            DumpNodeList(call->Arguments, indentation + 4);
            DumpNodeExpr(call->Parent, indentation + 4);
            return;
        }
        
        if (ExprCall* call = GetNode<ExprCall>(expr)) {
            m_Output += fmt::format("CallExpr \"{}\" '{}' {}\n", call->Name, VariableTypeToString(call->ResolvedReturnType), (call->Extern) ? "extern" : "");
            DumpNodeList(call->Arguments, indentation + 4);
            return;
        }
        
        if (ExprParen* paren = GetNode<ExprParen>(expr)) {
            m_Output += "ParenExpr\n";
            DumpNodeExpr(paren->Expression, indentation + 4);
            return;
        }
        
        if (ExprCast* cast = GetNode<ExprCast>(expr)) {
            m_Output += fmt::format("CastExpr '{}'\n", VariableTypeToString(cast->ResolvedDstType));
            DumpNodeExpr(cast->Expression, indentation + 4);
            return;
        }
        
        if (ExprUnaryOperator* unOp = GetNode<ExprUnaryOperator>(expr)) {
            m_Output += fmt::format("UnaryOperatorExpr '{}'\n", UnaryOperatorTypeToString(unOp->Type));
            DumpNodeExpr(unOp->Expression, indentation + 4);
            return;
        }
        
        if (ExprBinaryOperator* binOp = GetNode<ExprBinaryOperator>(expr)) {
            m_Output += fmt::format("BinaryOperatorExpr '{}'\n", BinaryOperatorTypeToString(binOp->Type));
            DumpNodeExpr(binOp->LHS, indentation + 4);
            DumpNodeExpr(binOp->RHS, indentation + 4);
            return;
        }
    }

    void ASTDumper::DumpNodeStmt(NodeStmt* stmt, size_t indentation) {
        std::string ident;
        ident.append(indentation, ' ');
        m_Output += fmt::format("{}<{}:{}> ", ident, stmt->Line, stmt->Column);

        if (StmtCompound* compound = GetNode<StmtCompound>(stmt)) {
            m_Output += fmt::format("CompoundStmt\n");
            DumpNodeList(compound->Nodes, indentation + 4);
            return;
        }

        if (StmtVarDecl* decl = GetNode<StmtVarDecl>(stmt)) {
            m_Output += fmt::format("VarDeclStmt \"{}\" '{}'\n", decl->Identifier, VariableTypeToString(decl->ResolvedType));
            if (decl->Value) {
                DumpNodeExpr(decl->Value, indentation + 4);
            }
            return;
        }

        if (StmtParamDecl* decl = GetNode<StmtParamDecl>(stmt)) {
            m_Output += fmt::format("ParamDeclStmt \"{}\", '{}'\n", decl->Identifier, VariableTypeToString(decl->ResolvedType));
            return;
        }

        if (StmtFunctionDecl* decl = GetNode<StmtFunctionDecl>(stmt)) {
            m_Output += fmt::format("FunctionDeclStmt \"{}\" '{}' {}\n", decl->Name, VariableTypeToString(decl->ResolvedType), (decl->Extern) ? "extern" : "");
            DumpNodeList(decl->Parameters, indentation + 4);
            if (decl->Body) {
                DumpNodeStmt(decl->Body, indentation + 4);
            }
            return;
        }

        if (StmtStructDecl* decl = GetNode<StmtStructDecl>(stmt)) {
            m_Output += fmt::format("StructDeclStmt \"{}\"\n", decl->Identifier);
            for (size_t i = 0; i < decl->Fields.Size; i++) {
                DumpASTNode(decl->Fields.Items[i], indentation);
            }
            return;
        }

        if (StmtFieldDecl* decl = GetNode<StmtFieldDecl>(stmt)) {
            m_Output += fmt::format("FieldDeclStmt \"{}\"\n", decl->Identifier);
            return;
        }

        if (StmtMethodDecl* decl = GetNode<StmtMethodDecl>(stmt)) {
            m_Output += fmt::format("MethodDeclStmt \"{}\" '{}'\n", decl->Name, VariableTypeToString(decl->ResolvedType));
            DumpNodeList(decl->Parameters, indentation + 4);
            if (decl->Body) {
                DumpNodeStmt(decl->Body, indentation + 4);
            }
            return;
        }

        if (StmtWhile* wh = GetNode<StmtWhile>(stmt)) {
            m_Output += "WhileStmt\n";
            DumpNodeExpr(wh->Condition, indentation + 4);
            DumpNodeStmt(wh->Body, indentation + 4);
            return;
        }

        if (StmtDoWhile* wh = GetNode<StmtDoWhile>(stmt)) {
            m_Output += "DoWhileStmt\n";
            DumpNodeExpr(wh->Condition, indentation + 4);
            DumpNodeStmt(wh->Body, indentation + 4);
            return;
        }

        if (StmtFor* fo = GetNode<StmtFor>(stmt)) {
            m_Output += "ForStmt\n";
            DumpNodeStmt(fo->Prologue, indentation + 4);
            DumpNodeExpr(fo->Condition, indentation + 4);
            DumpNodeExpr(fo->Epilogue, indentation + 4);
            DumpNodeStmt(fo->Body, indentation + 4);
            return;
        }

        if (StmtIf* i = GetNode<StmtIf>(stmt)) {
            m_Output += "IfStmt\n";
            DumpNodeExpr(i->Condition, indentation + 4);
            DumpNodeStmt(i->Body, indentation + 4);
            return;
        }

        if (StmtReturn* ret = GetNode<StmtReturn>(stmt)) {
            m_Output += "ReturnStmt\n";
            DumpNodeExpr(ret->Value, indentation + 4);
            return;
        }
    }

    void ASTDumper::DumpASTNode(Node* n, size_t indentation) {
        if (NodeExpr* expr = GetNode<NodeExpr>(n)) {
            DumpNodeExpr(expr, indentation);
        } else if (NodeStmt* stmt = GetNode<NodeStmt>(n)) {
            DumpNodeStmt(stmt, indentation);
        }
    }

} // namespace BlackLua::Internal