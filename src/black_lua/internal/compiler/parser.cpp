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
            Node* node = ParseToken();
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
            
            StmtParamDecl* param = Allocate<StmtParamDecl>();
            param->Identifier = ident.Data;
            param->Type = type;
            
            Node* n = Allocate<Node>(Allocate<NodeStmt>(param, ident.Loc));

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

    BinaryOperatorType Parser::ParseOperator() {
        Token op = *Peek();

        switch (op.Type) {
            case TokenType::Plus: return BinaryOperatorType::Add;
            case TokenType::PlusEq: return BinaryOperatorType::AddInPlace;
            case TokenType::Minus: return BinaryOperatorType::Sub;
            case TokenType::MinusEq: return BinaryOperatorType::SubInPlace;
            case TokenType::Star: return BinaryOperatorType::Mul;
            case TokenType::StarEq: return BinaryOperatorType::MulInPlace;
            case TokenType::Slash: return BinaryOperatorType::Div;
            case TokenType::SlashEq: return BinaryOperatorType::DivInPlace;
            case TokenType::Percent: return BinaryOperatorType::Mod;
            case TokenType::PercentEq: return BinaryOperatorType::ModInPlace;
            case TokenType::Ampersand: return BinaryOperatorType::And;
            case TokenType::AmpersandEq: return BinaryOperatorType::AndInPlace;
            case TokenType::DoubleAmpersand: return BinaryOperatorType::BitAnd;
            case TokenType::Pipe: return BinaryOperatorType::Or;
            case TokenType::PipeEq: return BinaryOperatorType::OrInPlace;
            case TokenType::DoublePipe: return BinaryOperatorType::BitOr;
            case TokenType::UpArrow: return BinaryOperatorType::Xor;
            case TokenType::UpArrowEq: return BinaryOperatorType::XorInPlace;
            case TokenType::Less: return BinaryOperatorType::Less;
            case TokenType::LessOrEq: return BinaryOperatorType::LessOrEq;
            case TokenType::Greater: return BinaryOperatorType::Greater;
            case TokenType::GreaterOrEq: return BinaryOperatorType::GreaterOrEq;
            case TokenType::Eq: return BinaryOperatorType::Eq;
            case TokenType::IsEq: return BinaryOperatorType::IsEq;
            case TokenType::IsNotEq: return BinaryOperatorType::IsNotEq;
            default: return BinaryOperatorType::Invalid;
        }
    }

    size_t Parser::GetBinaryPrecedence(BinaryOperatorType type) {
        switch (type) {
            case BinaryOperatorType::Eq:
            case BinaryOperatorType::AddInPlace:
            case BinaryOperatorType::SubInPlace:
            case BinaryOperatorType::MulInPlace:
            case BinaryOperatorType::ModInPlace:
            case BinaryOperatorType::DivInPlace:
            case BinaryOperatorType::AndInPlace:
            case BinaryOperatorType::OrInPlace:
            case BinaryOperatorType::XorInPlace:
                return 10;

            case BinaryOperatorType::Less:
            case BinaryOperatorType::LessOrEq:
            case BinaryOperatorType::Greater:
            case BinaryOperatorType::GreaterOrEq:
            case BinaryOperatorType::IsEq:
            case BinaryOperatorType::IsNotEq:
            case BinaryOperatorType::BitAnd:
            case BinaryOperatorType::BitOr:
                return 20;

            case BinaryOperatorType::And:
            case BinaryOperatorType::Or:
            case BinaryOperatorType::Xor:
                return 30;

            case BinaryOperatorType::Add:
            case BinaryOperatorType::Sub:
                return 40;

            case BinaryOperatorType::Mod:
            case BinaryOperatorType::Mul:
            case BinaryOperatorType::Div:
                return 50;
        }

        BLUA_ASSERT(false, "Unreachable");
        return -1;
    }

    NodeExpr* Parser::ParseValue() {
        if (!Peek()) { return nullptr; }
        Token& value = *Peek();
    
        NodeExpr* final = nullptr;

        switch (value.Type) {
            case TokenType::False: {
                Consume();
    
                final = Allocate<NodeExpr>(Allocate<ExprConstant>(Allocate<ConstantBool>(false)), value.Loc);
                break;
            }
    
            case TokenType::True: {
                Consume();
    
                final = Allocate<NodeExpr>(Allocate<ExprConstant>(Allocate<ConstantBool>(true)), value.Loc);
                break;
            }
    
            case TokenType::CharLit: {
                Consume();
    
                int8_t ch = static_cast<int8_t>(value.Data.Data()[0]);
                
                ConstantChar* node = Allocate<ConstantChar>(ch);
                final = Allocate<NodeExpr>(Allocate<ExprConstant>(node), value.Loc);
                break;
            }
    
            case TokenType::IntLit: {
                Consume();
    
                int32_t num = 0;
                auto [ptr, ec] = std::from_chars(value.Data.Data(), value.Data.Data() + value.Data.Size(), num);
    
                if (ec != std::errc()) {
                    ErrorTooLarge(value.Data);
                }
                ConstantInt* node = Allocate<ConstantInt>(num, false);
                
                final = Allocate<NodeExpr>(Allocate<ExprConstant>(node), value.Loc);
                break;
            }
    
            case TokenType::FloatLit: {
                Consume();
    
                float num = 0.0f;
                auto [ptr, ec] = std::from_chars(value.Data.Data(), value.Data.Data() + value.Data.Size(), num);
    
                if (ec != std::errc()) {
                    ErrorTooLarge(value.Data);
                }
                ConstantFloat* node = Allocate<ConstantFloat>(num);
                
                final = Allocate<NodeExpr>(Allocate<ExprConstant>(node), value.Loc);
                break;
            }
    
            case TokenType::StrLit: {
                Consume();
    
                StringView str = value.Data;
                ConstantString* node = Allocate<ConstantString>(str);
                
                final = Allocate<NodeExpr>(Allocate<ExprConstant>(node), value.Loc);
                break;
            }
    
            case TokenType::Minus: {
                Token m = Consume();
    
                NodeExpr* expr = ParseValue();
                ExprUnaryOperator* node = Allocate<ExprUnaryOperator>(expr, UnaryOperatorType::Negate);
    
                final = Allocate<NodeExpr>(node, SourceRange(m.Loc.Start, Peek(-1)->Loc.End, StringView(m.Loc.SourceString, Peek(-1)->Loc.SourceString)));
                break;
            }
    
            case TokenType::LeftParen: {
                Token p = Consume();
    
                if (IsVariableType()) {
                    StringBuilder type = ParseVariableType();

                    TryConsume(TokenType::RightParen, "')'");
                
                    ExprCast* node = Allocate<ExprCast>();
                    node->Type = StringView(type.Data(), type.Size());
                    node->Expression = ParseValue();
                
                    final = Allocate<NodeExpr>(node, SourceRange(p.Loc.Start, Peek(-1)->Loc.End, StringView(p.Loc.SourceString, Peek(-1)->Loc.SourceString)));
                } else {
                    NodeExpr* expr = ParseExpression();
                    ExprParen* node = Allocate<ExprParen>(expr);
                    
                    TryConsume(TokenType::RightParen, "')'");
                    
                    final = Allocate<NodeExpr>(node, SourceRange(p.Loc.Start, Peek(-1)->Loc.End, StringView(p.Loc.SourceString, Peek(-1)->Loc.SourceString)));
                }
                
                break;
            }

            case TokenType::Self: {
                Token s = Consume();

                final = Allocate<NodeExpr>(Allocate<ExprSelf>(), s.Loc);
                break;
            }
    
            case TokenType::Identifier: {
                Token i = Consume();

                // Check if this is a function call
                if (Match(TokenType::LeftParen)) {
                    Consume();
    
                    ExprCall* node = Allocate<ExprCall>();
                    node->Name = value.Data;
    
                    while (!Match(TokenType::RightParen)) {
                        NodeExpr* val = ParseExpression();
    
                        if (Match(TokenType::Comma)) {
                            Consume();
                        }
    
                        node->Arguments.Append(m_Context, Allocate<Node>(val));
                    }
    
                    TryConsume(TokenType::RightParen, "')'");
    
                    final = Allocate<NodeExpr>(node, SourceRange(i.Loc.Start, Peek(-1)->Loc.End, StringView(i.Loc.SourceString, Peek(-1)->Loc.SourceString)));
                } else {
                    ExprVarRef* varRef = Allocate<ExprVarRef>();
                    varRef->Identifier = value.Data;

                    final = Allocate<NodeExpr>(varRef, SourceRange(i.Loc.Start, Peek(-1)->Loc.End, StringView(i.Loc.SourceString, Peek(-1)->Loc.SourceString)));
                }

                break;
            }
        }

        // Handle member access (foo.bar) and array access (foo[5])
        while (Match(TokenType::Dot) || Match(TokenType::LeftBracket)) {
            Token op = Consume();

            if (op.Type == TokenType::Dot) {
                Token* member = TryConsume(TokenType::Identifier, "identifier");
                if (!member) { return nullptr; }

                if (Match(TokenType::LeftParen)) {
                    Consume();
    
                    ExprMethodCall* methodExpr = Allocate<ExprMethodCall>();
                    methodExpr->Member = member->Data;
                    methodExpr->Parent = final;
    
                    while (!Match(TokenType::RightParen)) {
                        NodeExpr* val = ParseExpression();
    
                        if (Match(TokenType::Comma)) {
                            Consume();
                        }
    
                        methodExpr->Arguments.Append(m_Context, Allocate<Node>(val));
                    }
    
                    TryConsume(TokenType::RightParen, "')'");
    
                    final = Allocate<NodeExpr>(methodExpr, SourceRange(final->Loc.Start, Peek(-1)->Loc.End, StringView(final->Loc.SourceString, Peek(-1)->Loc.SourceString)));
                } else {
                    ExprMember* memExpr = Allocate<ExprMember>();

                    memExpr->Member = member->Data;
                    memExpr->Parent = final;
                    final = Allocate<NodeExpr>(memExpr, SourceRange(final->Loc.Start, Peek(-1)->Loc.End, StringView(final->Loc.SourceString, Peek(-1)->Loc.SourceString)));
                }
            } else if (op.Type == TokenType::LeftBracket) {
                ExprArrayAccess* arrExpr = Allocate<ExprArrayAccess>();

                arrExpr->Index = ParseExpression();
                arrExpr->Parent = final;
                TryConsume(TokenType::RightBracket, "']'");

                final = Allocate<NodeExpr>(arrExpr, SourceRange(final->Loc.Start, Peek(-1)->Loc.End, StringView(final->Loc.SourceString, Peek(-1)->Loc.SourceString)));
            }
        }

        return final;
    }

    NodeExpr* Parser::ParseExpression(size_t minbp) {
        NodeExpr* lhsExpr = ParseValue();

        // There are two conditions to this loop
        // A valid operator (the first half of the check) and a valid precedence (the second half)
        while ((Peek() && ParseOperator() != BinaryOperatorType::Invalid) && 
               (GetBinaryPrecedence(ParseOperator()) >= minbp)) {
            BinaryOperatorType op = ParseOperator();
            Token o = Consume();

            NodeExpr* rhsExpr = ParseExpression(GetBinaryPrecedence(op) + 1);

            ExprBinaryOperator* newExpr = Allocate<ExprBinaryOperator>();
            newExpr->LHS = lhsExpr;
            newExpr->RHS = rhsExpr;
            newExpr->Type = op;

            lhsExpr = Allocate<NodeExpr>(newExpr, SourceRange(lhsExpr->Loc.Start, rhsExpr->Loc.End, StringView(lhsExpr->Loc.SourceString, rhsExpr->Loc.SourceString)));
        }

        return lhsExpr;
    }

    NodeStmt* Parser::ParseCompound() {
        StmtCompound* node = Allocate<StmtCompound>();

        Token* l = TryConsume(TokenType::LeftCurly, "'{'");

        while (!Match(TokenType::RightCurly)) {
            Node* n = ParseToken();
            node->Nodes.Append(m_Context, n);
        }

        TryConsume(TokenType::RightCurly, "'}'");

        return Allocate<NodeStmt>(node, SourceRange(l->Loc.Start, Peek(-1)->Loc.End, StringView(l->Loc.SourceString, Peek(-1)->Loc.SourceString)));
    }

    NodeStmt* Parser::ParseCompoundInline() {
        if (Match(TokenType::LeftCurly)) {
            return ParseCompound();
        } else {
            SourceRange start = Peek()->Loc;

            StmtCompound* node = Allocate<StmtCompound>();
            node->Nodes.Append(m_Context, ParseToken());
            return Allocate<NodeStmt>(node, SourceRange(start.Start, Peek(-1)->Loc.End, StringView(start.SourceString, Peek(-1)->Loc.SourceString)));
        }
    }

    NodeStmt* Parser::ParseType(bool external) {
        StringBuilder type = ParseVariableType();

        if (Peek(1)) {
            if (Peek(1)->Type == TokenType::LeftParen) {
                return ParseFunctionDecl(type, Peek(-1)->Loc, external);
            }
        }

        return ParseVariableDecl(type, Peek(-1)->Loc);
    }

    NodeStmt* Parser::ParseVariableDecl(StringBuilder type, SourceRange start) {
        Token* ident = TryConsume(TokenType::Identifier, "identifier");

        if (ident) {
            StmtVarDecl* node = Allocate<StmtVarDecl>();
            node->Identifier = ident->Data;
            node->Type = type;

            if (Match(TokenType::Eq)) {
                Consume();
                node->Value = ParseExpression();
            }

            return Allocate<NodeStmt>(node, SourceRange(start.Start, Peek(-1)->Loc.End, StringView(start.SourceString, Peek(-1)->Loc.SourceString)));
        } else {
            return nullptr;
        }
    }

    NodeStmt* Parser::ParseFunctionDecl(StringBuilder returnType, SourceRange start, bool external) {
        Token* ident = TryConsume(TokenType::Identifier, "identifier");

        if (ident) {
            NodeList params;

            if (Match(TokenType::LeftParen)) {
                Consume();
                params = ParseFunctionParameters();
                TryConsume(TokenType::RightParen, "')'");

                StmtFunctionDecl* node = Allocate<StmtFunctionDecl>();
                node->Name = ident->Data;
                node->ReturnType = returnType;
                node->Parameters = params;
                node->Extern = external;

                SourceLocation end = Peek(-1)->Loc.End;

                if (Match(TokenType::LeftCurly)) {
                    node->Body = ParseCompound();
                    m_NeedsSemi = false;
                }

                return Allocate<NodeStmt>(node, SourceRange(start.Start, end, StringView(start.SourceString, Peek(-1)->Loc.SourceString)));
            } else {
                ErrorExpected("'('");
            }

            return nullptr;
        } else {
            return nullptr;
        }
    }

    NodeStmt* Parser::ParseExtern() {
        Consume();

        return ParseType(true);
    }

    NodeStmt* Parser::ParseStructDecl() {
        Token s = Consume(); // Consume "struct"

        Token* ident = TryConsume(TokenType::Identifier, "indentifier");

        if (!ident) { return nullptr; }

        StmtStructDecl* node = Allocate<StmtStructDecl>();
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

                    StmtMethodDecl* decl = Allocate<StmtMethodDecl>();
                    decl->Name = fieldName->Data;
                    decl->ReturnType = type;

                    NodeList params = ParseFunctionParameters();
                    TryConsume(TokenType::RightParen, "')'");

                    decl->Parameters = params;
                    decl->Body = ParseCompound();

                    node->Fields.Append(m_Context, Allocate<Node>(Allocate<NodeStmt>(decl, fieldName->Loc)));
                } else {
                    StmtFieldDecl* decl = Allocate<StmtFieldDecl>();
                    decl->Identifier = fieldName->Data;
                    decl->Type = type;

                    TryConsume(TokenType::Semi, "';'");

                    node->Fields.Append(m_Context, Allocate<Node>(Allocate<NodeStmt>(decl, fieldName->Loc)));
                }
            }
        }

        TryConsume(TokenType::RightCurly, "'}'");

        return Allocate<NodeStmt>(node, SourceRange(s.Loc.Start, Peek(-1)->Loc.End, StringView(s.Loc.SourceString, Peek(-1)->Loc.SourceString)));
    }

    NodeStmt* Parser::ParseWhile() {
        Token w = Consume(); // Consume "while"

        StmtWhile* node = Allocate<StmtWhile>();

        TryConsume(TokenType::LeftParen, "'('");
        node->Condition = ParseExpression();
        SourceLocation end = Peek(-1)->Loc.End;
        TryConsume(TokenType::RightParen, "')'");
        node->Body = ParseCompoundInline();

        m_NeedsSemi = false;

        return Allocate<NodeStmt>(node, SourceRange(w.Loc.Start, end, StringView(w.Loc.SourceString, Peek(-1)->Loc.SourceString)));
    }

    NodeStmt* Parser::ParseDoWhile() {
        Token d = Consume(); // Consume "do"

        StmtDoWhile* node = Allocate<StmtDoWhile>();
        node->Body = ParseCompoundInline();

        TryConsume(TokenType::While, "while");
        TryConsume(TokenType::LeftParen, "'('");
        node->Condition = ParseExpression();
        TryConsume(TokenType::RightParen, "')'");

        m_NeedsSemi = false;

        return Allocate<NodeStmt>(node, d.Loc);
    }

    NodeStmt* Parser::ParseFor() {
        Token f = Consume(); // Consume "for"

        StmtFor* node = Allocate<StmtFor>();
        TryConsume(TokenType::LeftParen, "'('");

        node->Prologue = ParseStatement();

        node->Condition = ParseExpression();
        TryConsume(TokenType::Semi, "';'");

        node->Epilogue = ParseExpression();
        TryConsume(TokenType::RightParen, "')'");

        SourceLocation end = Peek(-1)->Loc.End;
        node->Body = ParseCompoundInline();

        m_NeedsSemi = false;

        return Allocate<NodeStmt>(node, SourceRange(f.Loc.Start, end, StringView(f.Loc.SourceString, Peek(-1)->Loc.SourceString)));
    }

    NodeStmt* Parser::ParseIf() {
        Token i = Consume(); // Consume "if"

        StmtIf* node = Allocate<StmtIf>();
        TryConsume(TokenType::LeftParen, "'('");

        node->Condition = ParseExpression();

        TryConsume(TokenType::RightParen, "')'");
        SourceLocation end = Peek(-1)->Loc.End;
        node->Body = ParseCompoundInline();

        if (Match(TokenType::Else)) {
            Consume();

            node->ElseBody = ParseCompoundInline();
        }

        m_NeedsSemi = false;

        return Allocate<NodeStmt>(node, SourceRange(i.Loc.Start, end, StringView(i.Loc.SourceString, Peek(-1)->Loc.SourceString)));
    }

    NodeStmt* Parser::ParseBreak() {
        Token b = Consume(); // Consume "break"

        return Allocate<NodeStmt>(nullptr, b.Loc);
    }

    NodeStmt* Parser::ParseContinue() {
        Token c = Consume(); // Consume "continue"

        return Allocate<NodeStmt>(nullptr, c.Loc);
    }

    NodeStmt* Parser::ParseReturn() {
        Token r = Consume(); // Consume "return"

        StmtReturn* node = Allocate<StmtReturn>();
        node->Value = ParseExpression();

        return Allocate<NodeStmt>(node, SourceRange(r.Loc.Start, Peek(-1)->Loc.End, StringView(r.Loc.SourceString, Peek(-1)->Loc.SourceString)));
    }

    NodeStmt* Parser::ParseStatement() {
        TokenType t = Peek()->Type;

        NodeStmt* node = nullptr;

        if (IsVariableType()) {
            node = ParseType();
        } else if (t == TokenType::Extern) {
            node = ParseExtern();
        } else if (t == TokenType::Struct) {
            node = ParseStructDecl();
        } else if (t == TokenType::LeftCurly) {
            node = ParseCompound();
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
        }

        return node;
    }

    Node* Parser::ParseToken() {
        Node* n = nullptr;

        if (NodeStmt* stmt = ParseStatement()) {
            n = Allocate<Node>(stmt);
        } else if (NodeExpr* expr = ParseExpression()) {
            n = Allocate<Node>(expr);
        }

        if (m_NeedsSemi) {
            TryConsume(TokenType::Semi, "';'");
        }

        m_NeedsSemi = true;

        return n;
    }

    void Parser::ErrorExpected(const StringView msg) {
        // if (Peek(-1)) {
        //     m_Context->ReportCompilerError(Peek(-1)->Line, Peek(-1)->Column, fmt::format("Expected {} after token \"{}\"", msg, TokenTypeToString(Peek(-1)->Type)));
        // } else {
        //     m_Context->ReportCompilerError(Peek()->Line, Peek()->Column, fmt::format("Expected {} after token \"{}\"", msg, TokenTypeToString(Peek()->Type)));
        // }
        
        m_Error = true;
    }

    void Parser::ErrorTooLarge(const StringView value) {
        // m_Context->ReportCompilerError(Peek(-1)->Line, Peek(-1)->Column, fmt::format("Constant {} is too large", value));
        m_Error = true;
    }

} // namespace BlackLua::Internal
