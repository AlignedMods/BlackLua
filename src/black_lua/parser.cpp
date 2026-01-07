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

        if (!Peek()) {
            std::cerr << "Expected identifier after \"local\"!" << '\n';
            exit(-1);
        }

        Token ident = Consume();


    }

    Node* Parser::ParseVariable(bool global) {
        Token ident = Consume();

        NodeVar* node = new NodeVar();
        node->Identifier = ident;
        node->Global = global;
        
        if (Peek()) {
            Token t = *Peek();

            if (t.Type == TokenType::Eq) {
                Consume();
                node->Value = ParseExpression();
            }
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
    }

    BinExprType Parser::ParseOperator() {
        Token op = Consume();

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
        } else {
            return lhsNode;
        }

        if (Peek() && op != BinExprType::Invalid) {
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
            ParseIdentifier();
        }
    }

} // namespace BlackLua