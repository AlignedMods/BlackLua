#pragma once

#include "internal/compiler/lexer.hpp"
#include "internal/compiler/ast/expr.hpp"
#include "internal/compiler/ast/stmt.hpp"
#include "allocator.hpp"
#include "context.hpp"

namespace BlackLua::Internal {

    class Parser {
    public:
        static Parser Parse(const Lexer::Tokens& tokens, Context* ctx);

        ASTNodes* GetNodes();
        bool IsValid() const;

    private:
        void ParseImpl();

        Token* Peek(size_t count = 0);
        Token& Consume();
        Token* TryConsume(TokenType type, StringView error);

        // Checks if the current token matches with the requested type
        // This function cannot fail
        bool Match(TokenType type);

        StringBuilder ParseVariableType();
        NodeList ParseFunctionParameters();
        bool IsPrimitiveType();
        bool IsVariableType();

        BinaryOperatorType ParseOperator();
        size_t GetBinaryPrecedence(BinaryOperatorType type);
        NodeExpr* ParseValue();
        NodeExpr* ParseExpression(size_t minbp = 0);

        NodeStmt* ParseCompound();
        NodeStmt* ParseCompoundInline();

        NodeStmt* ParseType(bool external = false);

        NodeStmt* ParseVariableDecl(StringBuilder type); // This function expects the first token to be the identifier!
        NodeStmt* ParseFunctionDecl(StringBuilder returnType, bool external = false);
        NodeStmt* ParseExtern();

        NodeStmt* ParseStructDecl();

        NodeStmt* ParseWhile();
        NodeStmt* ParseDoWhile();
        NodeStmt* ParseFor();

        NodeStmt* ParseIf();

        NodeStmt* ParseBreak();
        NodeStmt* ParseContinue();
        NodeStmt* ParseReturn();

        NodeStmt* ParseStatement();

        Node* ParseToken();

        void ErrorExpected(const StringView msg);
        void ErrorTooLarge(const StringView value);

        template <typename T>
        T* Allocate() {
            return m_Context->GetAllocator()->AllocateNamed<T>();
        }

        template <typename T, typename... Args>
        T* Allocate(Args&&... args) {
            return m_Context->GetAllocator()->AllocateNamed<T>(std::forward<Args>(args)...);
        }

        template <typename T>
        Node* CreateNode(T* node) {
            Node* newNode = m_Context->GetAllocator()->AllocateNamed<Node>();
            newNode->Data = node;
            return newNode;
        }

    private:
        ASTNodes m_Nodes;
        size_t m_Index = 0;
        Lexer::Tokens m_Tokens;
        bool m_NeedsSemi = true; // A flag to see if the current statement needs to finish with a semicolon
        bool m_Error = false;

        Context* m_Context = nullptr;
    };

} // namespace BlackLua::Internal