#pragma once

#include <cstddef>
#include <string>

#include "error.hpp"
#include "token.hpp"

class Lexer {
public:
    explicit Lexer(std::string source);

    // Produces the next token. Returns EndOfFile forever once the
    // input is exhausted. Throws CompileError on a lex error.
    Token nextToken();

private:
    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);

    void skipIgnored();

    Token lexNumber();
    Token lexIdentifier();

    Token make(TokenType type, std::string text) const;
    CompileError error(const std::string& message) const;
    CompileError errorAt(int line, const std::string& message) const;

    std::string source_;
    std::size_t pos_ = 0;

    // Line currently being scanned.
    int line_ = 1;

    // Line on which the token being built started.
    int tokenLine_ = 1;
};
