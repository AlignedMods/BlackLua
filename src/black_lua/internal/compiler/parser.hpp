#pragma once

#include "allocator.hpp"
#include "internal/compiler/variable_type.hpp"
#include "internal/compiler/ast.hpp"
#include "internal/compiler/lexer.hpp"
#include "context.hpp"

namespace BlackLua {
    struct Context;
}

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

        Node* ParseType(bool external = false);

        Node* ParseVariableDecl(StringBuilder type); // This function expects the first token to be the identifier!
        Node* ParseFunctionDecl(StringBuilder returnType, bool external = false);
        Node* ParseExtern();

        Node* ParseStructDecl();

        Node* ParseWhile();
        Node* ParseDoWhile();
        Node* ParseFor();

        Node* ParseIf();

        Node* ParseBreak();
        Node* ParseContinue();
        Node* ParseReturn();

        // Parses either an rvalue (70, "hello world", { 7, 4, 23 }, ...) or an lvalue (variables)
        Node* ParseValue();
        BinExprType ParseOperator();
        StringBuilder ParseVariableType();
        NodeList ParseFunctionParameters();
        bool IsPrimitiveType();
        bool IsVariableType();
        size_t GetBinaryPrecedence(BinExprType type);

        Node* ParseScope();

        // Parses an expression (8 + 4, 6.3 - 4, "hello " + "world", ...)
        // NOTE: For certain reasons a varaible assignment is NOT considered an expression
        // So int var = 6; is not an expression (6 technically does count as an expression)
        Node* ParseExpression(size_t minbp = 0);

        Node* ParseStatement();

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
        Node* CreateNode(NodeType type, T* node, size_t line, size_t column) {
            Node* newNode = new Node();
            newNode->Type = type;
            newNode->Data = node;
            newNode->Line = line;
            newNode->Column = column;
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