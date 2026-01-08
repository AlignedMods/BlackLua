#include "compiler.hpp"

#include <string>

namespace BlackLua {

    Parser Parser::Parse(const Lexer::Tokens& tokens) {
        Parser p;
        p.m_Tokens = tokens;
        p.ParseImpl();
        
        return p;
    }

    const Parser::Nodes& Parser::GetNodes() const {
        return m_Nodes;
    }

    void Parser::ParseImpl() {
        while (Peek()) {
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

    Token Parser::Consume() {
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
            Token t = Peek()[1];

            if (t.Type == TokenType::LeftParen) {
                return ParseFunctionCall();
            }
        }

        return ParseVariable(true);
    }

    Node* Parser::ParseLocal() {
        Consume(); // Eat "local"

        if (!Peek()) {
            std::cerr << "Expected identifier after \"local\"!" << '\n';
            exit(-1);
        }

        return ParseVariable(false);
    }

    Node* Parser::ParseFunction() {
        Consume(); // Eat "function"

        NodeFunction* node = Allocate<NodeFunction>();

        if (Match(TokenType::Identifier)) {
            std::string sig = "fB5";

            Token ident = Consume();
            sig += ident.Data;
            node->Name = ident;

            if (Match(TokenType::LeftParen)) {
                Consume();
                sig += 'Z';

                while (Match(TokenType::Identifier)) {
                    Node* arg = ParseVariable(false);

                    sig += "a";
                    node->Arguments.push_back(arg);
                }

                if (!Match(TokenType::RightParen)) {
                    std::cerr << "Expected ')' after function declaration!" <<  '\n';
                    exit(-1);
                }

                Consume();

                node->Signature = sig;

                if (Peek()->Type != TokenType::End) {
                    // Get the function body
                    while (!Match(TokenType::End) && Peek()) {
                        Node* stmt = ParseStatement();

                        node->Body.push_back(stmt);
                    }
                }

                if (Match(TokenType::End)) {
                    Consume(); // Eat "end"
                } else {
                    std::cerr << "Expected end after function implementation!" <<  '\n';
                    exit(-1);
                }
            } else {
                std::cerr << "Expected '(' after \"" << ident.Data << "\"!" <<  '\n';
                exit(-1);
            }
        } else {
            std::cerr << "Expected identifier after \"local\"!" << '\n';
            exit(-1);
        }

        return CreateNode(NodeType::Function, node);
    }

    Node* Parser::ParseFunctionCall() {
        NodeFunctionCall* node = Allocate<NodeFunctionCall>();

        Token ident = Consume();
        Consume(); // Eat '('

        std::string sig = "fB5";
        sig += ident.Data;
        sig += 'Z';

        while (!Match(TokenType::RightParen) && Peek()) {
            sig += "a";

            Node* param = ParseValue();

            node->Paramaters.push_back(param);
        }

        if (!Match(TokenType::RightParen)) {
            std::cerr << "Expected ')' after function call!\n";
            exit(-1);
        }
        Consume();

        node->Signature = sig;

        return CreateNode(NodeType::FunctionCall, node);
    }

    Node* Parser::ParseVariable(bool global) {
        Token ident = Consume();

        NodeVar* node = Allocate<NodeVar>();
        node->Identifier = ident;
        node->Global = global;
        
        if (Match(TokenType::Eq)) {
            Consume();
            node->Value = ParseExpression();
        }

        return CreateNode(NodeType::Var, node);
    }

    Node* Parser::ParseValue() {
        Token value = *Peek();

        switch (value.Type) {
            case TokenType::NumLit: {
                Consume();

                double num = std::stod(value.Data.c_str());
                NodeNumber* node = Allocate<NodeNumber>(num);
                
                return CreateNode(NodeType::Number, node);
                break;
            }
            case TokenType::Identifier: {
                if (Peek(1)) {
                    Token t = Peek()[1];

                    if (t.Type == TokenType::LeftParen) {
                        return ParseFunctionCall();
                    }
                }

                Consume();
                NodeVarRef* node = Allocate<NodeVarRef>(value);

                return CreateNode(NodeType::VarRef, node);
                break;
            }
        }

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

            return CreateNode(NodeType::Binary, node);
        }

        return nullptr;
    }

    Node* Parser::ParseIf() {
        NodeIf* node = Allocate<NodeIf>();

        Consume(); // Eat "if"
        
        Node* expr = ParseExpression();
        node->Expression = expr;

        Consume(); // Eat "then"

        if (Peek()->Type != TokenType::End) {
            // Get the if body
            while (!Match(TokenType::End) && Peek()) {
                std::cout << "If body: " << TokenTypeToString(Peek()->Type) << '\n';

                Node* stmt = ParseStatement();

                node->Body.push_back(stmt);
            }
        }

        Consume(); // Eat "end"

        return CreateNode(NodeType::If, node);
    }

    Node* Parser::ParseReturn() {
        NodeReturn* node = Allocate<NodeReturn>();

        Consume(); // Eat "return"

        Node* expr = ParseExpression();

        node->Value = expr;

        return CreateNode(NodeType::Return, node);
    }

    Node* Parser::ParseStatement() {
        if (Peek()->Type == TokenType::Local) {
            return ParseLocal();
        } else if (Peek()->Type == TokenType::Function) {
            return ParseFunction();
        } else if (Peek()->Type == TokenType::Identifier) {
            return ParseIdentifier();
        } else if (Peek()->Type == TokenType::If) {
            return ParseIf();
        } else if (Peek()->Type == TokenType::Return) {
            return ParseReturn();
        } else {
            BLUA_ASSERT(false, "Unreachable!");
            Consume();
        }

        return nullptr;
    }

} // namespace BlackLua
