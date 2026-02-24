#pragma once

#include "internal/compiler/ast/expr.hpp"
#include "internal/compiler/ast/stmt.hpp"
#include "internal/compiler/semantic_analyzer/type_checker.hpp"
#include "internal/vm/vm.hpp"
#include "context.hpp"

#include <unordered_map>

namespace BlackLua::Internal {

    class SemanticAnalyzer {
    private:
        struct Declaration {
            VariableType* Type = nullptr;
            NodeStmt* Decl = nullptr;
            bool Extern = false;
        };

        struct Scope {
            VariableType* ReturnType = nullptr; // This is only a valid type in function scopes!
            std::unordered_map<std::string, Declaration> DeclaredSymbols;
        };

    public:
        SemanticAnalyzer(ASTNodes* nodes, Context* ctx);

    private:
        void AnalyzeImpl();

        Node* Peek(size_t amount = 0);
        Node* Consume();

        VariableType* HandleExprConstant(NodeExpr* expr);
        VariableType* HandleExprVarRef(NodeExpr* expr);
        VariableType* HandleExprArrayAccess(NodeExpr* expr);
        VariableType* HandleExprSelf(NodeExpr* expr);
        VariableType* HandleExprMember(NodeExpr* expr);
        VariableType* HandleExprMethodCall(NodeExpr* expr);
        VariableType* HandleExprCall(NodeExpr* expr);
        VariableType* HandleExprParen(NodeExpr* expr);
        VariableType* HandleExprCast(NodeExpr* expr);
        VariableType* HandleExprUnaryOperator(NodeExpr* expr);
        VariableType* HandleExprBinaryOperator(NodeExpr* expr);

        VariableType* HandleNodeExpression(NodeExpr* expr);

        void HandleStmtCompound(NodeStmt* stmt);
        void HandleStmtVarDecl(NodeStmt* stmt);
        void HandleStmtParamDecl(NodeStmt* stmt);
        void HandleStmtFunctionDecl(NodeStmt* stmt);
        void HandleStmtStructDecl(NodeStmt* stmt);
        void HandleStmtFieldDecl(NodeStmt* stmt);
        void HandleStmtMethodDecl(NodeStmt* stmt);
        void HandleStmtWhile(NodeStmt* stmt);
        void HandleStmtDoWhile(NodeStmt* stmt);
        void HandleStmtFor(NodeStmt* stmt);
        void HandleStmtIf(NodeStmt* stmt);
        void HandleStmtReturn(NodeStmt* stmt);

        void HandleNodeStatement(NodeStmt* stmt);

        void HandleNode(Node* node);

        void PushScope(VariableType* returnType = nullptr);
        void PopScope();

        template <typename T>
        T* Allocate() {
            return m_Context->GetAllocator()->AllocateNamed<T>();
        }

        template <typename T, typename... Args>
        T* Allocate(Args&&... args) {
            return m_Context->GetAllocator()->AllocateNamed<T>(std::forward<Args>(args)...);
        }

    private:
        ASTNodes* m_Nodes = nullptr; // We modify these directly
        size_t m_Index = 0;

        std::unordered_map<std::string, Declaration> m_DeclaredSymbols;
        std::unordered_map<std::string, StructDeclaration> m_DeclaredStructs;
        
        std::vector<Scope> m_Scopes;

        StructDeclaration* m_ActiveStruct = nullptr;

        TypeChecker m_TypeChecker;
        Context* m_Context = nullptr;
    };

} // namespace BlackLua::Internal