#include "compiler.hpp"

#include <string>

namespace BlackLua::Internal {

    Parser Parser::Parse(const Lexer::Tokens& tokens) {
        Parser p;
        p.m_Tokens = tokens;
        p.ParseImpl();
        
        return p;
    }

    const Parser::Nodes& Parser::GetNodes() const {
        return m_Nodes;
    }

    bool Parser::IsValid() const {
        return !m_Error;
    }

    void Parser::ParseImpl() {
        while (Peek() && !m_Error) {
            Node* node = ParseStatement();
            m_Nodes.push_back(node);
        }
    }

    Token* Parser::Peek(size_t count) {
        if (m_Index + count < m_Tokens.size()) {
            return &m_Tokens.at(m_Index);
        } else {
            return nullptr;
        }
    }

    Token& Parser::Consume() {
        BLUA_ASSERT(m_Index < m_Tokens.size(), "Consume out of bounds!");

        return m_Tokens.at(m_Index++);
    }

    bool Parser::Match(TokenType type) {
        if (!Peek()) {
            return false;
        }

        if (Peek()->Type == type) {
            return true;
        } else {
            return false;
        }
    }

    Node* Parser::ParseIdentifier() {
        if (Peek(1)) {
            Token& t = Peek()[1];

            if (t.Type == TokenType::LeftParen) {
                return ParseFunctionCall();
            }
        }

        Token& ident = Consume();

        NodeVarSet* node = Allocate<NodeVarSet>();
        node->Identifier = ident.Data;
        
        if (Match(TokenType::Eq)) {
            Consume();
            node->Value = ParseExpression();
        }

        if (Match(TokenType::Semi)) {
            Consume();
        } else {
            ErrorExpected("';'");
        }

        return CreateNode(NodeType::VarSet, node);
    }

    Node* Parser::ParseType() {
        VariableType type = ParseVariableType();
        Consume(); // Consume the type (ParseVariableType doesn't)

        if (Peek(1)) {
            Token t = Peek()[1];

            if (t.Type == TokenType::LeftParen) {
                return ParseFunctionDecl();
            } else {
                return ParseVariableDecl(type);
            }
        }

        return nullptr;
    }

    Node* Parser::ParseVariableDecl(VariableType type) {
        Token& ident = Consume();

        NodeVarDecl* node = Allocate<NodeVarDecl>();
        node->Identifier = ident.Data;
        node->Type = type;
        
        if (Match(TokenType::Eq)) {
            Consume();
            node->Value = ParseExpression();
        }

        if (Match(TokenType::Semi)) {
            Consume();
        } else {
            ErrorExpected("';'");
        }

        return CreateNode(NodeType::VarDecl, node);
    }

    Node* Parser::ParseFunctionDecl() {
        Token& ident = Consume();

        NodeFunctionDecl* node = Allocate<NodeFunctionDecl>();
        node->Name = ident.Data;

        if (Match(TokenType::LeftParen)) {
            Consume();

            while (!Match(TokenType::RightParen)) {
                VariableType type = ParseVariableType();
                Consume();
                Token& ident = Consume();
                
                NodeVarDecl* var = Allocate<NodeVarDecl>();
                var->Identifier = ident.Data;
                var->Type = type;
                
                if (Match(TokenType::Eq)) {
                    Consume();
                    var->Value = ParseExpression();
                }
                Node* n = CreateNode(NodeType::VarDecl, var);

                if (Match(TokenType::Comma)) {
                    Consume();
                }

                node->Arguments.push_back(n);
            }

            if (Match(TokenType::RightParen)) {
                Consume();
            } else {
                ErrorExpected("')'");
            }

            // Parse function body
            if (Match(TokenType::Semi)) {
                Consume();
            } else if (Match(TokenType::LeftCurly)) {
                node->HasBody = true;
                Consume();

                while (!Match(TokenType::RightCurly)) {
                    Node* n = ParseStatement();
                    node->Body.push_back(n);
                }

                if (Match(TokenType::RightCurly)) {
                    Consume();
                } else {
                    ErrorExpected("'}'");
                }
            } else {
                ErrorExpected("';'");
            }
        } else {
            ErrorExpected("'('");
        }

        return CreateNode(NodeType::FunctionDecl, node);
    }

    Node* Parser::ParseFunctionCall() {
        Token& ident = Consume();

        NodeFunctionCall* node = Allocate<NodeFunctionCall>();
        node->Name = ident.Data;

        if (Match(TokenType::LeftParen)) {
            Consume();

            while (!Match(TokenType::RightParen)) {
                Node* val = ParseExpression();

                if (Match(TokenType::Comma)) {
                    Consume();
                }

                node->Parameters.push_back(val);
            }

            if (Match(TokenType::RightParen)) {
                Consume();
            } else {
                ErrorExpected("')'");
            }
        } else {
            ErrorExpected("'('");
        }

        if (Match(TokenType::Semi)) {
            Consume();
        } else {
            ErrorExpected("';'");
        }

        return CreateNode(NodeType::FunctionCall, node);
    }

