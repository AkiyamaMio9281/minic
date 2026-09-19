#include "token.hpp"

const char* tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::KwInt:        return "KwInt";
        case TokenType::KwVoid:       return "KwVoid";
        case TokenType::KwReturn:     return "KwReturn";
        case TokenType::KwIf:         return "KwIf";
        case TokenType::KwElse:       return "KwElse";
        case TokenType::KwWhile:      return "KwWhile";

        case TokenType::Identifier:   return "Identifier";
        case TokenType::Integer:      return "Integer";

        case TokenType::Plus:         return "Plus";
        case TokenType::Minus:        return "Minus";
        case TokenType::Star:         return "Star";
        case TokenType::Slash:        return "Slash";
        case TokenType::Percent:      return "Percent";

        case TokenType::Assign:       return "Assign";
        case TokenType::EqualEqual:   return "EqualEqual";
        case TokenType::NotEqual:     return "NotEqual";
        case TokenType::Less:         return "Less";
        case TokenType::LessEqual:    return "LessEqual";
        case TokenType::Greater:      return "Greater";
        case TokenType::GreaterEqual: return "GreaterEqual";

        case TokenType::AndAnd:       return "AndAnd";
        case TokenType::OrOr:         return "OrOr";
        case TokenType::Not:          return "Not";

        case TokenType::LParen:       return "LParen";
        case TokenType::RParen:       return "RParen";
        case TokenType::LBrace:       return "LBrace";
        case TokenType::RBrace:       return "RBrace";
        case TokenType::Semicolon:    return "Semicolon";
        case TokenType::Comma:        return "Comma";

        case TokenType::EndOfFile:    return "EndOfFile";
    }

    return "Unknown";
}

const char* tokenSpelling(TokenType type) {
    switch (type) {
        case TokenType::KwInt:        return "int";
        case TokenType::KwVoid:       return "void";
        case TokenType::KwReturn:     return "return";
        case TokenType::KwIf:         return "if";
        case TokenType::KwElse:       return "else";
        case TokenType::KwWhile:      return "while";

        case TokenType::Identifier:   return "";
        case TokenType::Integer:      return "";

        case TokenType::Plus:         return "+";
        case TokenType::Minus:        return "-";
        case TokenType::Star:         return "*";
        case TokenType::Slash:        return "/";
        case TokenType::Percent:      return "%";

        case TokenType::Assign:       return "=";
        case TokenType::EqualEqual:   return "==";
        case TokenType::NotEqual:     return "!=";
        case TokenType::Less:         return "<";
        case TokenType::LessEqual:    return "<=";
        case TokenType::Greater:      return ">";
        case TokenType::GreaterEqual: return ">=";

        case TokenType::AndAnd:       return "&&";
        case TokenType::OrOr:         return "||";
        case TokenType::Not:          return "!";

        case TokenType::LParen:       return "(";
        case TokenType::RParen:       return ")";
        case TokenType::LBrace:       return "{";
        case TokenType::RBrace:       return "}";
        case TokenType::Semicolon:    return ";";
        case TokenType::Comma:        return ",";

        case TokenType::EndOfFile:    return "";
    }

    return "";
}
