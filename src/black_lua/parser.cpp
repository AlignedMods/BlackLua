#include "compiler.hpp"

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

        NodeFunction* node = new NodeFunction();

        if (Match(TokenType::Identifier)) {
            std::string sig;

            Token ident = Consume();
            sig += ident.Data;

            if (Match(TokenType::LeftParen)) {
                Consume();
                sig += '(';

                while (Match(TokenType::Identifier)) {
                    Node* arg = ParseVariable(false);

                    sig += '_';
                    node->Arguments.push_back(arg);
                }

                if (!Match(TokenType::RightParen)) {
                    std::cerr << "Expected ')' after function declaration!" <<  '\n';
                    exit(-1);
                }

                Consume();

                sig += ')';

                node->Signature = sig;

                if (Peek()->Type != TokenType::End) {
                    // Get the function body
                    while (!Match(TokenType::End) && Peek()) {
                        Node* stmt = ParseStatement();

                        node->Body.push_back(stmt);
                    }
                }

                if (Peek()) {
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

    Node* Parser::ParseVariable(bool global) {
        Token ident = Consume();

        NodeVar* node = new NodeVar();
        node->Identifier = ident;
        node->Global = global;
        
        if (Match(TokenType::Eq)) {
            Consume();
            node->Value = ParseExpression();
        }

        return CreateNode(NodeType::Var, node);
    }

    Node* Parser::ParseValue() {
        Token value = Consume();

        switch (value.Type) {
            case TokenType::NumLit: {
                double num = std::stod(value.Data);
                NodeNumber* node = new NodeNumber(num);
                
                return CreateNode(NodeType::Number, node);
                break;
            }
            case TokenType::Identifier: {
                NodeVarRef* node = new NodeVarRef(value);

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

            NodeBinExpr* node = new NodeBinExpr();
            node->LHS = lhsNode;
            node->RHS = rhsNode;
            node->Type = op;

            return CreateNode(NodeType::Binary, node);
        }

        return nullptr;
    }

    Node* Parser::ParseStatement() {
        if (Peek()->Type == TokenType::Local) {
            return ParseLocal();
        } else if (Peek()->Type == TokenType::Function) {
            return ParseFunction();
        } else if (Peek()->Type == TokenType::Identifier) {
            return ParseIdentifier();
        } else {
            BLUA_ASSERT(false, "Unreachable!");
            Consume();
        }
    }

} // namespace BlackLua