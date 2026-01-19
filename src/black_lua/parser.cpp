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

    void Parser::PrintAST() {
        for (const auto& n : m_Nodes) {
            PrintNode(n, 0);
        }
    }

    void Parser::ParseImpl() {
        while (Peek() && !m_Error) {
            Node* node = ParseStatement();
            m_Nodes.push_back(node);
        }
    }

    Token* Parser::Peek(size_t count) {
        // While the count is a size_t, you are still allowed to use -1
        // Even if you pass -1 the actual data underneath is the same
        if (m_Index + count < m_Tokens.size()) {
            return &m_Tokens.at(m_Index + count);
        } else {
            return nullptr;
        }
    }

    Token& Parser::Consume() {
        BLUA_ASSERT(m_Index < m_Tokens.size(), "Consume out of bounds!");

        return m_Tokens.at(m_Index++);
    }

    Token* Parser::TryConsume(TokenType type, const std::string& error) {
        if (Match(type)) {
            Token& t = Consume();
            return &t;
        }

        ErrorExpected(error);

        return nullptr;
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

    Node* Parser::ParseType() {
        VariableType type = ParseVariableType();
        Consume(); // Consume the type (ParseVariableType doesn't)

        if (Peek(1)) {
            Token t = *Peek(1);

            if (t.Type == TokenType::LeftParen) {
                return ParseFunctionDecl(type);
            } else {
                Node* node = ParseVariableDecl(type);
                Node* current = node;

                while (Match(TokenType::Comma)) {
                    Consume();
                    
                    Node* next = ParseVariableDecl(type);
                    std::get<NodeVarDecl*>(current->Data)->Next = next;
                    current = next;
                }

                return node;
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

        return CreateNode(NodeType::VarDecl, node);
    }

    Node* Parser::ParseFunctionDecl(VariableType returnType) {
        Token& ident = Consume();

        Node* finalNode = nullptr;
        std::vector<Node*> args;

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

                args.push_back(n);
            }

            TryConsume(TokenType::RightParen, "')'");

            if (Match(TokenType::LeftCurly)) {
                NodeFunctionImpl* node = Allocate<NodeFunctionImpl>();
                node->Name = ident.Data;
                node->ReturnType = returnType;
                node->Arguments = args;

                node->Body = CreateNode(NodeType::Scope, Allocate<NodeScope>());

                Consume();

                while (!Match(TokenType::RightCurly)) {
                    Node* n = ParseStatement();
                    std::get<NodeScope*>(node->Body->Data)->Nodes.Append(n);
                }

                TryConsume(TokenType::RightCurly, "'}'");

                finalNode = CreateNode(NodeType::FunctionImpl, node);
            } else {
                NodeFunctionDecl* node = Allocate<NodeFunctionDecl>();
                node->Name = ident.Data;
                node->ReturnType = returnType;
                node->Arguments = args;

                finalNode = CreateNode(NodeType::FunctionDecl, node);
            }
        } else {
            ErrorExpected("'('");
        }

        return finalNode;
    }

    Node* Parser::ParseWhile() {
        Consume(); // Consume "while"

        NodeWhile* node = Allocate<NodeWhile>();

        TryConsume(TokenType::LeftParen, "'('");
        node->Condition = ParseExpression();
        TryConsume(TokenType::RightParen, "')'");
        TryConsume(TokenType::LeftCurly, "'}'");

        node->Body = CreateNode(NodeType::Scope, Allocate<NodeScope>());

        while (!Match(TokenType::RightCurly)) {
            Node* n = ParseStatement();
            std::get<NodeScope*>(node->Body->Data)->Nodes.Append(n);
        }

        TryConsume(TokenType::RightCurly, "'}'");

        return CreateNode(NodeType::While, node);
    }

    Node* Parser::ParseDoWhile() {
        Consume(); // Consume "do"

        NodeDoWhile* node = Allocate<NodeDoWhile>();
        TryConsume(TokenType::LeftCurly, "'}'");

        node->Body = CreateNode(NodeType::Scope, Allocate<NodeScope>());

        while (!Match(TokenType::RightCurly)) {
            Node* n = ParseStatement();
            std::get<NodeScope*>(node->Body->Data)->Nodes.Append(n);
        }

        TryConsume(TokenType::RightCurly, "'}'");

        TryConsume(TokenType::While, "while");
        TryConsume(TokenType::LeftParen, "'('");
        node->Condition = ParseExpression();
        TryConsume(TokenType::RightParen, "')'");

        return CreateNode(NodeType::DoWhile, node);
    }

    Node* Parser::ParseFor() {
        Consume(); // Consume "for"

        NodeFor* node = Allocate<NodeFor>();
        TryConsume(TokenType::LeftParen, "'('");

        node->Prologue = ParseStatement();

        node->Condition = ParseExpression();
        TryConsume(TokenType::Semi, "';'");

        node->Epilogue = ParseExpression();
        TryConsume(TokenType::RightParen, "')'");

        TryConsume(TokenType::LeftCurly, "'{'");

        node->Body = CreateNode(NodeType::Scope, Allocate<NodeScope>());

        while (!Match(TokenType::RightCurly)) {
            Node* n = ParseStatement();
            std::get<NodeScope*>(node->Body->Data)->Nodes.Append(n);
        }

        TryConsume(TokenType::RightCurly, "'}'");

        return CreateNode(NodeType::For, node);
    }

    Node* Parser::ParseReturn() {
        Consume(); // Consume "return"

        NodeReturn* node = Allocate<NodeReturn>();

        Node* value = ParseExpression();
        node->Value = value;

        return CreateNode(NodeType::Return, node);
    }

    Node* Parser::ParseValue() {
        Token& value = *Peek();

        switch (value.Type) {
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
                    Node* n = ParseExpression();
                    node->Nodes.Append(n);

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

                // Check if this is a function call
                if (Match(TokenType::LeftParen)) {
                    Consume();

                    NodeFunctionCallExpr* node = Allocate<NodeFunctionCallExpr>();
                    node->Name = value.Data;

                    while (!Match(TokenType::RightParen)) {
                        Node* val = ParseExpression();

                        if (Match(TokenType::Comma)) {
                            Consume();
                        }

                        node->Parameters.push_back(val);
                    }

                    TryConsume(TokenType::RightParen, "')'");

                    return CreateNode(NodeType::FunctionCallExpr, node);
                } else {
                    NodeVarRef* node = Allocate<NodeVarRef>();
                    node->Identifier = value.Data;

                    return CreateNode(NodeType::VarRef, node);
                }
                
                break;
            }
        }

        return nullptr;
    }

    BinExprType Parser::ParseOperator() {
        Token op = *Peek();

        switch (op.Type) {
            case TokenType::Plus: return BinExprType::Add;
            case TokenType::PlusEq: return BinExprType::AddInPlace;
            case TokenType::Minus: return BinExprType::Sub;
            case TokenType::MinusEq: return BinExprType::SubInPlace;
            case TokenType::Star: return BinExprType::Mul;
            case TokenType::StarEq: return BinExprType::MulInPlace;
            case TokenType::Slash: return BinExprType::Div;
            case TokenType::SlashEq: return BinExprType::DivInPlace;
            case TokenType::Less: return BinExprType::Less;
            case TokenType::LessOrEq: return BinExprType::LessOrEq;
            case TokenType::Greater: return BinExprType::Greater;
            case TokenType::GreaterOrEq: return BinExprType::GreaterOrEq;
            case TokenType::Eq: return BinExprType::Eq;
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

        if (!lhsNode) {
            ErrorExpected("expression");
        }

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

        Node* node = nullptr;

        if (ParseVariableType() != VariableType::Invalid) {
            node = ParseType();
        } else if (t == TokenType::While) {
            node = ParseWhile();
        } else if (t == TokenType::Do) {
            node = ParseDoWhile();
        } else if (t == TokenType::For) {
            node = ParseFor();
        } else if (t == TokenType::Return) {
            node = ParseReturn();
        } else {
            node = ParseExpression();
        }

        if (Peek(-1)->Type != TokenType::RightCurly) {
            TryConsume(TokenType::Semi, "';'");
        }

        return node;
    }

    void Parser::ErrorExpected(const std::string& msg) {
        if (m_Index > 0) {
            std::cerr << "Syntax error: Expected " << msg << " after token \"" << TokenTypeToString(Peek(-1)->Type) << "\" on line " << Peek(-1)->Line << "!\n";
        } else {
            std::cerr << "Syntax error: Expected " << msg << "!\n";
        }

        m_Error = true;
    }

    void Parser::PrintNode(Node* n, size_t indentation) {
        std::string ident;
        ident.append(indentation, ' ');
        std::cout << ident;

        if (n == nullptr) {
            std::cout << "NULL\n";
        } else {
            switch (n->Type) {
                case NodeType::Bool: {
                    NodeBool* b = std::get<NodeBool*>(n->Data);

                    if (b->Value) {
                        std::cout << "Bool, Value: true\n";
                    } else {
                        std::cout << "Bool, Value: false\n";
                    }

                    break;
                }

                case NodeType::Int: {
                    NodeInt* i = std::get<NodeInt*>(n->Data);

                    std::cout << "Int, Value: " << i->Int << '\n';

                    break;
                }

                case NodeType::Number: {
                    NodeNumber* num = std::get<NodeNumber*>(n->Data);

                    std::cout << "Number, Value: " << num->Number << '\n';

                    break;
                }

                case NodeType::String: {
                    NodeString* str = std::get<NodeString*>(n->Data);

                    std::cout << "String, Value: \"" << str->String << "\"\n";

                    break;
                }

                case NodeType::InitializerList: {
                    NodeInitializerList* list = std::get<NodeInitializerList*>(n->Data);

                    std::cout << "InitializerList, Values: \n";
                    for (size_t i = 0; i < list->Nodes.Size; i++) {
                        PrintNode(list->Nodes.Items[i], indentation + 4);
                    }

                    break;
                }

                case NodeType::Scope: {
                    NodeScope* scope = std::get<NodeScope*>(n->Data);

                    std::cout << "Scope, Body: \n";
                    for (size_t i = 0; i < scope->Nodes.Size; i++) {
                        PrintNode(scope->Nodes.Items[i], indentation + 4);
                    }

                    break;
                }
                
                case NodeType::VarDecl: {
                    NodeVarDecl* decl = std::get<NodeVarDecl*>(n->Data);

                    std::cout << "VarDecl, Type: " << VariableTypeToString(decl->Type) << ", Name: " << decl->Identifier << ", Value: \n";
                    PrintNode(decl->Value, indentation + 4);

                    std::cout << ident << "Next: \n";
                    PrintNode(decl->Next, indentation + 4);

                    break;
                }

                case NodeType::VarRef: {
                    NodeVarRef* set = std::get<NodeVarRef*>(n->Data);

                    std::cout << "VarRef, Name: " << set->Identifier << '\n';

                    break;
                }

                case NodeType::FunctionDecl: {
                    NodeFunctionDecl* decl = std::get<NodeFunctionDecl*>(n->Data);

                    std::cout << "FunctionDecl, Name: " << decl->Name << ", Return type: " << VariableTypeToString(decl->ReturnType) << '\n';

                    break;
                }

                case NodeType::FunctionImpl: {
                    NodeFunctionImpl* impl = std::get<NodeFunctionImpl*>(n->Data);

                    std::cout << "FunctionImpl, Name: " << impl->Name << ", Return type: " << VariableTypeToString(impl->ReturnType) << ", Body: \n";
                    PrintNode(impl->Body, indentation + 4);

                    break;
                }

                case NodeType::While: {
                    NodeWhile* wh = std::get<NodeWhile*>(n->Data);

                    std::cout << "WhileStatement: \n";
                    PrintNode(wh->Condition, indentation + 4);
                    PrintNode(wh->Body, indentation + 4);

                    break;
                }

                case NodeType::DoWhile: {
                    NodeDoWhile* dowh = std::get<NodeDoWhile*>(n->Data);

                    std::cout << "DoWhileStatement: \n";
                    PrintNode(dowh->Body, indentation + 4);
                    PrintNode(dowh->Condition, indentation + 4);

                    break;
                }

                case NodeType::For: {
                    NodeFor* nfor = std::get<NodeFor*>(n->Data);

                    std::cout << "ForStatement :\n";
                    PrintNode(nfor->Prologue, indentation + 4);
                    PrintNode(nfor->Condition, indentation + 4);
                    PrintNode(nfor->Epilogue, indentation + 4);
                    PrintNode(nfor->Body, indentation + 4);

                    break;
                }

                case NodeType::Return: {
                    NodeReturn* ret = std::get<NodeReturn*>(n->Data);

                    std::cout << "Return, Value: \n";
                    PrintNode(ret->Value, indentation + 4);

                    break;
                }

                case NodeType::FunctionCallExpr: {
                    NodeFunctionCallExpr* call = std::get<NodeFunctionCallExpr*>(n->Data);

                    std::cout << "FunctionCallExpr, Name: " << call->Name << ", Parameters: \n";
                    for (const auto& node : call->Parameters) {
                        PrintNode(node, indentation + 4);
                    }

                    break;
                }

                case NodeType::BinExpr: {
                    NodeBinExpr* expr = std::get<NodeBinExpr*>(n->Data);

                    std::cout << "BinExpr, Operation: " << BinExprTypeToString(expr->Type) << "\n";
                    PrintNode(expr->LHS, indentation + 4);
                    PrintNode(expr->RHS, indentation + 4);

                    break;
                }
            }
        }
    }

} // namespace BlackLua::Internal
