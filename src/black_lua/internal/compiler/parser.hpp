#pragma once

#include "allocator.hpp"
#include "internal/compiler/variable_type.hpp"
#include "internal/compiler/ast.hpp"
#include "internal/compiler/lexer.hpp"

namespace BlackLua::Internal {

    class Parser {
    public:
        static Parser Parse(const Lexer::Tokens& tokens);

        ASTNodes* GetNodes();
        bool IsValid() const;

        void PrintAST();

    private:
        void ParseImpl();

        Token* Peek(size_t count = 0);
        Token& Consume();
        Token* TryConsume(TokenType type, const std::string& error);

        // Checks if the current token matches with the requested type
        // This function cannot fail
        bool Match(TokenType type);

        Node* ParseType(bool external = false);

        Node* ParseVariableDecl(const std::string& type); // This function expects the first token to be the identifier!
        Node* ParseFunctionDecl(const std::string& returnType, bool external = false);
        Node* ParseExtern();

        Node* ParseStructDecl();

        Node* ParseWhile();
        Node* ParseDoWhile();
        Node* ParseFor();

        Node* ParseIf();

        Node* ParseReturn();

        // Parses either an rvalue (70, "hello world", { 7, 4, 23 }, ...) or an lvalue (variables)
        Node* ParseValue();
        BinExprType ParseOperator();
        std::string ParseVariableType();
        bool IsPrimitiveType();
        bool IsVariableType();
        size_t GetBinaryPrecedence(BinExprType type);

        Node* ParseScope();

        // Parses an expression (8 + 4, 6.3 - 4, "hello " + "world", ...)
        // NOTE: For certain reasons a varaible assignment is NOT considered an expression
        // So int var = 6; is not an expression (6 technically does count as an expression)
        Node* ParseExpression(size_t minbp = 0);

        Node* ParseStatement();

        void ErrorExpected(const std::string& msg);
        void ErrorTooLarge(const std::string_view value);

        template <typename T>
        T* Allocate() {
            return GetAllocator()->AllocateNamed<T>();
        }

        template <typename T, typename... Args>
        T* Allocate(Args&&... args) {
            return GetAllocator()->AllocateNamed<T>(std::forward<Args>(args)...);
        }

        template <typename T>
        Node* CreateNode(NodeType type, T* node) {
            Node* newNode = new Node();
            newNode->Type = type;
            newNode->Data = node;
            return newNode;
        }

        void PrintNode(Node* node, size_t indentation);

    private:
        ASTNodes m_Nodes;
        size_t m_Index = 0;
        Lexer::Tokens m_Tokens;
        bool m_NeedsSemi = true; // A flag to see if the current statement needs to finish with a semicolon
        bool m_Error = false;
    };

} // namespace BlackLua::Internal