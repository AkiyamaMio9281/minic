#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "token.hpp"

// Every node records its kind. Later phases switch on `kind` and
// static_cast to the concrete type, so no visitor boilerplate is needed.
// Every node also records the line it starts on, for error messages.

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

struct Expr {
    enum class Kind {
        IntLiteral,
        Variable,
        Unary,
        Binary,
        Assign,
        Call,
    };

    const Kind kind;
    const int line;

    virtual ~Expr() = default;

protected:
    Expr(Kind k, int ln) : kind(k), line(ln) {}
};

using ExprPtr = std::unique_ptr<Expr>;

struct IntLiteral : Expr {
    int value;

    IntLiteral(int ln, int v)
        : Expr(Kind::IntLiteral, ln), value(v) {}
};

struct Variable : Expr {
    std::string name;

    Variable(int ln, std::string n)
        : Expr(Kind::Variable, ln), name(std::move(n)) {}
};

// op is Minus or Not.
struct Unary : Expr {
    TokenType op;
    ExprPtr operand;

    Unary(int ln, TokenType o, ExprPtr e)
        : Expr(Kind::Unary, ln), op(o), operand(std::move(e)) {}
};

// op is one of + - * / % == != < <= > >= && ||.
struct Binary : Expr {
    TokenType op;
    ExprPtr lhs;
    ExprPtr rhs;

    Binary(int ln, TokenType o, ExprPtr l, ExprPtr r)
        : Expr(Kind::Binary, ln), op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};

// The grammar only allows a plain variable left of '=', so the target is
// kept as a name rather than as an arbitrary expression.
struct Assign : Expr {
    std::string name;
    ExprPtr value;

    Assign(int ln, std::string n, ExprPtr v)
        : Expr(Kind::Assign, ln), name(std::move(n)), value(std::move(v)) {}
};

struct Call : Expr {
    std::string callee;
    std::vector<ExprPtr> args;

    Call(int ln, std::string c)
        : Expr(Kind::Call, ln), callee(std::move(c)) {}
};

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------

struct Stmt {
    enum class Kind {
        VarDecl,
        ExprStmt,
        If,
        While,
        Return,
        Block,
    };

    const Kind kind;
    const int line;

    virtual ~Stmt() = default;

protected:
    Stmt(Kind k, int ln) : kind(k), line(ln) {}
};

using StmtPtr = std::unique_ptr<Stmt>;

// int name = init;    init is null when there is no initializer.
struct VarDecl : Stmt {
    std::string name;
    ExprPtr init;

    VarDecl(int ln, std::string n, ExprPtr i)
        : Stmt(Kind::VarDecl, ln), name(std::move(n)), init(std::move(i)) {}
};

// expr;    expr is null for the empty statement ";".
struct ExprStmt : Stmt {
    ExprPtr expr;

    ExprStmt(int ln, ExprPtr e)
        : Stmt(Kind::ExprStmt, ln), expr(std::move(e)) {}
};

// elseBranch is null when there is no else.
struct If : Stmt {
    ExprPtr cond;
    StmtPtr thenBranch;
    StmtPtr elseBranch;

    If(int ln, ExprPtr c, StmtPtr t, StmtPtr e)
        : Stmt(Kind::If, ln),
          cond(std::move(c)),
          thenBranch(std::move(t)),
          elseBranch(std::move(e)) {}
};

struct While : Stmt {
    ExprPtr cond;
    StmtPtr body;

    While(int ln, ExprPtr c, StmtPtr b)
        : Stmt(Kind::While, ln), cond(std::move(c)), body(std::move(b)) {}
};

// value is null for "return;".
struct Return : Stmt {
    ExprPtr value;

    Return(int ln, ExprPtr v)
        : Stmt(Kind::Return, ln), value(std::move(v)) {}
};

struct Block : Stmt {
    std::vector<StmtPtr> items;

    explicit Block(int ln) : Stmt(Kind::Block, ln) {}
};

// ---------------------------------------------------------------------------
// Top level
// ---------------------------------------------------------------------------

struct Param {
    std::string name;
    int line = 0;
};

struct Function {
    bool returnsVoid = false;
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<Block> body;
    int line = 0;
};

struct Program {
    std::vector<Function> functions;
};

// Writes the tree with two spaces of indentation per level, one node per
// line. This is what --ast prints.
void printAst(const Program& program, std::ostream& out);
