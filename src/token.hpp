#pragma once

#include <string>

enum class TokenType {
    // Keywords
    KwInt,
    KwVoid,
    KwReturn,
    KwIf,
    KwElse,
    KwWhile,

    // Identifiers and literals
    Identifier,
    Integer,

    // Arithmetic
    Plus,
    Minus,
    Star,
    Slash,
    Percent,

    // Assignment and comparison
    Assign,
    EqualEqual,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    // Logical
    AndAnd,
    OrOr,
    Not,

    // Punctuation
    LParen,
    RParen,
    LBrace,
    RBrace,
    Semicolon,
    Comma,

    EndOfFile
};

struct Token {
    TokenType type = TokenType::EndOfFile;

    // Raw text as it appeared in the source.
    std::string text;

    // Only meaningful when type == Integer.
    long long value = 0;

    // 1-based line where this token starts. Every later phase
    // (parser, semantic analysis, codegen) reports errors with it.
    int line = 1;
};
