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
        // std::string ident;
        // ident.append(indentation, ' ');
        // m_Output += fmt::format("{}<{}:{}>", ident, expr->Line, expr->Column);
        // 
        // if (ExprConstant* con = GetNode<ExprConstant>(expr)) {
        //     if (ConstantBool* cb = GetNode<ConstantBool>(con)) {
        //         m_Output += fmt::format("BooleanConstantExpr, {}\n", cb->Value);
        //         return;
        //     }
        // 
        //     if (ConstantChar* cc = GetNode<ConstantChar>(con)) {
        //         m_Output += fmt::format("CharacterConstantExpr, {}\n", cc->Char);
        //         return;
        //     }
        // 
        //     if (ConstantInt* ci = GetNode<ConstantInt>(con)) {
        //         m_Output += fmt::format("IntegerConstantExpr, Signed: {}, {}\n", !ci->Unsigned, ci->Int);
        //         return;
        //     }
        // 
        //     if (ConstantLong* cl = GetNode<ConstantLong>(con)) {
        //         m_Output += fmt::format("LongConstantExpr, Signed: {}, {}\n", !cl->Unsigned, cl->Long);
        //         return;
        //     }
        // 
        //     if (ConstantFloat* cf = GetNode<ConstantFloat>(con)) {
        //         m_Output += fmt::format("FloatConstantExpr, {}\n", cf->Float);
        //         return;
        //     }
        // 
        //     if (ConstantDouble* cd = GetNode<ConstantDouble>(con)) {
        //         m_Output += fmt::format("DoubleConstantExpr, {}\n", cd->Double);
        //         return;
        //     }
        // 
        //     if (ConstantString* cs = GetNode<ConstantString>(con)) {
        //         m_Output += fmt::format("StringConstantExpr, {}\n", cs->String);
        //         return;
        //     }
        // }
        // 
        // if (ExprVarRef* varRef = GetNode<ExprVarRef>(expr)) {
        //     m_Output += fmt::format("VarRefExpr, {}\n", varRef->Identifier);
        //     return;
        // }
        // 
        // if (ExprArrayAccess* arrAccess = GetNode<ExprArrayAccess>(expr)) {
        //     m_Output += "ArrayAccessExpr\n";
        //     DumpNodeExpr(arrAccess->Index, indentation + 4);
        //     DumpNodeExpr(arrAccess->Parent, indentation + 4);
        //     return;
        // }
        // 
        // if (ExprMember* mem = GetNode<ExprMember>(expr)) {
        //     m_Output += fmt::format("MemberExpr, {}\n", mem->Member);
        //     DumpNodeExpr(mem->Parent, indentation + 4);
        //     return;
        // }
        // 
        // if (ExprMethodCall* call = GetNode<ExprMethodCall>(expr)) {
        //     m_Output += fmt::format("MethodCallExpr, {}\n", call->Member);
        //     DumpNodeList(call->Arguments, indentation + 4);
        //     DumpNodeExpr(call->Parent, indentation + 4);
        //     return;
        // }
        // 
        // if (ExprCall* call = GetNode<ExprCall>(expr)) {
        //     m_Output += fmt::format("CallExpr, Extern: {}, {}\n", call->Extern, call->Name);
        //     DumpNodeList(call->Arguments, indentation + 4);
        //     return;
        // }
        // 
        // if (ExprParen* paren = GetNode<ExprParen>(expr)) {
        //     m_Output += "ParenExpr\n";
        //     DumpNodeExpr(paren->Expression, indentation + 4);
        //     return;
        // }
        // 
        // if (ExprCast* cast = GetNode<ExprCast>(expr)) {
        //     m_Output += fmt::format("CastExpr '{}'\n", VariableTypeToString(cast->ResolvedDstType));
        //     DumpNodeExpr(cast->Expression, indentation + 4);
        //     return;
        // }
        // 
        // if (ExprUnaryOperator* unOp = GetNode<ExprUnaryOperator>(expr)) {
        //     m_Output += fmt::format("UnaryOperatorExpr '{}'\n", UnaryOperatorTypeToString(unOp->Type));
        //     DumpNodeExpr(unOp->Expression, indentation + 4);
        //     return;
        // }
        // 
        // if (ExprBinaryOperator* binOp = GetNode<ExprBinaryOperator>(expr)) {
        //     m_Output += fmt::format("BinaryOperatorExpr '{}'\n", BinaryOperatorTypeToString(binOp->Type));
        //     DumpNodeExpr(binOp->LHS, indentation + 4);
        //     DumpNodeExpr(binOp->RHS, indentation + 4);
        //     return;
        // }
    }

    void ASTDumper::DumpNodeStmt(NodeStmt* stmt, size_t indentation) {
        std::string ident;
        ident.append(indentation, ' ');
        m_Output += ident;
    }

    void ASTDumper::DumpASTNode(Node* n, size_t indentation) {
        if (NodeExpr* expr = GetNode<NodeExpr>(n)) {
            DumpNodeExpr(expr, indentation);
        } else if (NodeStmt* stmt = GetNode<NodeStmt>(n)) {
            DumpNodeStmt(stmt, indentation);
        }
    }

} // namespace BlackLua::Internal