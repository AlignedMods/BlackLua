#pragma once

#include "black_lua.hpp"

#include <variant>
#include <memory>

namespace BlackLua {

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

        NumLit,
        StrLit,

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

            case TokenType::NumLit: return "num-lit";
            case TokenType::StrLit: return "str-lit";

            case TokenType::Identifier: return "identifier";
        }

        BLUA_ASSERT(false, "Unreachable!");

        return nullptr;
    }

    struct Token {
        TokenType Type = TokenType::And;
        std::string Data;
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
    };

    enum class NodeType {
        Number,
        Var,
        VarRef,
        Function,
        FunctionCall,
        Binary, // Binary expression (5 + 3 / 5, 3 * 4, 9 - 3, ...)
        If,
        Return
    };

    enum class BinExprType {
        Invalid,
        Add,
        Sub,
        Mul,
        Div,

        Less,
        LessOrEq,
        Greater,
        GreaterOrEq
    };

    struct Node; // Forward declaration of Expr

    struct NodeNumber {
        double Number = 0.0;
    };

    struct NodeVar {
        bool Global = false;
        Token Identifier;
        Node* Value = nullptr;
    };

    struct NodeVarRef {
        Token Identifier;
    };

    struct NodeFunction {
        Token Name;
        std::string Signature;
        std::vector<Node*> Arguments;
        std::vector<Node*> Body;
    };

    struct NodeFunctionCall {
        std::string Signature;
        std::vector<Node*> Paramaters;
    };

    // NOTE: RHS can be nullptr if there is no right hand side
    struct NodeBinExpr {
        Node* LHS = nullptr;
        Node* RHS = nullptr;
        BinExprType Type = BinExprType::Invalid;
    };

    struct NodeIf {
        Node* Expression = nullptr;
        std::vector<Node*> Body;
    };

    struct NodeReturn {
        Node* Value = nullptr;
    };

    struct Node {
        NodeType Type = NodeType::Number;
        std::variant<NodeNumber*, NodeVar*, NodeVarRef*, NodeFunction*, NodeFunctionCall*, NodeBinExpr*, NodeIf*, NodeReturn*> Data;
    };

    class Parser {
    public:
        using Nodes = std::vector<Node*>;

        static Parser Parse(const Lexer::Tokens& tokens);

        const Nodes& GetNodes() const;

    private:
        void ParseImpl();

        Token* Peek(size_t count = 0);
        Token Consume();

        // Checks if the current token matches with the requested type
        // This function cannot fail
        bool Match(TokenType type);

        Node* ParseIdentifier();
        Node* ParseLocal();
        Node* ParseFunction();
        Node* ParseFunctionCall();
        Node* ParseVariable(bool global); // This function expects the first token to be the identifier!

        Node* ParseValue();
        BinExprType ParseOperator();

        Node* ParseIf();

        Node* ParseReturn();

        Node* ParseExpression();

        Node* ParseStatement();

        template <typename T>
        Node* CreateNode(NodeType type, T* node) {
            Node* newNode = new Node();
            newNode->Type = type;
            newNode->Data = node;
            return newNode;
        }

    private:
        Nodes m_Nodes;
        size_t m_Index = 0;
        Lexer::Tokens m_Tokens;
    };

    class Emitter {
    public:
        static Emitter Emit(const Parser::Nodes& nodes);

        const std::string& GetOutput() const;

    private:
        void EmitImpl();

        Node* Peek(size_t amount = 0);
        Node* Consume();

        void EmitNode(Node* node);

        void EmitNodeExpression(Node* node);
        void EmitNodeVar(Node* node);

        void EmitNodeFunction(Node* node);
        void EmitNodeFunctionCall(Node* node);

        void EmitNodeIf(Node* node);

        void EmitNodeReturn(Node* node);

    private:
        std::string m_Output;
        size_t m_Index = 0;
        Parser::Nodes m_Nodes;
    };

} // namespace BlackLua