    Node* Parser::ParseReturn() {
        Consume(); // Consume "return"

        NodeReturn* node = Allocate<NodeReturn>();

        Node* value = ParseExpression();
        node->Value = value;

        if (Match(TokenType::Semi)) {
            Consume();
        } else {
            ErrorExpected("';'");
        }

        return CreateNode(NodeType::Return, node);
    }

    Node* Parser::ParseValue() {
        Token& value = *Peek();

        switch (value.Type) {
            case TokenType::Nil: {
                Consume();

                return CreateNode(NodeType::Nil, Allocate<NodeNil>());
                break;
            }
            case TokenType::False: {
                Consume();

                return CreateNode(NodeType::Bool, Allocate<NodeBool>(false));
                break;
            }
            case TokenType::True: {
                Consume();

                return CreateNode(NodeType::Bool, Allocate<NodeBool>(true));
                break;
            }
            case TokenType::IntLit: {
                Consume();

                int64_t num = std::stoll(value.Data.c_str());
                NodeInt* node = Allocate<NodeInt>(num);
                
                return CreateNode(NodeType::Int, node);
                break;
            }
            case TokenType::NumLit: {
                Consume();

                double num = std::stod(value.Data.c_str());
                NodeNumber* node = Allocate<NodeNumber>(num);
                
                return CreateNode(NodeType::Number, node);
                break;
            }
            case TokenType::StrLit: {
                Consume();

                std::string_view str = value.Data;
                NodeString* node = Allocate<NodeString>(str);
                
                return CreateNode(NodeType::String, node);
                break;
            }
            case TokenType::LeftCurly: {
                Consume();

                NodeInitializerList* node = Allocate<NodeInitializerList>();

                while (!Match(TokenType::RightCurly)) {
                    Node* n = ParseStatement();
                    node->Nodes.push_back(n);

                    if (Match(TokenType::Comma)) {
                        Consume();
                    }
                }

                Consume(); // Eat '}'

                return CreateNode(NodeType::InitializerList, node);
                break;
            }
            case TokenType::Identifier: {
                Consume();

                NodeVarRef* node = Allocate<NodeVarRef>();
                node->Identifier = value.Data;

                return CreateNode(NodeType::VarRef, node);
                break;
            }
        }

        ErrorExpected("value");

        return nullptr;
    }

    BinExprType Parser::ParseOperator() {
        Token op = *Peek();

        switch (op.Type) {
            case TokenType::Add: return BinExprType::Add;
            case TokenType::Sub: return BinExprType::Sub;
            case TokenType::Mul: return BinExprType::Mul;
            case TokenType::Div: return BinExprType::Div;
            case TokenType::Less: return BinExprType::Less;
            case TokenType::LessOrEq: return BinExprType::LessOrEq;
            case TokenType::Greater: return BinExprType::Greater;
            case TokenType::GreaterOrEq: return BinExprType::GreaterOrEq;
            default: return BinExprType::Invalid;
        }
    }

    VariableType Parser::ParseVariableType() {
        Token type = *Peek();

        switch (type.Type) {
            case TokenType::Bool: return VariableType::Bool;
            case TokenType::Int: return VariableType::Int;
            case TokenType::Long: return VariableType::Long;
            case TokenType::Float: return VariableType::Float;
            case TokenType::Double: return VariableType::Double;
            case TokenType::String: return VariableType::String;
            default: return VariableType::Invalid;
        }
    }

    Node* Parser::ParseExpression() {
        Node* lhsNode = ParseValue();
        BinExprType op = BinExprType::Invalid;
        if (Peek()) {
            op = ParseOperator();

            if (op == BinExprType::Invalid) {
                return lhsNode;
            } else {
                Consume(); // Eat the operator
            }
        } else {
            return lhsNode;
        }

        if (Peek()) {
            Node* rhsNode = ParseExpression();

            NodeBinExpr* node = Allocate<NodeBinExpr>();
            node->LHS = lhsNode;
            node->RHS = rhsNode;
            node->Type = op;

            return CreateNode(NodeType::BinExpr, node);
        }

        return nullptr;
    }

    Node* Parser::ParseStatement() {
        TokenType t = Peek()->Type;

        if (t == TokenType::Identifier) {
            return ParseIdentifier();
        } else if (ParseVariableType() != VariableType::Invalid) {
            return ParseType();
        } else if (t == TokenType::Return) {
            return ParseReturn();
        }

        BLUA_ASSERT(false, "Unreachable!");
        Consume();

        return nullptr;
    }

    void Parser::ErrorExpected(const std::string& msg) {
        if (m_Index > 0) {
            std::cerr << "Syntax error: Expected " << msg << " after token \"" << TokenTypeToString(m_Tokens[m_Index - 1].Type) << "\" on line " << m_Tokens[m_Index - 1].Line << "!\n";
        } else {
            std::cerr << "Syntax error: Expected " << msg << "!\n";
        }

        m_Error = true;
    }

} // namespace BlackLua::Internal
