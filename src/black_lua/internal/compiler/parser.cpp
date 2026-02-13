#include "internal/compiler/parser.hpp"
#include "context.hpp"

#include "fmt/format.h"

#include <string>
#include <charconv>
#include <limits>

namespace BlackLua::Internal {

    Parser Parser::Parse(const Lexer::Tokens& tokens, Context* ctx) {
        Parser p;
        p.m_Tokens = tokens;
        p.m_Context = ctx;
        p.ParseImpl();
        
        return p;
    }

    ASTNodes* Parser::GetNodes() {
        return &m_Nodes;
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

    Token* Parser::TryConsume(TokenType type, const StringView error) {
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
        StringBuilder type = ParseVariableType();

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

    Node* Parser::ParseVariableDecl(StringBuilder type) {
        Token* ident = TryConsume(TokenType::Identifier, "identifier");

        if (ident) {
            NodeVarDecl* node = Allocate<NodeVarDecl>();
            node->Identifier = ident->Data;
            node->Type = type;

            if (Match(TokenType::Eq)) {
                Consume();
                node->Value = ParseExpression();
            }

            return CreateNode(NodeType::VarDecl, node, ident->Line, ident->Column);
        } else {
            return nullptr;
        }
    }

    Node* Parser::ParseFunctionDecl(StringBuilder returnType, bool external) {
        Token* ident = TryConsume(TokenType::Identifier, "identifier");

        if (ident) {
            Node* finalNode = nullptr;
            NodeList params;

            if (Match(TokenType::LeftParen)) {
                Consume();
                params = ParseFunctionParameters();
                TryConsume(TokenType::RightParen, "')'");

                NodeFunctionDecl* node = Allocate<NodeFunctionDecl>();
                node->Name = ident->Data;
                node->ReturnType = returnType;
                node->Parameters = params;
                node->Extern = external;

                if (Match(TokenType::LeftCurly)) {
                    node->Body = ParseScope();
                    m_NeedsSemi = false;
                }

                finalNode = CreateNode(NodeType::FunctionDecl, node, ident->Line, ident->Column);
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
        Token s = Consume(); // Consume "struct"

        Token* ident = TryConsume(TokenType::Identifier, "indentifier");

        if (!ident) { return nullptr; }

        NodeStructDecl* node = Allocate<NodeStructDecl>();
        node->Identifier = ident->Data;

        TryConsume(TokenType::LeftCurly, "'{'");

        while (!Match(TokenType::RightCurly)) {
            if (IsVariableType()) {
                Node* field = nullptr;
                StringBuilder type = ParseVariableType();

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

                    node->Fields.Append(m_Context, CreateNode(NodeType::MethodDecl, decl, fieldName->Line, fieldName->Column));
                } else {
                    NodeFieldDecl* decl = Allocate<NodeFieldDecl>();
                    decl->Identifier = fieldName->Data;
                    decl->Type = type;

                    TryConsume(TokenType::Semi, "';'");

                    node->Fields.Append(m_Context, CreateNode(NodeType::FieldDecl, decl, fieldName->Line, fieldName->Column));
                }
            }
        }

        TryConsume(TokenType::RightCurly, "'}'");

        return CreateNode(NodeType::StructDecl, node, s.Line, s.Column);
    }

    Node* Parser::ParseWhile() {
        Token w = Consume(); // Consume "while"

        NodeWhile* node = Allocate<NodeWhile>();

        TryConsume(TokenType::LeftParen, "'('");
        node->Condition = ParseExpression();
        TryConsume(TokenType::RightParen, "')'");
        node->Body = ParseScope();

        m_NeedsSemi = false;

        return CreateNode(NodeType::While, node, w.Line, w.Column);
    }

    Node* Parser::ParseDoWhile() {
        Token d = Consume(); // Consume "do"

        NodeDoWhile* node = Allocate<NodeDoWhile>();
        node->Body = ParseScope();

        TryConsume(TokenType::While, "while");
        TryConsume(TokenType::LeftParen, "'('");
        node->Condition = ParseExpression();
        TryConsume(TokenType::RightParen, "')'");

        m_NeedsSemi = false;

        return CreateNode(NodeType::DoWhile, node, d.Line, d.Column);
    }

    Node* Parser::ParseFor() {
        Token f = Consume(); // Consume "for"

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

        return CreateNode(NodeType::For, node, f.Line, f.Column);
    }

    Node* Parser::ParseIf() {
        Token i = Consume(); // Consume "if"

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

        return CreateNode(NodeType::If, node, i.Line, i.Column);
    }

    Node* Parser::ParseBreak() {
        Token b = Consume(); // Consume "break"

        NodeEmpty* node = Allocate<NodeEmpty>();

        return CreateNode(NodeType::Break, node, b.Line, b.Column);
    }

    Node* Parser::ParseContinue() {
        Token c = Consume(); // Consume "continue"

        NodeEmpty* node = Allocate<NodeEmpty>();

        return CreateNode(NodeType::Continue, node, c.Line, c.Column);
    }

    Node* Parser::ParseReturn() {
        Token r = Consume(); // Consume "return"

        NodeReturn* node = Allocate<NodeReturn>();

        Node* value = ParseExpression();
        node->Value = value;

        return CreateNode(NodeType::Return, node, r.Line, r.Column);
    }

    Node* Parser::ParseValue() {
        if (!Peek()) { return nullptr; }
        Token& value = *Peek();
    
        switch (value.Type) {
            case TokenType::False: {
                Consume();
    
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Bool, Allocate<NodeBool>(false)), value.Line, value.Column);
                break;
            }
    
            case TokenType::True: {
                Consume();
    
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Bool, Allocate<NodeBool>(true)), value.Line, value.Column);
                break;
            }
    
            case TokenType::CharLit: {
                Consume();
    
                int8_t ch = static_cast<int8_t>(value.Data.Data()[0]);
                
                NodeChar* node = Allocate<NodeChar>(ch);
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Char, node), value.Line, value.Column);
                break;
            }
    
            case TokenType::IntLit: {
                Consume();
    
                int32_t num = 0;
                auto [ptr, ec] = std::from_chars(value.Data.Data(), value.Data.Data() + value.Data.Size(), num);
    
                if (ec != std::errc()) {
                    ErrorTooLarge(value.Data);
                }
    
                NodeInt* node = Allocate<NodeInt>(num, false);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Int, node), value.Line, value.Column);
                break;
            }
    
            case TokenType::UIntLit: {
                Consume();
    
                uint32_t num = 0;
                auto [ptr, ec] = std::from_chars(value.Data.Data(), value.Data.Data() + value.Data.Size(), num);
    
                if (ec != std::errc()) {
                    ErrorTooLarge(value.Data);
                }
    
                NodeInt* node = Allocate<NodeInt>(static_cast<int32_t>(num), true);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Int, node), value.Line, value.Column);
                break;
            }
    
            case TokenType::LongLit: {
                Consume();
    
                int64_t num = 0;
                auto [ptr, ec] = std::from_chars(value.Data.Data(), value.Data.Data() + value.Data.Size(), num);
    
                if (ec != std::errc()) {
                    ErrorTooLarge(value.Data);
                }
                NodeLong* node = Allocate<NodeLong>(num, false);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Long, node), value.Line, value.Column);
                break;
            }
    
            case TokenType::ULongLit: {
                Consume();
    
                uint64_t num = 0;
                auto [ptr, ec] = std::from_chars(value.Data.Data(), value.Data.Data() + value.Data.Size(), num);
    
                if (ec != std::errc()) {
                    ErrorTooLarge(value.Data);
                }
                NodeLong* node = Allocate<NodeLong>(static_cast<int64_t>(num), true);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Long, node), value.Line, value.Column);
                break;
            }
    
            case TokenType::FloatLit: {
                Consume();
    
                float num = 0.0f;
                auto [ptr, ec] = std::from_chars(value.Data.Data(), value.Data.Data() + value.Data.Size(), num);
    
                if (ec != std::errc()) {
                    ErrorTooLarge(value.Data);
                }
                NodeFloat* node = Allocate<NodeFloat>(num);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Float, node), value.Line, value.Column);
                break;
            }
    
            case TokenType::DoubleLit: {
                Consume();
    
                double num = 0.0;
                auto [ptr, ec] = std::from_chars(value.Data.Data(), value.Data.Data() + value.Data.Size(), num);
    
                if (ec != std::errc()) {
                    ErrorTooLarge(value.Data);
                }
                NodeDouble* node = Allocate<NodeDouble>(num);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::Double, node), value.Line, value.Column);
                break;
            }
    
            case TokenType::StrLit: {
                Consume();
    
                StringView str = value.Data;
                NodeString* node = Allocate<NodeString>(str);
                
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::String, node), value.Line, value.Column);
                break;
            }
    
            case TokenType::Minus: {
                Token m = Consume();
    
                Node* expr = ParseValue();
                NodeUnaryExpr* node = Allocate<NodeUnaryExpr>(expr, UnaryExprType::Negate);
    
                return CreateNode(NodeType::UnaryExpr, node, m.Line, m.Column);
                break;
            }
    
            case TokenType::LeftParen: {
                Token p = Consume();
    
                if (IsVariableType()) {
                    StringBuilder type = ParseVariableType();

                    TryConsume(TokenType::RightParen, "')'");
                
                    NodeCastExpr* node = Allocate<NodeCastExpr>();
                    node->Type = StringView(type.Data(), type.Size());
                    node->Expression = ParseValue();
                
                    return CreateNode(NodeType::CastExpr, node, p.Line, p.Column);
                } else {
                    Node* expr = ParseExpression();
                    NodeParenExpr* node = Allocate<NodeParenExpr>(expr);
                    
                    TryConsume(TokenType::RightParen, "')'");
                    
                    return CreateNode(NodeType::ParenExpr, node, p.Line, p.Column);
                }
                
                break;
            }
    
            case TokenType::LeftCurly: {
                Token c = Consume();
    
                NodeInitializerList* node = Allocate<NodeInitializerList>();
    
                while (!Match(TokenType::RightCurly)) {
                    Node* n = ParseExpression();
                    node->Nodes.Append(m_Context, n);
    
                    if (Match(TokenType::Comma)) {
                        Consume();
                    }
                }
    
                Consume(); // Eat '}'
    
                return CreateNode(NodeType::Constant, Allocate<NodeConstant>(NodeType::InitializerList, node), c.Line, c.Column);
                break;
            }
    
            case TokenType::Identifier: {
                Token i = Consume();

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
    
                        node->Arguments.Append(m_Context, val);
                    }
    
                    TryConsume(TokenType::RightParen, "')'");
    
                    final = CreateNode(NodeType::FunctionCallExpr, node, i.Line, i.Column);
                } else {
                    NodeVarRef* varRef = Allocate<NodeVarRef>();
                    varRef->Identifier = value.Data;

                    final = CreateNode(NodeType::VarRef, varRef, i.Line, i.Column);
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
    
                                methodExpr->Arguments.Append(m_Context, val);
                            }
    
                            TryConsume(TokenType::RightParen, "')'");
    
                            final = CreateNode(NodeType::MethodCallExpr, methodExpr, i.Line, i.Column);
                        } else {
                            NodeMemberExpr* memExpr = Allocate<NodeMemberExpr>();

                            memExpr->Member = member->Data;
                            memExpr->Parent = final;
                            final = CreateNode(NodeType::MemberExpr, memExpr, i.Line, i.Column);
                        }
                    } else if (op.Type == TokenType::LeftBracket) {
                        NodeArrayAccessExpr* arrExpr = Allocate<NodeArrayAccessExpr>();

                        arrExpr->Index = ParseExpression();
                        arrExpr->Parent = final;
                        final = CreateNode(NodeType::ArrayAccessExpr, arrExpr, i.Line, i.Column);

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
            case TokenType::Minus: return BinExprType::Sub;
            case TokenType::MinusEq: return BinExprType::SubInPlace;
            case TokenType::Star: return BinExprType::Mul;
            case TokenType::StarEq: return BinExprType::MulInPlace;
            case TokenType::Slash: return BinExprType::Div;
            case TokenType::SlashEq: return BinExprType::DivInPlace;
            case TokenType::Percent: return BinExprType::Mod;
            case TokenType::PercentEq: return BinExprType::ModInPlace;
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

    StringBuilder Parser::ParseVariableType() {
        Token type = Consume();

        StringBuilder strType;

        switch (type.Type) {
            case TokenType::Void:       strType.Append(m_Context, "void"); break;
            case TokenType::Bool:       strType.Append(m_Context, "bool"); break;
            case TokenType::Char:       strType.Append(m_Context, "char"); break;
            case TokenType::UChar:      strType.Append(m_Context, "uchar"); break;
            case TokenType::Short:      strType.Append(m_Context, "short"); break;
            case TokenType::UShort:     strType.Append(m_Context, "ushort"); break;
            case TokenType::Int:        strType.Append(m_Context, "int"); break;
            case TokenType::UInt:       strType.Append(m_Context, "uint"); break;
            case TokenType::Long:       strType.Append(m_Context, "long"); break;
            case TokenType::ULong:      strType.Append(m_Context, "ulong"); break;
            case TokenType::Float:      strType.Append(m_Context, "float"); break;
            case TokenType::Double:     strType.Append(m_Context, "double"); break;
            case TokenType::String:     strType.Append(m_Context, "string"); break;
            case TokenType::Identifier: strType.Append(m_Context, type.Data); break;
            default:                    strType.Append(m_Context, ""); break;
        }

        if (Peek(1)) {
            if (Peek(0)->Type == TokenType::LeftBracket && Peek(1)->Type == TokenType::RightBracket) {
                Consume();
                Consume();

                strType.Append(m_Context, "[]");
            }
        }

        return strType;
    }

    NodeList Parser::ParseFunctionParameters() {
        NodeList params;

        while (!Match(TokenType::RightParen)) {
            StringBuilder type = ParseVariableType();

            Token& ident = Consume();
            
            NodeParamDecl* param = Allocate<NodeParamDecl>();
            param->Identifier = ident.Data;
            param->Type = type;
            
            Node* n = CreateNode(NodeType::ParamDecl, param, ident.Line, ident.Column);

            if (Match(TokenType::Comma)) {
                Consume();
            }

            params.Append(m_Context, n);
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
            case TokenType::UChar:
            case TokenType::Short:
            case TokenType::UShort:
            case TokenType::Int:
            case TokenType::UInt:
            case TokenType::Long:
            case TokenType::ULong:
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
            case BinExprType::AddInPlace:
            case BinExprType::SubInPlace:
            case BinExprType::MulInPlace:
            case BinExprType::ModInPlace:
            case BinExprType::DivInPlace:
                return 10;

            case BinExprType::Less:
            case BinExprType::LessOrEq:
            case BinExprType::Greater:
            case BinExprType::GreaterOrEq:
            case BinExprType::IsEq:
            case BinExprType::IsNotEq:
                return 20;

            case BinExprType::Mod:
                return 30;

            case BinExprType::Add:
            case BinExprType::AddOne:
            case BinExprType::Sub:
            case BinExprType::SubOne:
                return 40;

            case BinExprType::Mul:
            case BinExprType::Div:
                return 50;
        }

        BLUA_ASSERT(false, "Unreachable");
        return -1;
    }

    Node* Parser::ParseScope() {
        NodeScope* node = Allocate<NodeScope>();

        Token* l = TryConsume(TokenType::LeftCurly, "'{'");

        while (!Match(TokenType::RightCurly)) {
            Node* n = ParseStatement();
            node->Nodes.Append(m_Context, n);
        }

        TryConsume(TokenType::RightCurly, "'}'");

        return CreateNode(NodeType::Scope, node, l->Line, l->Column);
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
            Token o = Consume();

            Node* rhsNode = ParseExpression(GetBinaryPrecedence(op) + 1);

            NodeBinExpr* newExpr = Allocate<NodeBinExpr>();
            newExpr->LHS = lhsNode;
            newExpr->RHS = rhsNode;
            newExpr->Type = op;

            lhsNode = CreateNode(NodeType::BinExpr, newExpr, o.Line, o.Column);
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
        } else if (t == TokenType::Break) {
            node = ParseBreak();
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

    void Parser::ErrorExpected(const StringView msg) {
        if (Peek(-1)) {
            m_Context->ReportCompilerError(Peek(-1)->Line, Peek(-1)->Column, fmt::format("Expected {} after token \"{}\"", msg, TokenTypeToString(Peek(-1)->Type)));
        } else {
            m_Context->ReportCompilerError(Peek()->Line, Peek()->Column, fmt::format("Expected {} after token \"{}\"", msg, TokenTypeToString(Peek()->Type)));
        }
        
        m_Error = true;
    }

    void Parser::ErrorTooLarge(const StringView value) {
        m_Context->ReportCompilerError(Peek(-1)->Line, Peek(-1)->Column, fmt::format("Constant {} is too large", value));
        m_Error = true;
    }

} // namespace BlackLua::Internal
