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

namespace BlackLua {

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

                if (!foundToken) {
                    BLUA_TOKEN_DATA(buf, Identifier, buf); 
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

                AddToken(TokenType::NumLit, buf);

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

                BLUA_TOKEN_C('+', Add);
                BLUA_TOKEN_C('*', Mul);
                BLUA_TOKEN_C('/', Div);
                BLUA_TOKEN_C('%', Mod);
                BLUA_TOKEN_C('#', Hash);

                // Special case if there is a dash (-), this could be either a minus sign or a comment
                if (c == '-') {
                    bool isComment = false;

                    if (Peek()) {
                        char nc = *Peek(); // Don't consume the character just in case

                        if (nc == '-') {
                            Consume();
                            isComment = true;
                        }
                    }

                    if (!isComment) {
                        AddToken(TokenType::Sub);
                    } else {
                        while (Peek()) {
                            char nc = Consume();

                            if (nc == '\n' || nc == EOF) {
                                break;
                            }
                        }
                    }

                    continue;
                }

                // Special case if there is an equal sign (=), this could be either an assignment or conditional check
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

                // Special case if there is a less than sign (<), this could be either a less than or less than equal check (<=)
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

                continue;
            } else {
                Consume();
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
        m_Tokens.push_back(token);
    }

} // namespace BlackLua
