#pragma once

#include "internal/compiler/ast/expr.hpp"
#include "internal/compiler/ast/stmt.hpp"
#include "internal/vm.hpp"

#include <unordered_map>

namespace BlackLua {
    struct Context;
}

namespace BlackLua::Internal {

    enum class ConversionType {
        None,
        Promotion,
        Narrowing
    };

    struct ConversionCost {
        ConversionType Type = ConversionType::None;

        bool CastNeeded = false;
        bool SignedMismatch = false;
        bool ImplicitCastPossible = false;
        bool ExplicitCastPossible = false;

        VariableType* Source = nullptr;
        VariableType* Destination = nullptr;
    };

    // A quick note about the type checker,
    // It doesn't "generate" anything, it only modifies the AST,
    class TypeChecker {
    public:
        static TypeChecker Check(ASTNodes* nodes, Context* ctx);
        bool IsValid() const;

    private:
        void CheckImpl();

        Node* Peek(size_t amount = 0);
        Node* Consume();

        VariableType* CheckNodeExpression(NodeExpr* expr);

        void CheckNodeScope(NodeStmt* stmt);

        void CheckNodeVarDecl(NodeStmt* stmt);
        void CheckNodeParamDecl(NodeStmt* stmt);

        void CheckNodeStructDecl(NodeStmt* stmt);

        void CheckNodeFunctionDecl(NodeStmt* stmt);

        void CheckNodeWhile(NodeStmt* stmt);
        void CheckNodeDoWhile(NodeStmt* stmt);

        void CheckNodeIf(NodeStmt* stmt);

        void CheckNodeReturn(NodeStmt* stmt);

        void CheckNodeStatement(NodeStmt* stmt);

        void CheckNode(Node* node);

        ConversionCost GetConversionCost(VariableType* type1, VariableType* type2);
        void InsertImplicitCast(NodeExpr* expr, VariableType* dest, VariableType* src);
        VariableType* GetVarTypeFromString(StringView str);

        bool IsLValue(NodeExpr* expr);

        void ErrorUndeclaredIdentifier(const StringView ident, size_t line, size_t column);
        void ErrorNoMatchingFunction(const StringView func, size_t line, size_t column);

    private:
        ASTNodes* m_Nodes = nullptr; // We modify these directly
        size_t m_Index = 0;
        bool m_Error = false;

        struct Declaration {
            VariableType* Type;
            NodeStmt* Decl = nullptr;
            bool Extern = false;
        };

        std::unordered_map<std::string, Declaration> m_DeclaredSymbols;
        std::unordered_map<std::string, StructDeclaration> m_DeclaredStructs;

        struct Scope {
            Scope* Parent = nullptr;
            VariableType* ReturnType; // This is only a valid type in function scopes!
            std::unordered_map<std::string, Declaration> DeclaredSymbols;
        };

        Scope* m_CurrentScope = nullptr;

        Context* m_Context = nullptr;
    };

} // namespace BlackLua::Internal