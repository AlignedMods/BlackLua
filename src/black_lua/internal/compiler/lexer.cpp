#include "internal/compiler/lexer.hpp"

#define BLUA_TOKEN_DATA(str, type) \
    if (buf == str) { \
        AddToken(TokenType::type, buf); \
        continue; \
    }

#define BLUA_TOKEN_POSSIBLE_EQ(base, noEq, yesEq) \
    case base: { \
        bool isEq = false; \
        \
        if (Peek()) { \
            char nc = *Peek(); \
            \
            if (nc == '=') { \
                Consume(); \
                isEq = true; \
            } \
            \
            if (isEq) { \
                AddToken(TokenType::yesEq); \
            } else { \
                AddToken(TokenType::noEq); \
            } \
        } \
        break; \
    }

namespace BlackLua::Internal {

    Lexer Lexer::Lex(const StringView source) {
        Lexer l;
        l.m_Source = source;
        l.LexImpl();
        
        return l;
    }

    const Lexer::Tokens& Lexer::GetTokens() const {
        return m_Tokens;
    }

    void Lexer::LexImpl() {
        while (Peek()) {
            if (std::isalpha(*Peek())) {
                size_t startIndex = m_Index;

                while (std::isalnum(*Peek()) || *Peek() == '_') {
                    Consume();
                }

                StringView buf(m_Source.Data() + startIndex, m_Index - startIndex);
                BLUA_TOKEN_DATA("if", If)
                BLUA_TOKEN_DATA("else", Else)

                BLUA_TOKEN_DATA("while", While)
                BLUA_TOKEN_DATA("do", Do)
                BLUA_TOKEN_DATA("for", For)

                BLUA_TOKEN_DATA("break", Break)
                BLUA_TOKEN_DATA("return", Return)

                BLUA_TOKEN_DATA("struct", Struct)

                BLUA_TOKEN_DATA("true", True)
                BLUA_TOKEN_DATA("false", False)

                BLUA_TOKEN_DATA("void", Void)

                BLUA_TOKEN_DATA("bool", Bool)

                BLUA_TOKEN_DATA("char", Char)
                BLUA_TOKEN_DATA("uchar", UChar)
                BLUA_TOKEN_DATA("short", Short)
                BLUA_TOKEN_DATA("ushort", UShort)
                BLUA_TOKEN_DATA("int", Int)
                BLUA_TOKEN_DATA("uint", UInt)
                BLUA_TOKEN_DATA("long", Long)
                BLUA_TOKEN_DATA("ulong", ULong)

                BLUA_TOKEN_DATA("float", Float)
                BLUA_TOKEN_DATA("double", Double)

                BLUA_TOKEN_DATA("string", String)

                BLUA_TOKEN_DATA("extern", Extern)

                // If all else fails, we say it's an identifier
                AddToken(TokenType::Identifier, buf);
            } else if (std::isdigit(*Peek())) {
                size_t startIndex = m_Index;
                size_t endIndex = 0;

                bool encounteredPeriod = false;
                bool isUnsigned = false;
                bool isLong = false;
                bool isFloat = false;

                while (Peek()) {
                    if (std::isdigit(*Peek())) {
                        Consume();
                    } else if (*Peek() == '.' && !encounteredPeriod) {
                        Consume();
                        encounteredPeriod = true;
                    } else {
                        break;
                    }
                }

                StringView buf(m_Source.Data() + startIndex, m_Index - startIndex);

                // Handle suffixes (u, l, f)
                while (Peek()) {
                    if (*Peek() == 'u') {
                        Consume();
                        isUnsigned = true;
                    } else if (*Peek() == 'l') {
                        Consume();
                        isLong = true;
                    } else if (*Peek() == 'f') {
                        Consume();
                        isFloat = true;
                    } else {
                        break;
                    }
                }

                if (isUnsigned && !isFloat) { // "u"
                    if (isLong) { // "lu/ul"
                        AddToken(TokenType::ULongLit, buf);
                        continue;
                    } else { // "u"
                        AddToken(TokenType::UIntLit, buf);
                        continue;
                    }
                } else if (isLong && !isFloat) { // "l"
                    AddToken(TokenType::LongLit, buf);
                    continue;
                } else if (isFloat && encounteredPeriod) { // "f"
                    AddToken(TokenType::FloatLit, buf);
                    continue;
                } else if (encounteredPeriod) { // No suffix but this can't be an integer literal
                    AddToken(TokenType::DoubleLit, buf);
                    continue;
                } else { // Finally if all else fails this is an integer literal
                    AddToken(TokenType::IntLit, buf);
                    continue;
                }

                continue;
            } else if (!std::isspace(*Peek())) {
                switch (Consume()) {
                    case ';': AddToken(TokenType::Semi); break;
                    case '(': AddToken(TokenType::LeftParen); break;
                    case ')': AddToken(TokenType::RightParen); break;
                    case '[': AddToken(TokenType::LeftBracket); break;
                    case ']': AddToken(TokenType::RightBracket); break;
                    case '{': AddToken(TokenType::LeftCurly); break;
                    case '}': AddToken(TokenType::RightCurly); break;
                    case '~': AddToken(TokenType::Squigly); break;
                    case ',': AddToken(TokenType::Comma); break;
                    case ':': AddToken(TokenType::Colon); break;
                    case '.': AddToken(TokenType::Dot); break;

                    BLUA_TOKEN_POSSIBLE_EQ('+', Plus, PlusEq)
                    BLUA_TOKEN_POSSIBLE_EQ('-', Minus, MinusEq)
                    BLUA_TOKEN_POSSIBLE_EQ('*', Star, StarEq)
                    BLUA_TOKEN_POSSIBLE_EQ('%', Percent, PercentEq)
                    BLUA_TOKEN_POSSIBLE_EQ('=', Eq, IsEq)
                    BLUA_TOKEN_POSSIBLE_EQ('!', Not, IsNotEq)
                    BLUA_TOKEN_POSSIBLE_EQ('<', Less, LessOrEq)
                    BLUA_TOKEN_POSSIBLE_EQ('>', Greater, GreaterOrEq)

                    case '/': {
                        bool isComment = false;
                        bool isEq = false;

                        if (Peek()) {
                            char nc = *Peek(); // Don't consume the character just in case

                            if (nc == '/') {
                                Consume();
                                isComment = true;
                            } else if (nc == '=') {
                                Consume();
                                isEq = true;
                            }
                        }

                        if (isComment) {
                            while (Peek()) {
                                char nc = Consume();

                                if (nc == '\n' || nc == EOF) {
                                    m_CurrentLine++;
                                    break;
                                }
                            }
                        } else if (isEq) {
                            AddToken(TokenType::SlashEq);
                        } else {
                            AddToken(TokenType::Slash);
                        }

                        break;
                    }

                    case '\'': {
                        size_t startIndex = m_Index;

                        if (Peek()) {
                            Consume();
                            
                            if (Peek() && *Peek() == '\'') {
                                Consume();
                            } else {
                                // TODO: Add error message
                            }
                        }

                        AddToken(TokenType::CharLit, StringView(m_Source.Data() + startIndex, 1));
                        break;
                    }

                    case '"': {
                        size_t startIndex = m_Index;

                        while (Peek()) {
                            char nc = Consume();
                        
                            if (nc == '"' || nc == EOF) {
                                break;
                            }
                        }

                        AddToken(TokenType::StrLit, StringView(m_Source.Data() + startIndex, m_Index - startIndex));
                        break;
                    }
                }
            } else {
                if (*Peek() == '\n') {
                    m_CurrentLine++;
                    m_CurrentLineStart = m_Index + 1;
                }

                Consume();

                continue;
            }
        }
    }

    const char* Lexer::Peek() {
        if (m_Index < m_Source.Size()) {
            return m_Source.Data() + m_Index;
        } else {
            return nullptr;
        }
    }

    char Lexer::Consume() {
        BLUA_ASSERT(m_Index < m_Source.Size(), "Consume out of bounds!");
        m_Index++;
        return m_Source.At(m_Index - 1);
    }

    void Lexer::AddToken(TokenType type, const StringView data) {
        Token token;
        token.Type = type;
        token.Data = data;
        token.Line = m_CurrentLine;
        token.Column = m_Index - m_CurrentLineStart;
        m_Tokens.push_back(token);
    }

} // namespace BlackLua::Internal
