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

            Plus, PlusEq, PlusPlus,
            Minus, MinusEq, MinusMinus,
            Star, StarEq,
            Slash, SlashEq,
            Percent,
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

            IntLit,
            UIntLit,
            LongLit,
            ULongLit,
            FloatLit,
            DoubleLit,
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

                case TokenType::Plus: return "+";
                case TokenType::PlusEq: return "+=";
                case TokenType::PlusPlus: return "++";
                case TokenType::Minus: return "-";
                case TokenType::MinusEq: return "-=";
                case TokenType::MinusMinus: return "--";
                case TokenType::Star: return "*";
                case TokenType::StarEq: return "*=";
                case TokenType::Slash: return "/";
                case TokenType::SlashEq: return "/=";
                case TokenType::Percent: return "%";
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

                case TokenType::IntLit: return "int-lit";
                case TokenType::UIntLit: return "uint-lit";
                case TokenType::LongLit: return "long-lit";
                case TokenType::ULongLit: return "ulong-lit";
                case TokenType::FloatLit: return "float-lit";
                case TokenType::DoubleLit: return "double-lit";
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
            Bool,
            Int,
            Long,
            Float,
            Double,
            String,
            InitializerList,

            Scope,

            VarDecl,
            VarRef,

            FunctionDecl,
            FunctionImpl,

            While,
            DoWhile,
            For,

            If,

            Return,

            FunctionCallExpr,
            ParenExpr,
            CastExpr,
            UnaryExpr,
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

        enum class UnaryExprType {
            Invalid,

            Not, // "!"
            Negate // "-8.7f"
        };

        inline const char* UnaryExprTypeToString(UnaryExprType type) {
            switch (type) {
                case UnaryExprType::Invalid: return "invalid";
                
                case UnaryExprType::Not: return "!";
                case UnaryExprType::Negate: return "-";
            }

            BLUA_ASSERT(false, "Unreachable!");
            return "invalid";
        }

        enum class BinExprType {
            Invalid,
            Add, AddInPlace, AddOne,
            Sub, SubInPlace, SubOne,
            Mul, MulInPlace,
            Div, DivInPlace,

            Less,
            LessOrEq,
            Greater,
            GreaterOrEq,

            Eq,
            IsEq
        };

        inline const char* BinExprTypeToString(BinExprType type) {
            switch (type) {
                case BinExprType::Invalid: return "invalid";
                case BinExprType::Add: return "+";
                case BinExprType::AddInPlace: return "+=";
                case BinExprType::AddOne: return "++";
                case BinExprType::Sub: return "-";
                case BinExprType::SubInPlace: return "-=";
                case BinExprType::SubOne: return "--";
                case BinExprType::Mul: return "*";
                case BinExprType::MulInPlace: return "*=";
                case BinExprType::Div: return "/";
                case BinExprType::DivInPlace: return "/=";

                case BinExprType::Less: return "<";
                case BinExprType::LessOrEq: return "<=";
                case BinExprType::Greater: return ">";
                case BinExprType::GreaterOrEq: return ">=";

                case BinExprType::Eq: return "=";
                case BinExprType::IsEq: return "==";
            }

            BLUA_ASSERT(false, "Unreachable!");
            return "invalid";
        }

        struct Node; // Forward declaration of Node

        struct NodeList {
            Node** Items = nullptr;
            size_t Capacity = 0;
            size_t Size = 0;

            inline void Append(Node* node) {
                if (Capacity == 0) {
                    Items = reinterpret_cast<Node**>(GetAllocator()->Allocate(sizeof(Node*) * 1));
                    Capacity = 1;
                }

                if (Size >= Capacity) {
                    Capacity *= 2;

                    Node** newItems = reinterpret_cast<Node**>(GetAllocator()->Allocate(sizeof(Node*) * Capacity));
                    memcpy(newItems, Items, sizeof(Node*) * Size);
                    Items = newItems;
                }

                Items[Size] = node;
                Size++;
            }
        };

        struct NodeBool {
            bool Value = false;
        };

        struct NodeInt {
            int32_t Int = 0;
            bool Unsigned = false;
        };

        struct NodeLong {
            int64_t Long = 0;
            bool Unsigned = false;
        };

        struct NodeFloat {
            float Float = 0.0f;
        };

        struct NodeDouble {
            double Double = 0.0;
        };

        struct NodeString {
            std::string_view String; // NOTE: This **should** be fine, if there is ever any issues with strings PLEASE check this first! (always make references to tokens!)
        };

        struct NodeInitializerList {
            NodeList Nodes{};
        };

        struct NodeScope {
            NodeList Nodes{};
        };

        struct NodeVarDecl {
            std::string_view Identifier;
            VariableType Type = VariableType::Invalid;

            Node* Value = nullptr;

            Node* Next = nullptr; // The next variable declaration in the list (if there is a list, such as int x, y, z = 3;), this does mean that variable declarations are recursive!
        };

        struct NodeVarRef {
            std::string_view Identifier;
        };

        struct NodeFunctionDecl {
            std::string_view Name;
            std::string Signature;

            NodeList Arguments;
            VariableType ReturnType = VariableType::Invalid;
        };

        struct NodeFunctionImpl {
            std::string_view Name;

            NodeList Arguments;
            VariableType ReturnType = VariableType::Invalid;

            Node* Body = nullptr; // Type is always NodeScope
        };

        struct NodeWhile {
            Node* Condition = nullptr;

            Node* Body = nullptr; // Type is always NodeScope
        };

        struct NodeDoWhile {
            Node* Body = nullptr; // Type is always NodeScope

            Node* Condition = nullptr;
        };

        struct NodeFor {
            Node* Prologue = nullptr; // int i = 0;
            Node* Condition = nullptr; // i < 5;
            Node* Epilogue = nullptr; // i += 1;

            Node* Body = nullptr;
        };

        struct NodeIf {
            Node* Condition = nullptr;

            Node* Body = nullptr;
        };

        struct NodeReturn {
            Node* Value = nullptr;
        };

        struct NodeFunctionCallExpr {
            std::string_view Name;

            std::vector<Node*> Parameters;
        };

        struct NodeParenExpr {
            Node* Expression = nullptr;
        };

        struct NodeCastExpr {
            VariableType Type = VariableType::Invalid;
            Node* Expression = nullptr;
        };

        struct NodeUnaryExpr {
            Node* Expression = nullptr;
            UnaryExprType Type = UnaryExprType::Invalid;
        };

        struct NodeBinExpr {
            Node* LHS = nullptr;
            Node* RHS = nullptr;
            BinExprType Type = BinExprType::Invalid;
        };

        struct Node {
            NodeType Type = NodeType::Bool;
            std::variant<NodeBool*, NodeInt*, NodeLong*, NodeFloat*, NodeDouble*, NodeString*, NodeInitializerList*, 
                         NodeScope*,
                         NodeVarDecl*, NodeVarRef*,
                         NodeFunctionDecl*, NodeFunctionImpl*,
                         NodeWhile*, NodeDoWhile*, NodeFor*,
                         NodeIf*,
                         NodeReturn*,
                         NodeFunctionCallExpr*, NodeParenExpr*, NodeCastExpr*, NodeUnaryExpr*, NodeBinExpr*> Data;
        };

        class Parser {
        public:
            using Nodes = std::vector<Node*>;

            static Parser Parse(const Lexer::Tokens& tokens);

            const Nodes& GetNodes() const;
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

            Node* ParseType();

            Node* ParseVariableDecl(VariableType type); // This function expects the first token to be the identifier!
            Node* ParseFunctionDecl(VariableType returnType);

            Node* ParseWhile();
            Node* ParseDoWhile();
            Node* ParseFor();

            Node* ParseIf();

            Node* ParseReturn();

            // Parses either an rvalue (70, "hello world", { 7, 4, 23 }, ...) or an lvalue (variables)
            Node* ParseValue();
            BinExprType ParseOperator();
            VariableType ParseVariableType();
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
            Nodes m_Nodes;
            size_t m_Index = 0;
            Lexer::Tokens m_Tokens;
            bool m_Error = false;
        };

        // A quick note about the type checker,
        // It doesn't "generate" anything, it only modifies the AST,
        class TypeChecker {
        public:
            static TypeChecker Check(const Parser::Nodes& nodes);

            const Parser::Nodes& GetCheckedNodes() const;
            bool IsValid() const;

        private:
            void CheckImpl();

            Node* Peek(size_t amount = 0);
            Node* Consume();

            void CheckNodeScope(Node* node);

            void CheckNodeVarDecl(Node* node);

            void CheckNodeFunctionImpl(Node* node);

            void CheckNodeReturn(Node* node);

            void CheckNodeExpression(VariableType type, Node* node);

            void CheckNode(Node* node);

            // Gets how "expensive" it is to convert a type2 into a type1
            // You can think of this as calculating how expensive this expression is:
            // type1 var = type2;
            // The bigger the return value is, the more expensive it is to do a conversion
            // 0 is the best conversion rate (no cast needed), and 3 is the worst conversion rate (impossible conversion)
            size_t GetConversionCost(VariableType type1, VariableType type2);
            VariableType GetNodeType(Node* node);

            void WarningMismatchedTypes(VariableType type1, VariableType type2);
            void ErrorMismatchedTypes(VariableType type1, VariableType type2);
            void ErrorCannotCast(VariableType type1, VariableType type2);
            void ErrorRedeclaration(const std::string_view msg);
            void ErrorUndeclaredIdentifier(const std::string_view msg);
            void ErrorInvalidReturn();
            void ErrorNoMatchingFunction(const std::string_view func);

        private:
            Parser::Nodes m_Nodes; // We modify these directly
            size_t m_Index = 0;
            bool m_Error = false;

            struct Declaration {
                VariableType Type = VariableType::Invalid;
                Node* Decl = nullptr;
            };

            std::unordered_map<std::string, Declaration> m_DeclaredSymbols;

            struct Scope {
                Scope* Parent = nullptr;
                VariableType ReturnType = VariableType::Invalid; // This is only a valid type in function scopes!
                std::unordered_map<std::string, Declaration> DeclaredSymbols;
            };

            Scope* m_CurrentScope = nullptr;
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
