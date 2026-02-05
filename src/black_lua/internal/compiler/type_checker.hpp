#pragma once

#include "internal/compiler/ast.hpp"
#include "internal/vm.hpp"

#include <unordered_map>

namespace BlackLua::Internal {

    // A quick note about the type checker,
    // It doesn't "generate" anything, it only modifies the AST,
    class TypeChecker {
    public:
        static TypeChecker Check(ASTNodes* nodes);
        bool IsValid() const;

    private:
        void CheckImpl();

        Node* Peek(size_t amount = 0);
        Node* Consume();

        void CheckNodeScope(Node* node);

        void CheckNodeVarDecl(Node* node);
        void CheckNodeVarArrayDecl(Node* node);
        void CheckNodeFunctionDecl(Node* node);

        void CheckNodeStructDecl(Node* node);

        void CheckNodeFunctionImpl(Node* node);

        void CheckNodeWhile(Node* node);
        void CheckNodeDoWhile(Node* node);

        void CheckNodeIf(Node* node);

        void CheckNodeReturn(Node* node);

        void CheckNodeExpression(VariableType* type, Node* node);

        void CheckNode(Node* node);

        // Gets how "expensive" it is to convert a type2 into a type1
        // You can think of this as calculating how expensive this expression is:
        // type1 var = type2;
        // The bigger the return value is, the more expensive it is to do a conversion
        // 0 is the best conversion rate (no cast needed), and 3 is the worst conversion rate (impossible conversion)
        size_t GetConversionCost(VariableType* type1, VariableType* type2);
        VariableType* GetNodeType(Node* node);
        VariableType* GetVarTypeFromString(const std::string& str);

        void WarningMismatchedTypes(VariableType type1, VariableType type2);
        void ErrorMismatchedTypes(VariableType type1, VariableType type2);
        void ErrorCannotCast(VariableType type1, VariableType type2);
        void ErrorRedeclaration(const std::string_view msg);
        void ErrorUndeclaredIdentifier(const std::string_view msg);
        void ErrorInvalidReturn();
        void ErrorNoMatchingFunction(const std::string_view func);
        void ErrorDefiningExternFunction(const std::string_view func);

    private:
        ASTNodes* m_Nodes = nullptr; // We modify these directly
        size_t m_Index = 0;
        bool m_Error = false;

        struct Declaration {
            VariableType* Type;
            Node* Decl = nullptr;
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
    };

} // namespace BlackLua::Internal