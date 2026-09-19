#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "ast.hpp"
#include "error.hpp"
#include "lexer.hpp"

// Recursive-descent parser. Each rule of the grammar in README.md is one
// member function, and operator precedence falls out of which function
// calls which: a function only ever calls the next-tighter level.
class Parser {
public:
    // Reads the first token right away, so a lex error at the very start
    // of the file is thrown from here.
    explicit Parser(Lexer& lexer);

    // Parses the whole file. Throws CompileError on the first error.
    Program parseProgram();

    // Deepest nesting accepted, counting every construct the parser handles
    // by recursion: parentheses, call arguments, unary operators, chained
    // assignment, blocks, and if/else/while bodies. Deeper input gets a
    // syntax error instead of overflowing the native stack. 256 matches
    // clang's default bracket depth.
    static constexpr int MAX_NESTING = 256;

private:
    // Held for the duration of each descent into a nested construct.
    class NestingGuard;

    // Token plumbing
    void advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    void expect(TokenType type, const char* context);
    std::string expectIdentifier(const char* what);

    std::string describeCurrent() const;
    CompileError errorAt(int line, const std::string& message) const;
    CompileError errorAtCurrent(const std::string& message) const;

    // Functions and statements
    Function parseFunction();
    std::vector<Param> parseParams();
    std::unique_ptr<Block> parseBlock();
    StmtPtr parseBlockItem();
    StmtPtr parseDeclaration();
    StmtPtr parseStatement();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseReturn();
    StmtPtr parseExprStatement();

    // Expressions, loosest-binding first
    ExprPtr parseExpr();
    ExprPtr parseAssign();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseRelational();
    ExprPtr parseAdditive();
    ExprPtr parseTerm();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();
    ExprPtr parseCall(int line, std::string callee);

    // Shared body of every left-associative binary level:
    //   operand (op operand)*
    ExprPtr parseLeftAssoc(
        ExprPtr (Parser::*operand)(),
        std::initializer_list<TokenType> ops
    );

    Lexer& lexer_;
    Token current_;
    Token previous_;
    int nesting_ = 0;
};
