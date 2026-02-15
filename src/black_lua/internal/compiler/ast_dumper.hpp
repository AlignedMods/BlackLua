#pragma once

#include "internal/compiler/ast/expr.hpp"
#include "internal/compiler/ast/stmt.hpp"

namespace BlackLua::Internal {

    class ASTDumper {
    public:
        static ASTDumper DumpAST(ASTNodes* nodes);
        std::string& GetOutput();

    private:
        void DumpASTImpl();

        void DumpNodeList(NodeList list, size_t indentation);

        void DumpNodeExpr(NodeExpr* expr, size_t indentation);
        void DumpNodeStmt(NodeStmt* stmt, size_t indentation);

        void DumpASTNode(Node* n, size_t indentation);

    private:
        ASTNodes* m_ASTNodes = nullptr;

        std::string m_Output;
        std::string m_Indentation;
    };

} // namespace BlackLua::Internal