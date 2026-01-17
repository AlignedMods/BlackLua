#pragma once

#include "black_lua.hpp"

#include <variant>
#include <memory>
#include <vector>
#include <unordered_map>

namespace BlackLua {

    struct CompiledSource {
        std::string Compiled;
    };

    struct LuaContext {
        static LuaContext Create();

        CompiledSource CompileFile(const std::string& path);
        CompiledSource CompileString(const std::string& source);

        // Run the compiled string in the lua interpreter (also stores the module)
        void Run(const CompiledSource& compiled, const std::string& module);

        // Sets the current active module
        void SetActiveModule(const std::string& module);

    private:
        void* m_State = nullptr; // Internal lua_State*
        std::unordered_map<std::string, int> m_Modules;
    };

    namespace Internal {

        enum class TokenType {
            Semi,
            Eq,
            LeftParen,
            RightParen,
            LeftBracket,
            RightBracket,
            LeftCurly,
            RightCurly,

            Add,
            Sub,
            Mul,
            Div,
            Mod,
            Hash,
            IsEq,
            IsNotEq,
            LessOrEq,
            GreaterOrEq,
            Less,
            Greater,

            Comma,
            Colon,
            Dot,
            DoubleDot,
            TripleDot,

            And,
            Not,
            Or,

            If,
            Else,
            ElseIf,
            Then,
            End,

            While,
            Do,
            For,
            Repeat,
            Until,
            In,

            Break,
            Return,

            True,
            False,
            Nil,

            Function,
            Local,

            IntLit, // No decimals are allowed in an integer literal!
            NumLit,
            StrLit,

            Bool,

            Int,
            Long,

            Float,
            Double,

            String,

            Identifier
        };

        inline const char* TokenTypeToString(TokenType type) {
            switch (type) {
                case TokenType::Semi: return ";";
                case TokenType::Eq: return "=";
                case TokenType::LeftParen: return "(";
                case TokenType::RightParen: return ")";
                case TokenType::LeftBracket: return "[";
                case TokenType::RightBracket: return "]";
                case TokenType::LeftCurly: return "{";
                case TokenType::RightCurly: return "}";

                case TokenType::Add: return "+";
                case TokenType::Sub: return "-";
                case TokenType::Mul: return "*";
                case TokenType::Div: return "/";
                case TokenType::Mod: return "%";
                case TokenType::Hash: return "#";
                case TokenType::IsEq: return "==";
                case TokenType::IsNotEq: return "~-";
                case TokenType::LessOrEq: return "<=";
                case TokenType::GreaterOrEq: return ">=";
                case TokenType::Less: return "<";
                case TokenType::Greater: return ">";

                case TokenType::Comma: return ",";
                case TokenType::Colon: return ":";
                case TokenType::Dot: return ".";
                case TokenType::DoubleDot: return "..";
                case TokenType::TripleDot: return "...";

                case TokenType::And: return "and";
                case TokenType::Not: return "not";
                case TokenType::Or: return "or";

                case TokenType::If: return "if";
                case TokenType::Else: return "else";
                case TokenType::ElseIf: return "elseif";
                case TokenType::Then: return "then";
                case TokenType::End: return "end";

                case TokenType::While: return "while";
                case TokenType::Do: return "do";
                case TokenType::For: return "for";
                case TokenType::Repeat: return "repeat";
                case TokenType::Until: return "until";
                case TokenType::In: return "in";

                case TokenType::Break: return "break";
                case TokenType::Return: return "return";

                case TokenType::True: return "true";
                case TokenType::False: return "false";
                case TokenType::Nil: return "nil";

                case TokenType::Function: return "function";
                case TokenType::Local: return "local";

                case TokenType::IntLit: return "int-lit";
                case TokenType::NumLit: return "num-lit";
                case TokenType::StrLit: return "str-lit";

                case TokenType::Bool: return "bool";

                case TokenType::Int: return "int";
                case TokenType::Long: return "long";

                case TokenType::Float: return "float";
                case TokenType::Double: return "double";

                case TokenType::String: return "string";

                case TokenType::Identifier: return "identifier";
            }

            BLUA_ASSERT(false, "Unreachable!");

            return nullptr;
        }

        struct Token {
            TokenType Type = TokenType::And;
            std::string Data;
            int Line = 0;
        };

