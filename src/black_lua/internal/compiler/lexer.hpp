#pragma once

#include "core.hpp"

#include <string>
#include <vector>

namespace BlackLua::Internal {

    enum class TokenType {
        Semi,
        Eq,
        LeftParen,
        RightParen,
        LeftBracket,
        RightBracket,
        LeftCurly,
        RightCurly,

        Plus, PlusEq, PlusPlus,
        Minus, MinusEq, MinusMinus,
        Star, StarEq,
        Slash, SlashEq,
        Percent,
        Hash,
        IsEq,
        IsNotEq,
        LessOrEq,
        GreaterOrEq,
        Less,
        Greater,

        Squigly,
        Comma,
        Colon,
        Dot,
        DoubleDot,
        TripleDot,

        And,
        Not,
        Or,

        If,
        Else,
        Then,
        End,

        While,
        Do,
        For,
        Repeat,
        Until,
        In,

        Break,
        Return,

        True,
        False,

        Struct,

        CharLit,
        IntLit,
        UIntLit,
        LongLit,
        ULongLit,
        FloatLit,
        DoubleLit,
        StrLit,

        Void,

        Bool,

        Char,
        UChar,
        Short,
        UShort,
        Int,
        UInt,
        Long,
        ULong,

        Float,
        Double,

        String,

        Extern,

        Identifier
    };

    inline const char* TokenTypeToString(TokenType type) {
        switch (type) {
            case TokenType::Semi: return ";";
            case TokenType::Eq: return "=";
            case TokenType::LeftParen: return "(";
            case TokenType::RightParen: return ")";
            case TokenType::LeftBracket: return "[";
            case TokenType::RightBracket: return "]";
            case TokenType::LeftCurly: return "{";
            case TokenType::RightCurly: return "}";

            case TokenType::Plus: return "+";
            case TokenType::PlusEq: return "+=";
            case TokenType::PlusPlus: return "++";
            case TokenType::Minus: return "-";
            case TokenType::MinusEq: return "-=";
            case TokenType::MinusMinus: return "--";
            case TokenType::Star: return "*";
            case TokenType::StarEq: return "*=";
            case TokenType::Slash: return "/";
            case TokenType::SlashEq: return "/=";
            case TokenType::Percent: return "%";
            case TokenType::Hash: return "#";
            case TokenType::IsEq: return "==";
            case TokenType::IsNotEq: return "~-";
            case TokenType::LessOrEq: return "<=";
            case TokenType::GreaterOrEq: return ">=";
            case TokenType::Less: return "<";
            case TokenType::Greater: return ">";

            case TokenType::Squigly: return "~";
            case TokenType::Comma: return ",";
            case TokenType::Colon: return ":";
            case TokenType::Dot: return ".";
            case TokenType::DoubleDot: return "..";
            case TokenType::TripleDot: return "...";

            case TokenType::And: return "and";
            case TokenType::Not: return "not";
            case TokenType::Or: return "or";

            case TokenType::If: return "if";
            case TokenType::Else: return "else";
            case TokenType::Then: return "then";
            case TokenType::End: return "end";

            case TokenType::While: return "while";
            case TokenType::Do: return "do";
            case TokenType::For: return "for";
            case TokenType::Repeat: return "repeat";
            case TokenType::Until: return "until";
            case TokenType::In: return "in";

            case TokenType::Break: return "break";
            case TokenType::Return: return "return";

            case TokenType::Struct: return "struct";

            case TokenType::True: return "true";
            case TokenType::False: return "false";

            case TokenType::IntLit: return "int-lit";
            case TokenType::UIntLit: return "uint-lit";
            case TokenType::LongLit: return "long-lit";
            case TokenType::ULongLit: return "ulong-lit";
            case TokenType::FloatLit: return "float-lit";
            case TokenType::DoubleLit: return "double-lit";
            case TokenType::StrLit: return "str-lit";

            case TokenType::Void: return "void";

            case TokenType::Bool: return "bool";

            case TokenType::Char: return "char";
            case TokenType::UChar: return "uchar";
            case TokenType::Short: return "short";
            case TokenType::UShort: return "ushort";
            case TokenType::Int: return "int";
            case TokenType::UInt: return "uint";
            case TokenType::Long: return "long";
            case TokenType::ULong: return "ulong";

            case TokenType::Float: return "float";
            case TokenType::Double: return "double";

            case TokenType::String: return "string";

            case TokenType::Extern: return "extern";

            case TokenType::Identifier: return "identifier";
        }

        BLUA_ASSERT(false, "Unreachable!");

        return nullptr;
    }

    struct Token {
        TokenType Type = TokenType::And;
        std::string Data;
        int Line = 0;
        int Column = 0;
    };

    class Lexer {
    public:
        using Tokens = std::vector<Token>;

        static Lexer Parse(const std::string& source);

        const Tokens& GetTokens() const;

    private:
        void ParseImpl();

        // "Peek" at the next character in the source string,
        // if it doesn't exist return NULL
        // "if (Peek())" is the same as if (m_Index < m_Source.size())
        char* Peek();

        // "Consume" the next character in the source string
        // NOTE: Consume is not allowed to be called after the end of the source string
        char Consume();

        void AddToken(TokenType type, const std::string& data = {});

    private:
        Tokens m_Tokens;
        size_t m_Index = 0;
        std::string m_Source;
        int m_CurrentLine = 1;
        int m_CurrentLineStart = 0; // The number of characters it takes to get to this line (from the start of the file)
    };

} // namespace BlackLua::Internal