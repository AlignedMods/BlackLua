#include "compiler.hpp"

#define BLUA_TOKEN(str, type)        \
    if (buf == str) {                \
        AddToken(TokenType::type);   \
        foundToken = true;           \
        continue;                    \
    }

#define BLUA_TOKEN_C(character, type) \
    if (c == character) {             \
        AddToken(TokenType::type);    \
        continue;                     \
    }

#define BLUA_TOKEN_DATA(str, type, data)   \
    if (buf == str) {                      \
        AddToken(TokenType::type, data);   \
        foundToken = true;                 \
        continue;                          \
    }

namespace BlackLua::Internal {

    Lexer Lexer::Parse(const std::string& source) {
        Lexer l;
        l.m_Source = source;
        l.ParseImpl();
        
        return l;
    }

    const Lexer::Tokens& Lexer::GetTokens() const {
        return m_Tokens;
    }

    void Lexer::ParseImpl() {
        while (Peek()) {
            std::string buf;

            if (std::isalpha(*Peek())) {
                buf += Consume();

                while (Peek()) {
                    if (std::isalnum(*Peek())) {
                        buf += Consume();
                    } else {
                        break;
                    }
                }

                bool foundToken = false;

                BLUA_TOKEN("and", And);
                BLUA_TOKEN("not", Not);
                BLUA_TOKEN("or", Or);

                BLUA_TOKEN("if", If);
                BLUA_TOKEN("else", Else);
                BLUA_TOKEN("elseif", ElseIf);
                BLUA_TOKEN("then", Then);
                BLUA_TOKEN("end", End);

                BLUA_TOKEN("while", While);
                BLUA_TOKEN("do", Do);
                BLUA_TOKEN("for", For);
                BLUA_TOKEN("repeat", Repeat);
                BLUA_TOKEN("until", Until);
                BLUA_TOKEN("in", In);

                BLUA_TOKEN("break", Break);
                BLUA_TOKEN("return", Return);

                BLUA_TOKEN("true", True);
                BLUA_TOKEN("false", False);
                BLUA_TOKEN("nil", Nil);

                BLUA_TOKEN("function", Function);
                BLUA_TOKEN("local", Local);

                BLUA_TOKEN("bool", Bool);

                BLUA_TOKEN("int", Int);
                BLUA_TOKEN("long", Long);

                BLUA_TOKEN("float", Float);
                BLUA_TOKEN("double", Double);

                BLUA_TOKEN("string", String);

                if (!foundToken) {
                    // BLUA_TOKEN_DATA(buf, Identifier, buf); 
                    AddToken(TokenType::Identifier, buf);
                    foundToken = true;
                    continue;
                }

                continue;
            } else if (std::isdigit(*Peek())) {
                buf += Consume();

                bool encounteredPeriod = false;

                while (Peek()) {
                    if (std::isdigit(*Peek())) {
                        buf += Consume();
                    } else if (*Peek() == '.' && !encounteredPeriod) {
                        buf += Consume();
                        encounteredPeriod = true;
                    } else {
                        break;
                    }
                }

                if (encounteredPeriod) {
                    AddToken(TokenType::NumLit, buf);
                } else {
                    AddToken(TokenType::IntLit, buf);
                }

                continue;
            } else if (!std::isspace(*Peek())) {
                char c = Consume();

                // NOTE: There are some special tokens (like -, <, >, =) which have to go through more testing
                BLUA_TOKEN_C(';', Semi);
                BLUA_TOKEN_C('(', LeftParen);
                BLUA_TOKEN_C(')', RightParen);
                BLUA_TOKEN_C('[', LeftBracket);
                BLUA_TOKEN_C(']', RightBracket);
                BLUA_TOKEN_C('{', LeftCurly);
                BLUA_TOKEN_C('}', RightCurly);

                if (c == '+') {
                    bool isInPlace = false;

                    if (Peek()) {
                        char nc = *Peek();

                        if (nc == '=') {
                            Consume();
                            isInPlace = true;
                        }

                        if (isInPlace) {
                            AddToken(TokenType::AddInPlace);
                        } else {
                            AddToken(TokenType::Add);
                        }
                    }
                }

                if (c == '-') {
                    bool isInPlace = false;

                    if (Peek()) {
                        char nc = *Peek();

                        if (nc == '=') {
                            Consume();
                            isInPlace = true;
                        }

                        if (isInPlace) {
                            AddToken(TokenType::SubInPlace);
                        } else {
                            AddToken(TokenType::Sub);
                        }
                    }
                }

                if (c == '*') {
                    bool isInPlace = false;

                    if (Peek()) {
                        char nc = *Peek();

                        if (nc == '=') {
                            Consume();
                            isInPlace = true;
                        }

                        if (isInPlace) {
                            AddToken(TokenType::MulInPlace);
                        } else {
                            AddToken(TokenType::Mul);
                        }
                    }
                }

                if (c == '/') {
                    bool isComment = false;
                    bool isInPlace = false;

                    if (Peek()) {
                        char nc = *Peek(); // Don't consume the character just in case

                        if (nc == '/') {
                            Consume();
                            isComment = true;
                        } else if (nc == '=') {
                            Consume();
                            isInPlace = true;
                        }
                    }

                    if (isComment) {
                        while (Peek()) {
                            char nc = Consume();

                            if (nc == '\n' || nc == EOF) {
                                break;
                            }
                        }
                    } else if (isInPlace) {
                        AddToken(TokenType::DivInPlace);
                    } else {
                        AddToken(TokenType::Div);
                    }

                    continue;
                }

                if (c == '=') {
                    bool isEq = false;

                    if (Peek()) {
                        char nc = *Peek();

                        if (nc == '=') {
                            isEq = true;
                        }
                    }

                    if (isEq) {
                        AddToken(TokenType::IsEq);
                        continue;
                    } else {
                        AddToken(TokenType::Eq);
                        continue;
                    }
                }

                if (c == '<') {
                    bool isEq = false;

                    if (Peek()) {
                        char nc = *Peek();

                        if (nc == '=') {
                            isEq = true;
                        }
                    }

                    if (isEq) {
                        AddToken(TokenType::LessOrEq);
                        continue;
                    } else {
                        AddToken(TokenType::Less);
                        continue;
                    }
                }

                if (c == '>') {
                    bool isEq = false;

                    if (Peek()) {
                        char nc = *Peek();

                        if (nc == '=') {
                            isEq = true;
                        }
                    }

                    if (isEq) {
                        AddToken(TokenType::GreaterOrEq);
                        continue;
                    } else {
                        AddToken(TokenType::Greater);
                        continue;
                    }
                }

                if (c == '"') {
                    while (Peek()) {
                        char nc = Consume();
                    
                        if (nc == '"' || nc == EOF) {
                            break;
                        }

                        buf += nc;
                    }

                    AddToken(TokenType::StrLit, buf);

                    continue;
                }

                continue;
            } else {
                char c = Consume();

                if (c == '\n') {
                    m_CurrentLine++;
                }

                continue;
            }
        }
    }

    char* Lexer::Peek() {
        if (m_Index < m_Source.size()) {
            return &m_Source.at(m_Index);
        } else {
            return nullptr;
        }
    }

    char Lexer::Consume() {
        BLUA_ASSERT(m_Index < m_Source.size(), "Consume out of bounds!");

        return m_Source.at(m_Index++);
    }

    void Lexer::AddToken(TokenType type, const std::string& data) {
        Token token;
        token.Type = type;
        token.Data = data;
        token.Line = m_CurrentLine;
        m_Tokens.push_back(token);
    }

} // namespace BlackLua::Internal
