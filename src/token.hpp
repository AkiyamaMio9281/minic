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

    // Only meaningful when type == Integer. The lexer guarantees it fits
    // in a 32-bit int, the only integer type MiniC has.
    int value = 0;

    // 1-based line where this token starts. Every later phase
    // (parser, semantic analysis, codegen) reports errors with it.
    int line = 1;
};

// Enumerator name, e.g. "EqualEqual". Used by --tokens.
const char* tokenTypeName(TokenType type);

// Fixed source spelling, e.g. "==" or "while". Empty for Identifier,
// Integer and EndOfFile, whose text varies or does not exist.
const char* tokenSpelling(TokenType type);
