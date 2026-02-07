#include "internal/compiler/parser.hpp"

#include <string>
#include <charconv>
#include <limits>

namespace BlackLua::Internal {

    Parser Parser::Parse(const Lexer::Tokens& tokens) {
        Parser p;
        p.m_Tokens = tokens;
        p.ParseImpl();
        
        return p;
    }

    ASTNodes* Parser::GetNodes() {
        return &m_Nodes;
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

    Node* Parser::ParseType(bool external) {
        std::string type = ParseVariableType();

        if (Peek(1)) {
            if (Peek(1)->Type == TokenType::LeftParen) {
                return ParseFunctionDecl(type, external);
            }
        }

        Node* node = ParseVariableDecl(type);
        Node* current = node;

        while (Match(TokenType::Comma)) {
            Consume();
            
            Node* next = ParseVariableDecl(type);
            current = next;
        }

        return node;
    }

    Node* Parser::ParseVariableDecl(const std::string& type) {
        Token* ident = TryConsume(TokenType::Identifier, "identifier");

        if (ident) {
            NodeVarDecl* node = Allocate<NodeVarDecl>();
            node->Identifier = ident->Data;
            node->Type = type;

            if (Match(TokenType::Eq)) {
                Consume();
                node->Value = ParseExpression();
            }

            return CreateNode(NodeType::VarDecl, node);
        } else {
            return nullptr;
        }
    }

    Node* Parser::ParseFunctionDecl(const std::string& returnType, bool external) {
        Token* ident = TryConsume(TokenType::Identifier, "identifier");

        if (ident) {
            Node* finalNode = nullptr;
            NodeList params;

            if (Match(TokenType::LeftParen)) {
                Consume();
                params = ParseFunctionParameters();
                TryConsume(TokenType::RightParen, "')'");

                if (Match(TokenType::LeftCurly)) {
                    NodeFunctionImpl* node = Allocate<NodeFunctionImpl>();
                    node->Name = ident->Data;
                    node->ReturnType = returnType;
                    node->Parameters = params;

                    node->Body = ParseScope();

                    finalNode = CreateNode(NodeType::FunctionImpl, node);
                    m_NeedsSemi = false;
                } else {
                    NodeFunctionDecl* node = Allocate<NodeFunctionDecl>();
                    node->Name = ident->Data;
                    node->ReturnType = returnType;
                    node->Parameters = params;
                    node->Extern = external;

                    finalNode = CreateNode(NodeType::FunctionDecl, node);
                }
            } else {
                ErrorExpected("'('");
            }

            return finalNode;
        } else {
            return nullptr;
        }
    }

    Node* Parser::ParseExtern() {
        Consume();

        return ParseType(true);
    }

    Node* Parser::ParseStructDecl() {
        Consume(); // Consume "struct"

        Token* ident = TryConsume(TokenType::Identifier, "indentifier");

        if (!ident) { return nullptr; }

        NodeStructDecl* node = Allocate<NodeStructDecl>();
        node->Identifier = ident->Data;

        TryConsume(TokenType::LeftCurly, "'{'");

        while (!Match(TokenType::RightCurly)) {
            if (IsVariableType()) {
                Node* field = nullptr;
                std::string type = ParseVariableType();

                Token* fieldName = TryConsume(TokenType::Identifier, "identifier");
                if (!fieldName) { return nullptr; }

                if (Match(TokenType::LeftParen)) {
                    Consume();

                    NodeMethodDecl* decl = Allocate<NodeMethodDecl>();
                    decl->Name = fieldName->Data;
                    decl->ReturnType = type;

                    NodeList params = ParseFunctionParameters();
                    TryConsume(TokenType::RightParen, "')'");

                    decl->Parameters = params;
                    decl->Body = ParseScope();

                    node->Fields.Append(CreateNode(NodeType::MethodDecl, decl));
                } else {
                    NodeFieldDecl* decl = Allocate<NodeFieldDecl>();
                    decl->Identifier = fieldName->Data;
                    decl->Type = type;

                    TryConsume(TokenType::Semi, "';'");

                    node->Fields.Append(CreateNode(NodeType::FieldDecl, decl));
                }
            }
        }

        TryConsume(TokenType::RightCurly, "'}'");

        return CreateNode(NodeType::StructDecl, node);
    }

    Node* Parser::ParseWhile() {
        Consume(); // Consume "while"

        NodeWhile* node = Allocate<NodeWhile>();

        TryConsume(TokenType::LeftParen, "'('");
        node->Condition = ParseExpression();
        TryConsume(TokenType::RightParen, "')'");
        node->Body = ParseScope();

        m_NeedsSemi = false;

        return CreateNode(NodeType::While, node);
    }

    Node* Parser::ParseDoWhile() {
        Consume(); // Consume "do"

        NodeDoWhile* node = Allocate<NodeDoWhile>();
        node->Body = ParseScope();

        TryConsume(TokenType::While, "while");
        TryConsume(TokenType::LeftParen, "'('");
        node->Condition = ParseExpression();
        TryConsume(TokenType::RightParen, "')'");

        m_NeedsSemi = false;

        return CreateNode(NodeType::DoWhile, node);
    }

    Node* Parser::ParseFor() {
        Consume(); // Consume "for"

        NodeFor* node = Allocate<NodeFor>();
        TryConsume(TokenType::LeftParen, "'('");

        // We could have a for statement like this: int i = 0; for (; i < 5; i++)
        if (Match(TokenType::Semi)) {
            Consume();
        } else {
            node->Prologue = ParseStatement();
        }

        node->Condition = ParseExpression();
        TryConsume(TokenType::Semi, "';'");

        node->Epilogue = ParseExpression();
        TryConsume(TokenType::RightParen, "')'");
        node->Body = ParseScope();

        m_NeedsSemi = false;

        return CreateNode(NodeType::For, node);
    }

    Node* Parser::ParseIf() {
        Consume(); // Consume "if"

        NodeIf* node = Allocate<NodeIf>();
        TryConsume(TokenType::LeftParen, "'('");

        node->Condition = ParseExpression();

        TryConsume(TokenType::RightParen, "')'");
        if (Match(TokenType::LeftCurly)) {
            node->Body = ParseScope();
        } else {
            node->Body = ParseStatement();
        }

        if (Match(TokenType::Else)) {
            Consume();

            if (Match(TokenType::LeftCurly)) {
                node->ElseBody = ParseScope();
            } else {
                node->ElseBody = ParseStatement();
            }
        }

        m_NeedsSemi = false;

        return CreateNode(NodeType::If, node);
    }

    Node* Parser::ParseReturn() {
        Consume(); // Consume "return"

        NodeReturn* node = Allocate<NodeReturn>();

        Node* value = ParseExpression();
        node->Value = value;

        return CreateNode(NodeType::Return, node);
    }

    Node* Parser::ParseValue() {
        if (!Peek()) { return nullptr; }
        Token& value = *Peek();
    
        switch (value.Type) {
            case TokenType::False: {
                Consume();
    
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Bool, Allocate<NodeBool>(false)));
                break;
            }
    
            case TokenType::True: {
                Consume();
    
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Bool, Allocate<NodeBool>(true)));
                break;
            }
    
            case TokenType::CharLit: {
                Consume();
    
                int8_t ch = static_cast<int8_t>(value.Data[0]);
                
                NodeChar* node = Allocate<NodeChar>(ch);
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Char, node));
                break;
            }
    
            case TokenType::IntLit: {
                Consume();
    
                int32_t num = 0;
                auto [ptr, ec] = std::from_chars(value.Data.data(), value.Data.data() + value.Data.size(), num);
    
                if (ec != std::errc()) {
                    ErrorTooLarge(value.Data);
                }
    
                NodeInt* node = Allocate<NodeInt>(num, false);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Int, node));
                break;
            }
    
            case TokenType::UIntLit: {
                Consume();
    
                uint32_t num = 0;
                auto [ptr, ec] = std::from_chars(value.Data.data(), value.Data.data() + value.Data.size(), num);
    
                if (ec != std::errc()) {
                    ErrorTooLarge(value.Data);
                }
    
                NodeInt* node = Allocate<NodeInt>(static_cast<int32_t>(num), true);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Int, node));
                break;
            }
    
            case TokenType::LongLit: {
                Consume();
    
                int64_t num = std::stoll(std::string(value.Data));
                NodeLong* node = Allocate<NodeLong>(num, false);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Long, node));
                break;
            }
    
            case TokenType::ULongLit: {
                Consume();
    
                uint64_t num = std::stoull(std::string(value.Data));
                NodeLong* node = Allocate<NodeLong>(static_cast<int64_t>(num), true);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Long, node));
                break;
            }
    
            case TokenType::FloatLit: {
                Consume();
    
                float f = std::stof(std::string(value.Data));
                NodeFloat* node = Allocate<NodeFloat>(f);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Float, node));
                break;
            }
    
            case TokenType::DoubleLit: {
                Consume();
    
                double d = std::stod(std::string(value.Data));
                NodeDouble* node = Allocate<NodeDouble>(d);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Double, node));
                break;
            }
    
            case TokenType::StrLit: {
                Consume();
    
                std::string_view str = value.Data;
                NodeString* node = Allocate<NodeString>(str);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::String, node));
                break;
            }
    
            case TokenType::Minus: {
                Consume();
    
                Node* expr = ParseValue();
                NodeUnaryExpr* node = Allocate<NodeUnaryExpr>(expr, UnaryExprType::Negate);
    
                return CreateNode(NodeType::UnaryExpr, node);
                break;
            }
    
            case TokenType::LeftParen: {
                Consume();
    
                if (IsVariableType()) {
                    std::string type = ParseVariableType();

                    TryConsume(TokenType::RightParen, "')'");
                
                    NodeCastExpr* node = Allocate<NodeCastExpr>();
                    node->Type = type;
                    node->Expression = ParseValue();
                
                    return CreateNode(NodeType::CastExpr, node);
                } else {
                    Node* expr = ParseExpression();
                    NodeParenExpr* node = Allocate<NodeParenExpr>(expr);
                    
                    TryConsume(TokenType::RightParen, "')'");
                    
                    return CreateNode(NodeType::ParenExpr, node);
                }
                
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
    
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::InitializerList, node));
                break;
            }
    
            case TokenType::Identifier: {
                Consume();

                Node* final = nullptr;
    
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
    
                        node->Arguments.Append(val);
                    }
    
                    TryConsume(TokenType::RightParen, "')'");
    
                    final = CreateNode(NodeType::FunctionCallExpr, node);
                } else {
                    NodeVarRef* varRef = Allocate<NodeVarRef>();
                    varRef->Identifier = value.Data;

                    final = CreateNode(NodeType::VarRef, varRef);
                }

                // Handle member access (foo.bar) and array access (foo[5])
                while (Match(TokenType::Dot) || Match(TokenType::LeftBracket)) {
                    Token op = Consume();

                    if (op.Type == TokenType::Dot) {
                        Token* member = TryConsume(TokenType::Identifier, "identifier");
                        if (!member) { return nullptr; }

                        if (Match(TokenType::LeftParen)) {
                            Consume();
    
                            NodeMethodCallExpr* methodExpr = Allocate<NodeMethodCallExpr>();
                            methodExpr->Member = member->Data;
                            methodExpr->Parent = final;
    
                            while (!Match(TokenType::RightParen)) {
                                Node* val = ParseExpression();
    
                                if (Match(TokenType::Comma)) {
                                    Consume();
                                }
    
                                methodExpr->Arguments.Append(val);
                            }
    
                            TryConsume(TokenType::RightParen, "')'");
    
                            final = CreateNode(NodeType::MethodCallExpr, methodExpr);
                        } else {
                            NodeMemberExpr* memExpr = Allocate<NodeMemberExpr>();

                            memExpr->Member = member->Data;
                            memExpr->Parent = final;
                            final = CreateNode(NodeType::MemberExpr, memExpr);
                        }
                    } else if (op.Type == TokenType::LeftBracket) {
                        NodeArrayAccessExpr* arrExpr = Allocate<NodeArrayAccessExpr>();

                        arrExpr->Index = ParseExpression();
                        arrExpr->Parent = final;
                        final = CreateNode(NodeType::ArrayAccessExpr, arrExpr);

                        TryConsume(TokenType::RightBracket, "']'");
                    }
                }

                return final;
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
            case TokenType::PlusPlus: return BinExprType::AddOne;
            case TokenType::Minus: return BinExprType::Sub;
            case TokenType::MinusEq: return BinExprType::SubInPlace;
            case TokenType::MinusMinus: return BinExprType::SubOne;
            case TokenType::Star: return BinExprType::Mul;
            case TokenType::StarEq: return BinExprType::MulInPlace;
            case TokenType::Slash: return BinExprType::Div;
            case TokenType::SlashEq: return BinExprType::DivInPlace;
            case TokenType::Less: return BinExprType::Less;
            case TokenType::LessOrEq: return BinExprType::LessOrEq;
            case TokenType::Greater: return BinExprType::Greater;
            case TokenType::GreaterOrEq: return BinExprType::GreaterOrEq;
            case TokenType::Eq: return BinExprType::Eq;
            case TokenType::IsEq: return BinExprType::IsEq;
            case TokenType::IsNotEq: return BinExprType::IsNotEq;
            default: return BinExprType::Invalid;
        }
    }

    std::string Parser::ParseVariableType() {
        if (!Peek()) { return ""; }
        Token type = Consume();

        std::string strType;

        switch (type.Type) {
            case TokenType::Void:       strType = "void"; break;
            case TokenType::Bool:       strType = "bool"; break;
            case TokenType::Char:       strType = "char"; break;
            case TokenType::Short:      strType = "short"; break;
            case TokenType::Int:        strType = "int"; break;
            case TokenType::Long:       strType = "long"; break;
            case TokenType::Float:      strType = "float"; break;
            case TokenType::Double:     strType = "double"; break;
            case TokenType::String:     strType = "string"; break;
            case TokenType::Identifier: strType = type.Data; break;
            default:                    strType = ""; break;
        }

        if (Peek(1)) {
            if (Peek(0)->Type == TokenType::LeftBracket && Peek(1)->Type == TokenType::RightBracket) {
                Consume();
                Consume();

                strType += "[]";
            }
        }

        return strType;
    }

    NodeList Parser::ParseFunctionParameters() {
        NodeList params;

        while (!Match(TokenType::RightParen)) {
            std::string type = ParseVariableType();

            Token& ident = Consume();
            
            NodeParamDecl* param = Allocate<NodeParamDecl>();
            param->Identifier = ident.Data;
            param->Type = type;
            
            Node* n = CreateNode(NodeType::ParamDecl, param);

            if (Match(TokenType::Comma)) {
                Consume();
            }

            params.Append(n);
        }

        return params;
    }

    bool Parser::IsPrimitiveType() {
        if (!Peek()) { return false; }

        Token type = *Peek();

        switch (type.Type) {
            case TokenType::Void:
            case TokenType::Bool:
            case TokenType::Char:
            case TokenType::Short:
            case TokenType::Int:
            case TokenType::Long:
            case TokenType::Float:
            case TokenType::Double:
            case TokenType::String: return true;
            default: return false;
        }

        return false;
    }

    bool Parser::IsVariableType() {
        if (IsPrimitiveType()) { return true; }

        if (Peek()->Type == TokenType::Identifier) {
            // If identifier is followed by another identifier or by a semicolon, it is a 
            if (Peek(1)) {
                if (Peek(1)->Type == TokenType::Semi) {
                    return false; // Expression
                } else if (Peek(1)->Type == TokenType::Identifier) {
                    return true; // Declaration
                } else if (Peek(1)->Type == TokenType::RightParen) {
                    return true; // Part of a cast
                } else { // Could be an array declaration
                    if (Peek(2)) {
                        if (Peek(1)->Type == TokenType::LeftBracket && Peek(2)->Type == TokenType::RightBracket) {
                            return true; // Array declaration
                        } else {
                            return false;
                        }
                    } else {
                        return false;
                    }
                }
            }

            return false;
        }

        return false;
    }

    size_t Parser::GetBinaryPrecedence(BinExprType type) {
        switch (type) {
            case BinExprType::Eq:
                return 10;

            case BinExprType::Less:
            case BinExprType::LessOrEq:
            case BinExprType::Greater:
            case BinExprType::GreaterOrEq:
            case BinExprType::IsEq:
            case BinExprType::IsNotEq:
                return 20;

            case BinExprType::Add:
            case BinExprType::AddInPlace:
            case BinExprType::AddOne:
            case BinExprType::Sub:
            case BinExprType::SubInPlace:
            case BinExprType::SubOne:
                return 30;

            case BinExprType::Mul:
            case BinExprType::MulInPlace:
            case BinExprType::Div:
            case BinExprType::DivInPlace:
                return 40;
        }

        BLUA_ASSERT(false, "Unreachable");
        return -1;
    }

    Node* Parser::ParseScope() {
        NodeScope* node = Allocate<NodeScope>();

        TryConsume(TokenType::LeftCurly, "'{'");

        while (!Match(TokenType::RightCurly)) {
            Node* n = ParseStatement();
            node->Nodes.Append(n);
        }

        TryConsume(TokenType::RightCurly, "'}'");

        return CreateNode(NodeType::Scope, node);
    }

    Node* Parser::ParseExpression(size_t minbp) {
        Node* lhsNode = ParseValue();

        if (!lhsNode) {
            ErrorExpected("expression");
        }

        // There are two conditions to this loop
        // A valid operator (the first half of the check) and a valid precedence (the second half)
        while ((Peek() && ParseOperator() != BinExprType::Invalid) && 
               (GetBinaryPrecedence(ParseOperator()) >= minbp)) {
            BinExprType op = ParseOperator();
            Consume();

            Node* rhsNode = ParseExpression(GetBinaryPrecedence(op) + 1);

            NodeBinExpr* newExpr = Allocate<NodeBinExpr>();
            newExpr->LHS = lhsNode;
            newExpr->RHS = rhsNode;
            newExpr->Type = op;

            lhsNode = CreateNode(NodeType::BinExpr, newExpr);
        }

        return lhsNode;
    }

    Node* Parser::ParseStatement() {
        TokenType t = Peek()->Type;

        Node* node = nullptr;

        if (IsVariableType()) {
            node = ParseType();
        } else if (t == TokenType::Extern) {
            node = ParseExtern();
        } else if (t == TokenType::Struct) {
            node = ParseStructDecl();
        } else if (t == TokenType::LeftCurly) {
            node = ParseScope();
        } else if (t == TokenType::While) {
            node = ParseWhile();
        } else if (t == TokenType::Do) {
            node = ParseDoWhile();
        } else if (t == TokenType::For) {
            node = ParseFor();
        } else if (t == TokenType::If) {
            node = ParseIf();
        } else if (t == TokenType::Return) {
            node = ParseReturn();
        } else {
            node = ParseExpression();
        }

        if (m_NeedsSemi) {
            TryConsume(TokenType::Semi, "';'");
        }

        m_NeedsSemi = true;

        return node;
    }

    void Parser::ErrorExpected(const std::string& msg) {
        BLUA_FORMAT_ERROR("Error {}:{}: Expected {} after token \"{}\"", Peek(-1)->Line, Peek(-1)->Column, msg, TokenTypeToString(Peek(-1)->Type));
        // std::cerr << "Error " << Peek(-1)->Line << ":0: Expected " << msg << " after token \"" << TokenTypeToString(Peek(-1)->Type) << "\"!\n";

        m_Error = true;
    }

    void Parser::ErrorTooLarge(const std::string_view value) {
        BLUA_FORMAT_ERROR("Error {}:{}: Constant {} is too large", Peek(-1)->Line, Peek(-1)->Column, value);
        // std::cerr << "Error " << Peek(-1)->Line << ":0: Constant " << value << " is too large!\n";

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
                case NodeType::Constant: {
                    NodeConstant* constant = std::get<NodeConstant*>(n->Data);

                    std::cout << "Constant, Value: \n";
                    ident.append(4, ' ');
                    std::cout << ident;

                    switch (constant->Type) {
                        case NodeType::Bool: {
                            NodeBool* b = std::get<NodeBool*>(constant->Data);

                            if (b->Value) {
                                std::cout << "Bool, Value: true\n";
                            } else {
                                std::cout << "Bool, Value: false\n";
                            }

                            break;
                        }

                        case NodeType::Int: {
                            NodeInt* i = std::get<NodeInt*>(constant->Data);

                            if (i->Unsigned) {  
                                std::cout << "Int, Value: " << static_cast<uint32_t>(i->Int) << ", Signed: false \n";
                            } else {
                                std::cout << "Int, Value: " << i->Int << ", Signed: true \n";
                            }

                            break;
                        }

                        case NodeType::Long: {
                            NodeLong* l = std::get<NodeLong*>(constant->Data);

                            if (l->Unsigned) {  
                                std::cout << "Long, Value: " << static_cast<uint64_t>(l->Long) << ", Signed: false \n";
                            } else {
                                std::cout << "Long, Value: " << l->Long << ", Signed: true \n";
                            }

                            break;
                        }

                        case NodeType::Float: {
                            NodeFloat* f = std::get<NodeFloat*>(constant->Data);

                            // For floating point numbers we want all the precision which is why we resort to printf
                            // This is possible do with std::cout, but is rather clunky. std::format could work nicely here but i don't
                            // want to depend on c++20. fmt::format is better than std::format but i don't want to drag unnecessary dependencies
                            // Even though fmt is a really nice library so we might end up using it at one point
                            printf("Float, Value: %.5f\n", f->Float);

                            break;
                        }

                        case NodeType::Double: {
                            NodeDouble* d = std::get<NodeDouble*>(constant->Data);

                            printf("Double, Value: %.15f\n", d->Double);

                            break;
                        }

                        case NodeType::String: {
                            NodeString* str = std::get<NodeString*>(constant->Data);

                            std::cout << "String, Value: \"" << str->String << "\"\n";

                            break;
                        }

                        case NodeType::InitializerList: {
                            NodeInitializerList* list = std::get<NodeInitializerList*>(constant->Data);

                            std::cout << "InitializerList, Values: \n";
                            for (size_t i = 0; i < list->Nodes.Size; i++) {
                                PrintNode(list->Nodes.Items[i], indentation + 4);
                            }

                            break;
                        }
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

                    std::cout << "VarDecl, Type: " << decl->Type << ", Name: " << decl->Identifier << ", Value: \n";
                    PrintNode(decl->Value, indentation + 4);

                    break;
                }

                case NodeType::ParamDecl: {
                    NodeParamDecl* decl = std::get<NodeParamDecl*>(n->Data);

                    std::cout << "ParamDecl, Type: " << decl->Type << ", Name: " << decl->Identifier << "\n";

                    break;
                }

                case NodeType::VarRef: {
                    NodeVarRef* ref = std::get<NodeVarRef*>(n->Data);

                    std::cout << "VarRef, Name: " << ref->Identifier << '\n';

                    break;
                }

                case NodeType::StructDecl: {
                    NodeStructDecl* decl = std::get<NodeStructDecl*>(n->Data);

                    std::cout << "StructDecl, Name: " << decl->Identifier << ", Fields: \n";

                    for (size_t i = 0; i < decl->Fields.Size; i++) {
                        PrintNode(decl->Fields.Items[i], indentation + 4);
                    }

                    break;
                }

                case NodeType::FieldDecl: {
                    NodeFieldDecl* decl = std::get<NodeFieldDecl*>(n->Data);

                    std::cout << "FieldDecl, Type: " << decl->Type << ", Name: " << decl->Identifier << '\n';

                    break;
                }

                case NodeType::MethodDecl: {
                    NodeMethodDecl* decl = std::get<NodeMethodDecl*>(n->Data);

                    std::cout << "MethodDecl, Name: " << decl->Name << ", Return type: " << decl->ReturnType << '\n';
                    for (size_t i = 0; i < decl->Parameters.Size; i++) {
                        PrintNode(decl->Parameters.Items[i], indentation + 4);
                    }
                    PrintNode(decl->Body, indentation + 4);

                    break;
                }

                case NodeType::FunctionDecl: {
                    NodeFunctionDecl* decl = std::get<NodeFunctionDecl*>(n->Data);

                    std::cout << "FunctionDecl, Name: " << decl->Name << ", Return type: " << decl->ReturnType << '\n';
                    for (size_t i = 0; i < decl->Parameters.Size; i++) {
                        PrintNode(decl->Parameters.Items[i], indentation + 4);
                    }

                    break;
                }

                case NodeType::FunctionImpl: {
                    NodeFunctionImpl* impl = std::get<NodeFunctionImpl*>(n->Data);

                    std::cout << "FunctionImpl, Name: " << impl->Name << ", Return type: " << impl->ReturnType << '\n';
                    for (size_t i = 0; i < impl->Parameters.Size; i++) {
                        PrintNode(impl->Parameters.Items[i], indentation + 4);
                    }
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

                    std::cout << "ForStatement: \n";
                    PrintNode(nfor->Prologue, indentation + 4);
                    PrintNode(nfor->Condition, indentation + 4);
                    PrintNode(nfor->Epilogue, indentation + 4);
                    PrintNode(nfor->Body, indentation + 4);

                    break;
                }

                case NodeType::If: {
                    NodeIf* nif = std::get<NodeIf*>(n->Data);

                    std::cout << "IfStatement: \n";
                    PrintNode(nif->Condition, indentation + 4);
                    PrintNode(nif->Body, indentation + 4);
                    PrintNode(nif->ElseBody, indentation + 4);

                    break;
                }

                case NodeType::Return: {
                    NodeReturn* ret = std::get<NodeReturn*>(n->Data);

                    std::cout << "Return, Value: \n";
                    PrintNode(ret->Value, indentation + 4);

                    break;
                }

                case NodeType::ArrayAccessExpr: {
                    NodeArrayAccessExpr* expr = std::get<NodeArrayAccessExpr*>(n->Data);

                    std::cout << "ArrayAccessExpr: \n";
                    PrintNode(expr->Parent, indentation + 4);
                    PrintNode(expr->Index, indentation + 4);

                    break;
                }

                case NodeType::MemberExpr: {
                    NodeMemberExpr* expr = std::get<NodeMemberExpr*>(n->Data);

                    std::cout << "MemberExpr: " << expr->Member << '\n';
                    PrintNode(expr->Parent, indentation + 4);

                    break;
                }

                case NodeType::MethodCallExpr: {
                    NodeMethodCallExpr* call = std::get<NodeMethodCallExpr*>(n->Data);

                    std::cout << "MethodCallExpr, Name: " << call->Member << ", Parameters: \n";
                    for (size_t i = 0; i < call->Arguments.Size; i++) {
                        PrintNode(call->Arguments.Items[i], indentation + 4);
                    }
                    PrintNode(call->Parent, indentation + 4);

                    break;
                }

                case NodeType::FunctionCallExpr: {
                    NodeFunctionCallExpr* call = std::get<NodeFunctionCallExpr*>(n->Data);

                    std::cout << "FunctionCallExpr, Name: " << call->Name << ", Parameters: \n";
                    for (size_t i = 0; i < call->Arguments.Size; i++) {
                        PrintNode(call->Arguments.Items[i], indentation + 4);
                    }

                    break;
                }

                case NodeType::ParenExpr: {
                    NodeParenExpr* expr = std::get<NodeParenExpr*>(n->Data);

                    std::cout << "ParenExpr, Expression: \n";
                    PrintNode(expr->Expression, indentation + 4);

                    break;
                }

                case NodeType::CastExpr: {
                    NodeCastExpr* expr = std::get<NodeCastExpr*>(n->Data);

                    std::cout << "CastExpr, Type: " << expr->Type << ", Expression: \n";
                    PrintNode(expr->Expression, indentation + 4);

                    break;
                }

                case NodeType::UnaryExpr: {
                    NodeUnaryExpr* expr = std::get<NodeUnaryExpr*>(n->Data);

                    std::cout << "UnaryExpr, Operation: " << UnaryExprTypeToString(expr->Type) << ", Expression: \n";
                    PrintNode(expr->Expression, indentation + 4);

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