        class Lexer {
        public:
            using Tokens = std::vector<Token>;

            static Lexer Parse(const std::string& source);

            const Tokens& GetTokens() const;

        private:
            void ParseImpl();

            // "Peek" at the next character in the source string,
            // if it doesn't exist return NULL
            // "if (Peek())" is the same as if (m_Index < m_Source.size())
            char* Peek();

            // "Consume" the next character in the source string
            // NOTE: Consume is not allowed to be called after the end of the source string
            char Consume();

            void AddToken(TokenType type, const std::string& data = {});

        private:
            Tokens m_Tokens;
            size_t m_Index = 0;
            std::string m_Source;
            int m_CurrentLine = 1;
        };

        enum class NodeType {
            Nil,
            Bool,
            Int,
            Number,
            String,
            InitializerList,

            Scope,

            VarDecl,
            VarRef,

            FunctionDecl,
            FunctionImpl,
            FunctionCall,

            Return,

            BinExpr
        };

        enum class VariableType {
            Invalid = 0,
            Void,
            Bool,
            Int,
            Long,
            Float,
            Double,
            String
        };

        inline const char* VariableTypeToString(VariableType type) {
            switch (type) {
                case VariableType::Invalid: return "invalid";
                case VariableType::Void: return "void";
                case VariableType::Bool: return "bool";
                case VariableType::Int: return "int";
                case VariableType::Long: return "long";
                case VariableType::Float: return "float";
                case VariableType::Double: return "double";
                case VariableType::String: return "string";
            }

            BLUA_ASSERT(false, "Unreachable!");
            return "invalid";
        }

        enum class BinExprType {
            Invalid,
            Add,
            Sub,
            Mul,
            Div,

            Less,
            LessOrEq,
            Greater,
            GreaterOrEq,

            Eq
        };

        inline const char* BinExprTypeToString(BinExprType type) {
            switch (type) {
                case BinExprType::Invalid: return "invalid";
                case BinExprType::Add: return "+";
                case BinExprType::Sub: return "-";
                case BinExprType::Mul: return "*";
                case BinExprType::Div: return "/";

                case BinExprType::Less: return "<";
                case BinExprType::LessOrEq: return "<=";
                case BinExprType::Greater: return ">";
                case BinExprType::GreaterOrEq: return ">=";

                case BinExprType::Eq: return "=";
            }

            BLUA_ASSERT(false, "Unreachable!");
            return "invalid";
        }

        struct Node; // Forward declaration of Node

        struct NodeNil {};

        struct NodeBool {
            bool Value = false;
        };

        struct NodeInt {
            int64_t Int = 0llu;
        };

        struct NodeNumber {
            double Number = 0.0;
        };

        struct NodeString {
            std::string_view String; // NOTE: This **should** be fine, if there is ever any issues with strings PLEASE check this first! (always make references to tokens!)
        };

        struct NodeInitializerList {
            std::vector<Node*> Nodes;
        };

        struct NodeScope {
            Node** Nodes = nullptr;
            size_t Capacity = 0;
            size_t Size = 0;

            inline void Append(Node* node) {
                if (Capacity == 0) {
                    Nodes = reinterpret_cast<Node**>(GetAllocator()->Allocate(sizeof(Node*) * 1));
                    Capacity = 1;
                }

                if (Size >= Capacity) {
                    Capacity *= 2;

                    Node** newNodes = reinterpret_cast<Node**>(GetAllocator()->Allocate(sizeof(Node*) * Capacity));
                    memcpy(newNodes, Nodes, sizeof(Node*) * Size);
                    Nodes = newNodes;
                }

                Nodes[Size] = node;
                Size++;
            }
        };

        struct NodeVarDecl {
            std::string_view Identifier;
            VariableType Type = VariableType::Invalid;

            Node* Value = nullptr;
        };

        struct NodeVarRef {
            std::string_view Identifier;
        };

        struct NodeFunctionDecl {
            std::string_view Name;
            std::string Signature;

            std::vector<Node*> Arguments;
            VariableType ReturnType = VariableType::Invalid;
        };

        struct NodeFunctionImpl {
            std::string_view Name;
            std::string Signature;

            std::vector<Node*> Arguments;
            VariableType ReturnType = VariableType::Invalid;

            Node* Body = nullptr; // Type is always NodeScope
        };

