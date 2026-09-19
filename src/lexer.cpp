#include "lexer.hpp"

#include <cctype>
#include <climits>
#include <utility>

namespace {

struct Keyword {
    const char* text;
    TokenType type;
};

// Add a keyword by adding one row here.
const Keyword KEYWORDS[] = {
    {"int",    TokenType::KwInt},
    {"void",   TokenType::KwVoid},
    {"return", TokenType::KwReturn},
    {"if",     TokenType::KwIf},
    {"else",   TokenType::KwElse},
    {"while",  TokenType::KwWhile},
};

bool isIdentStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool isIdentPart(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool isDigit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

}  // namespace

Lexer::Lexer(std::string source)
    : source_(std::move(source)) {}

char Lexer::peek() const {
    if (pos_ >= source_.size()) {
        return '\0';
    }

    return source_[pos_];
}

char Lexer::peekNext() const {
    if (pos_ + 1 >= source_.size()) {
        return '\0';
    }

    return source_[pos_ + 1];
}

char Lexer::advance() {
    if (pos_ >= source_.size()) {
        return '\0';
    }

    const char c = source_[pos_++];

    if (c == '\n') {
        ++line_;
    }

    return c;
}

bool Lexer::match(char expected) {
    if (peek() != expected) {
        return false;
    }

    advance();
    return true;
}

Token Lexer::make(TokenType type, std::string text) const {
    Token token;
    token.type = type;
    token.text = std::move(text);
    token.line = tokenLine_;

    return token;
}

CompileError Lexer::errorAt(int line, const std::string& message) const {
    return CompileError("lex", line, message);
}

CompileError Lexer::error(const std::string& message) const {
    return errorAt(line_, message);
}

void Lexer::skipIgnored() {
    while (true) {
        // Whitespace
        while (std::isspace(static_cast<unsigned char>(peek()))) {
            advance();
        }

        // Line comment: // to end of line
        if (peek() == '/' && peekNext() == '/') {
            while (peek() != '\n' && peek() != '\0') {
                advance();
            }

            continue;
        }

        // Block comment: /* to */
        if (peek() == '/' && peekNext() == '*') {
            // Report the opening line, not wherever the input ran out.
            const int openedOn = line_;

            advance();
            advance();

            while (!(peek() == '*' && peekNext() == '/')) {
                if (peek() == '\0') {
                    throw errorAt(openedOn, "unterminated block comment");
                }

                advance();
            }

            advance();
            advance();
            continue;
        }

        break;
    }
}

Token Lexer::lexNumber() {
    const std::size_t start = pos_;

    while (isDigit(peek())) {
        advance();
    }

    // "123abc" is not two tokens, it is a malformed number.
    if (isIdentStart(peek())) {
        throw error("digit cannot be followed by an identifier character");
    }

    std::string text = source_.substr(start, pos_ - start);

    // In C a leading 0 means octal, so 010 is 8. MiniC has no octal, and
    // quietly reading 010 as ten would disagree with every C compiler.
    if (text.size() > 1 && text[0] == '0') {
        throw error(
            "integer literal '" + text + "' has a leading zero; octal is not supported"
        );
    }

    // int is 32 bits. Checking after every digit stops long before the
    // accumulator itself could overflow, however long the literal is.
    long long value = 0;

    for (const char digit : text) {
        value = value * 10 + (digit - '0');

        if (value > INT_MAX) {
            throw error(
                "integer literal '" + text + "' does not fit in int (max 2147483647)"
            );
        }
    }

    Token token = make(TokenType::Integer, std::move(text));
    token.value = static_cast<int>(value);

    return token;
}

Token Lexer::lexIdentifier() {
    const std::size_t start = pos_;

    while (isIdentPart(peek())) {
        advance();
    }

    std::string text = source_.substr(start, pos_ - start);

    for (const Keyword& keyword : KEYWORDS) {
        if (text == keyword.text) {
            return make(keyword.type, std::move(text));
        }
    }

    return make(TokenType::Identifier, std::move(text));
}

Token Lexer::nextToken() {
    skipIgnored();

    // Fix the token's line before consuming any of its characters.
    tokenLine_ = line_;

    if (peek() == '\0') {
        return make(TokenType::EndOfFile, "");
    }

    const char c = peek();

    if (isIdentStart(c)) {
        return lexIdentifier();
    }

    if (isDigit(c)) {
        return lexNumber();
    }

    advance();

    switch (c) {
        case '+':
            return make(TokenType::Plus, "+");

        case '-':
            return make(TokenType::Minus, "-");

        case '*':
            return make(TokenType::Star, "*");

        case '/':
            return make(TokenType::Slash, "/");

        case '%':
            return make(TokenType::Percent, "%");

        case '(':
            return make(TokenType::LParen, "(");

        case ')':
            return make(TokenType::RParen, ")");

        case '{':
            return make(TokenType::LBrace, "{");

        case '}':
            return make(TokenType::RBrace, "}");

        case ';':
            return make(TokenType::Semicolon, ";");

        case ',':
            return make(TokenType::Comma, ",");

        case '=':
            if (match('=')) {
                return make(TokenType::EqualEqual, "==");
            }

            return make(TokenType::Assign, "=");

        case '!':
            if (match('=')) {
                return make(TokenType::NotEqual, "!=");
            }

            return make(TokenType::Not, "!");

        case '<':
            if (match('=')) {
                return make(TokenType::LessEqual, "<=");
            }

            return make(TokenType::Less, "<");

        case '>':
            if (match('=')) {
                return make(TokenType::GreaterEqual, ">=");
            }

            return make(TokenType::Greater, ">");

        case '&':
            if (match('&')) {
                return make(TokenType::AndAnd, "&&");
            }

            throw error("expected '&&'; bitwise '&' is not part of MiniC");

        case '|':
            if (match('|')) {
                return make(TokenType::OrOr, "||");
            }

            throw error("expected '||'; bitwise '|' is not part of MiniC");

        default:
            break;
    }

    throw error("unknown character: '" + std::string(1, c) + "'");
}
