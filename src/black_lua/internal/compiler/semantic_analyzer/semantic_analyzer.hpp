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
            TypeInfo* Type = nullptr;
            NodeStmt* Decl = nullptr;
            bool Extern = false;
        };

        struct Scope {
            TypeInfo* ReturnType = nullptr; // This is only a valid type in function scopes!
            std::unordered_map<std::string, Declaration> DeclaredSymbols;
        };

    public:
        SemanticAnalyzer(ASTNodes* nodes, Context* ctx);

    private:
        void AnalyzeImpl();

        Node* Peek(size_t amount = 0);
        Node* Consume();

        TypeInfo* HandleExprConstant(NodeExpr* expr);
        TypeInfo* HandleExprVarRef(NodeExpr* expr);
        TypeInfo* HandleExprArrayAccess(NodeExpr* expr);
        TypeInfo* HandleExprSelf(NodeExpr* expr);
        TypeInfo* HandleExprMember(NodeExpr* expr);
        TypeInfo* HandleExprMethodCall(NodeExpr* expr);
        TypeInfo* HandleExprCall(NodeExpr* expr);
        TypeInfo* HandleExprParen(NodeExpr* expr);
        TypeInfo* HandleExprCast(NodeExpr* expr);
        TypeInfo* HandleExprUnaryOperator(NodeExpr* expr);
        TypeInfo* HandleExprBinaryOperator(NodeExpr* expr);

        TypeInfo* HandleNodeExpression(NodeExpr* expr);

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

        void PushScope(TypeInfo* returnType = nullptr);
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