        struct NodeFunctionCall {
            std::string_view Name;
            std::string Signature;

            std::vector<Node*> Parameters;
        };

        struct NodeReturn {
            Node* Value = nullptr;
        };

        // NOTE: RHS can be nullptr if there is no right hand side
        struct NodeBinExpr {
            Node* LHS = nullptr;
            Node* RHS = nullptr;
            BinExprType Type = BinExprType::Invalid;
        };

        struct Node {
            NodeType Type = NodeType::Nil;
            std::variant<NodeNil*, NodeBool*, NodeInt*, NodeNumber*, NodeString*, NodeInitializerList*, 
                         NodeScope*,
                         NodeVarDecl*, NodeVarRef*,
                         NodeFunctionDecl*, NodeFunctionImpl*, NodeFunctionCall*,
                         NodeReturn*,
                         NodeBinExpr*> Data;
        };

        class Parser {
        public:
            using Nodes = std::vector<Node*>;

            static Parser Parse(const Lexer::Tokens& tokens);

            const Nodes& GetNodes() const;
            bool IsValid() const;

            

        private:
            void ParseImpl();

            Token* Peek(size_t count = 0);
            Token& Consume();

            // Checks if the current token matches with the requested type
            // This function cannot fail
            bool Match(TokenType type);

            Node* ParseType();

            Node* ParseVariableDecl(VariableType type); // This function expects the first token to be the identifier!

            Node* ParseFunctionDecl(VariableType returnType);
            Node* ParseFunctionCall();

            Node* ParseReturn();

            // Parses either an rvalue (70, "hello world", { 7, 4, 23 }, ...) or an lvalue (variables and function calls)
            Node* ParseValue();
            BinExprType ParseOperator();
            VariableType ParseVariableType();

            // Parses an expression (8 + 4, 6.3 - 4, "hello " + "world", ...)
            // NOTE: For certain reasons a varaible assignment is NOT considered an expression
            // So int var = 6; is not an expression (6 technically does count as an expression)
            Node* ParseExpression();

            Node* ParseStatement();

            void ErrorExpected(const std::string& msg);

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

        public:
            void PrintAST();

        private:
            Nodes m_Nodes;
            size_t m_Index = 0;
            Lexer::Tokens m_Tokens;
            bool m_Error = false;
        };

        // A quick note about the type checker,
        // It doesn't "generate" anything, it only modifies the AST,
        // It is also technically the only part that is not neccesary for the compiler (not optional though lolz)
        class TypeChecker {
        public:
            static TypeChecker Check(const Parser::Nodes& nodes);

            const Parser::Nodes& GetCheckedNodes() const;
            bool IsValid() const;

        private:
            void CheckImpl();

            Node* Peek(size_t amount = 0);
            Node* Consume();

            void CheckNodeVarDecl(Node* node);
            void CheckNodeVarSet(Node* node);

            void CheckNodeExpression(Node* node, VariableType type);

            void CheckNodeFunctionDecl(Node* node);
            void CheckNodeFunctionCall(Node* node);

            void CheckNode(Node* node);

            void WarningMismatchedTypes(VariableType type1, const std::string_view type2);
            void ErrorMismatchedTypes(VariableType type1, const std::string_view type2);
            void ErrorRedeclaration(const std::string_view msg);
            void ErrorUndeclaredIdentifier(const std::string_view msg);

        private:
            Parser::Nodes m_Nodes; // We modify these directly
            size_t m_Index = 0;
            bool m_Error = false;

            std::unordered_map<std::string, Node*> m_DeclaredIdentifiers;
        };

        class Emitter {
        public:
            static Emitter Emit(const Parser::Nodes& nodes);

            const std::string& GetOutput() const;

        private:
            void EmitImpl();

            Node* Peek(size_t amount = 0);
            Node* Consume();

            void EmitNodeVarDecl(Node* node);
            void EmitNodeVarSet(Node* node);

            void EmitNodeFunctionDecl(Node* node);
            void EmitNodeFunctionCall(Node* node);

            void EmitNodeReturn(Node* node);

            void EmitNodeExpression(Node* node);

            void EmitNode(Node* node);

        private:
            std::string m_Output;
            size_t m_Index = 0;
            Parser::Nodes m_Nodes;
        };

    } // namespace Internal

} // namespace BlackLua
