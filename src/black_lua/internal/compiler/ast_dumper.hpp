#pragma once

#include "internal/compiler/ast.hpp"

namespace BlackLua::Internal {

    class ASTDumper {
    public:
        static ASTDumper DumpAST(ASTNodes* nodes);
        std::string& GetOutput();

    private:
        void DumpASTImpl();
        void DumpASTNode(Node* n, size_t indentation = 0);

    private:
        ASTNodes* m_ASTNodes = nullptr;

        std::string m_Output;
        std::string m_Indentation;
    };

} // namespace BlackLua::Internal