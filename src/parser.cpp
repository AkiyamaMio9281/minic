#include "parser.hpp"

#include <algorithm>
#include <string>
#include <utility>

class Parser::NestingGuard {
public:
    explicit NestingGuard(Parser& parser)
        : parser_(parser) {
        if (parser_.nesting_ == MAX_NESTING) {
            throw parser_.errorAt(
                parser_.current_.line,
                "nesting is too deep (the limit is " +
                    std::to_string(MAX_NESTING) + " levels)"
            );
        }

        ++parser_.nesting_;
    }

    ~NestingGuard() {
        --parser_.nesting_;
    }

    NestingGuard(const NestingGuard&) = delete;
    NestingGuard& operator=(const NestingGuard&) = delete;

private:
    Parser& parser_;
};

Parser::Parser(Lexer& lexer)
    : lexer_(lexer) {
    advance();
}

// ---------------------------------------------------------------------------
// Token plumbing
// ---------------------------------------------------------------------------

void Parser::advance() {
    previous_ = std::move(current_);
    current_ = lexer_.nextToken();
}

bool Parser::check(TokenType type) const {
    return current_.type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) {
        return false;
    }

    advance();
    return true;
}

// A missing token belongs right after the previous one, so that is the
// line reported. A ';' missing at the end of line 3 is reported on line 3,
// not on line 4 where the next token happens to sit.
void Parser::expect(TokenType type, const char* context) {
    if (match(type)) {
        return;
    }

    throw errorAt(
        previous_.line,
        std::string("expected '") + tokenSpelling(type) + "' " + context +
            ", found " + describeCurrent()
    );
}

std::string Parser::expectIdentifier(const char* what) {
    if (!check(TokenType::Identifier)) {
        throw errorAtCurrent(std::string("expected ") + what);
    }

    std::string name = current_.text;
    advance();

    return name;
}

std::string Parser::describeCurrent() const {
    if (current_.type == TokenType::EndOfFile) {
        return "end of file";
    }

    return "'" + current_.text + "'";
}

CompileError Parser::errorAt(int line, const std::string& message) const {
    return CompileError("syntax", line, message);
}

CompileError Parser::errorAtCurrent(const std::string& message) const {
    return errorAt(current_.line, message + ", found " + describeCurrent());
}

// ---------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------

Program Parser::parseProgram() {
    Program program;

    while (!check(TokenType::EndOfFile)) {
        program.functions.push_back(parseFunction());
    }

    return program;
}

Function Parser::parseFunction() {
    Function function;

    if (match(TokenType::KwInt)) {
        function.returnsVoid = false;
    } else if (match(TokenType::KwVoid)) {
        function.returnsVoid = true;
    } else {
        throw errorAtCurrent(
            "expected a function definition starting with 'int' or 'void'"
        );
    }

    function.line = current_.line;
    function.name = expectIdentifier("a function name");

    // "int x;" at the top level would be a global variable.
    if (check(TokenType::Semicolon) ||
        check(TokenType::Assign) ||
        check(TokenType::Comma)) {
        throw errorAt(
            function.line,
            "global variable '" + function.name +
                "' is not supported; declare it inside a function"
        );
    }

    expect(TokenType::LParen, "after function name");
    function.params = parseParams();
    expect(TokenType::RParen, "after parameter list");

    // "int f(int a);" is a prototype. MiniC resolves calls after the whole
    // file is parsed, so it never needs one.
    if (check(TokenType::Semicolon)) {
        throw errorAt(
            function.line,
            "function '" + function.name + "' has no body; MiniC has no "
                "prototypes because a function can be called before its definition"
        );
    }

    function.body = parseBlock();

    return function;
}

std::vector<Param> Parser::parseParams() {
    std::vector<Param> params;

    if (check(TokenType::RParen)) {
        return params;
    }

    // f(void) is C's way of writing "no parameters".
    if (match(TokenType::KwVoid)) {
        if (!check(TokenType::RParen)) {
            throw errorAt(
                previous_.line,
                "'void' must be the only thing in a parameter list"
            );
        }

        return params;
    }

    do {
        if (!check(TokenType::KwInt)) {
            throw errorAtCurrent("expected parameter type 'int'");
        }

        advance();

        Param param;
        param.line = current_.line;
        param.name = expectIdentifier("a parameter name");
        params.push_back(std::move(param));
    } while (match(TokenType::Comma));

    return params;
}

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------

std::unique_ptr<Block> Parser::parseBlock() {
    const int openLine = current_.line;
    expect(TokenType::LBrace, "to start a block");

    auto block = std::make_unique<Block>(openLine);

    while (!check(TokenType::RBrace)) {
        // Report where the block opened. The end of the file says nothing
        // about which brace was left unclosed.
        if (check(TokenType::EndOfFile)) {
            throw errorAt(openLine, "'{' is never closed");
        }

        block->items.push_back(parseBlockItem());
    }

    advance();

    return block;
}

// A block holds declarations and statements. A declaration is not itself
// a statement, which is why "if (x) int y;" is rejected in parseStatement.
StmtPtr Parser::parseBlockItem() {
    if (check(TokenType::KwInt)) {
        return parseDeclaration();
    }

    if (check(TokenType::KwVoid)) {
        throw errorAt(current_.line, "a variable cannot have type 'void'");
    }

    return parseStatement();
}

StmtPtr Parser::parseDeclaration() {
    const int line = current_.line;
    advance();

    std::string name = expectIdentifier("a variable name after 'int'");

    ExprPtr init;

    if (match(TokenType::Assign)) {
        init = parseExpr();
    }

    if (check(TokenType::Comma)) {
        throw errorAt(
            current_.line,
            "declare one variable per statement; 'int a, b;' is not supported"
        );
    }

    expect(TokenType::Semicolon, "after variable declaration");

    return std::make_unique<VarDecl>(line, std::move(name), std::move(init));
}

StmtPtr Parser::parseStatement() {
    NestingGuard guard(*this);

    switch (current_.type) {
        case TokenType::KwIf:
            return parseIf();

        case TokenType::KwWhile:
            return parseWhile();

        case TokenType::KwReturn:
            return parseReturn();

        case TokenType::LBrace:
            return parseBlock();

        case TokenType::KwElse:
            throw errorAt(current_.line, "'else' without a matching 'if'");

        // Only reachable as the body of if, else or while. Blocks handle
        // declarations themselves in parseBlockItem.
        case TokenType::KwInt:
            throw errorAt(
                current_.line,
                "a declaration cannot be the body of 'if', 'else' or 'while'; "
                    "wrap it in { }"
            );

        default:
            return parseExprStatement();
    }
}

StmtPtr Parser::parseIf() {
    const int line = current_.line;
    advance();

    expect(TokenType::LParen, "after 'if'");
    ExprPtr cond = parseExpr();
    expect(TokenType::RParen, "after if condition");

    StmtPtr thenBranch = parseStatement();

    // Checking for else right here, before returning to any enclosing if,
    // is what binds an else to the nearest unmatched if.
    StmtPtr elseBranch;

    if (match(TokenType::KwElse)) {
        elseBranch = parseStatement();
    }

    return std::make_unique<If>(
        line, std::move(cond), std::move(thenBranch), std::move(elseBranch)
    );
}

StmtPtr Parser::parseWhile() {
    const int line = current_.line;
    advance();

    expect(TokenType::LParen, "after 'while'");
    ExprPtr cond = parseExpr();
    expect(TokenType::RParen, "after while condition");

    StmtPtr body = parseStatement();

    return std::make_unique<While>(line, std::move(cond), std::move(body));
}

StmtPtr Parser::parseReturn() {
    const int line = current_.line;
    advance();

    ExprPtr value;

    if (!check(TokenType::Semicolon)) {
        value = parseExpr();
    }

    expect(TokenType::Semicolon, "after return statement");

    return std::make_unique<Return>(line, std::move(value));
}

StmtPtr Parser::parseExprStatement() {
    const int line = current_.line;

    if (match(TokenType::Semicolon)) {
        return std::make_unique<ExprStmt>(line, nullptr);
    }

    ExprPtr expr = parseExpr();
    expect(TokenType::Semicolon, "after expression");

    return std::make_unique<ExprStmt>(line, std::move(expr));
}

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

ExprPtr Parser::parseExpr() {
    return parseAssign();
}

ExprPtr Parser::parseAssign() {
    ExprPtr target = parseOr();

    if (!check(TokenType::Assign)) {
        return target;
    }

    const int assignLine = current_.line;
    advance();

    if (target->kind != Expr::Kind::Variable) {
        throw errorAt(assignLine, "left side of '=' must be a variable");
    }

    // Recursing into parseAssign, not parseOr, makes '=' right-associative:
    // a = b = 0 parses as a = (b = 0).
    NestingGuard guard(*this);
    ExprPtr value = parseAssign();

    auto& var = static_cast<Variable&>(*target);

    return std::make_unique<Assign>(var.line, std::move(var.name), std::move(value));
}

ExprPtr Parser::parseLeftAssoc(
    ExprPtr (Parser::*operand)(),
    std::initializer_list<TokenType> ops
) {
    ExprPtr lhs = (this->*operand)();

    while (std::find(ops.begin(), ops.end(), current_.type) != ops.end()) {
        const TokenType op = current_.type;
        const int line = current_.line;
        advance();

        // Folding into lhs on every iteration is what makes the level
        // left-associative: a - b - c parses as (a - b) - c.
        ExprPtr rhs = (this->*operand)();
        lhs = std::make_unique<Binary>(line, op, std::move(lhs), std::move(rhs));
    }

    return lhs;
}

ExprPtr Parser::parseOr() {
    return parseLeftAssoc(&Parser::parseAnd, {TokenType::OrOr});
}

ExprPtr Parser::parseAnd() {
    return parseLeftAssoc(&Parser::parseEquality, {TokenType::AndAnd});
}

ExprPtr Parser::parseEquality() {
    return parseLeftAssoc(
        &Parser::parseRelational,
        {TokenType::EqualEqual, TokenType::NotEqual}
    );
}

ExprPtr Parser::parseRelational() {
    return parseLeftAssoc(
        &Parser::parseAdditive,
        {TokenType::Less, TokenType::LessEqual,
         TokenType::Greater, TokenType::GreaterEqual}
    );
}

ExprPtr Parser::parseAdditive() {
    return parseLeftAssoc(&Parser::parseTerm, {TokenType::Plus, TokenType::Minus});
}

ExprPtr Parser::parseTerm() {
    return parseLeftAssoc(
        &Parser::parseUnary,
        {TokenType::Star, TokenType::Slash, TokenType::Percent}
    );
}

ExprPtr Parser::parseUnary() {
    if (check(TokenType::Minus) || check(TokenType::Not)) {
        const TokenType op = current_.type;
        const int line = current_.line;
        advance();

        NestingGuard guard(*this);
        ExprPtr operand = parseUnary();

        return std::make_unique<Unary>(line, op, std::move(operand));
    }

    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    const int line = current_.line;

    if (check(TokenType::Integer)) {
        const int value = current_.value;
        advance();

        return std::make_unique<IntLiteral>(line, value);
    }

    if (check(TokenType::Identifier)) {
        std::string name = current_.text;
        advance();

        if (match(TokenType::LParen)) {
            return parseCall(line, std::move(name));
        }

        return std::make_unique<Variable>(line, std::move(name));
    }

    // Parentheses only steer the parse; they leave no node behind.
    if (match(TokenType::LParen)) {
        NestingGuard guard(*this);
        ExprPtr inner = parseExpr();
        expect(TokenType::RParen, "to close '('");

        return inner;
    }

    throw errorAtCurrent("expected an expression");
}

// Called with the callee name and '(' already consumed.
ExprPtr Parser::parseCall(int line, std::string callee) {
    auto call = std::make_unique<Call>(line, std::move(callee));
    NestingGuard guard(*this);

    if (!check(TokenType::RParen)) {
        do {
            call->args.push_back(parseExpr());
        } while (match(TokenType::Comma));
    }

    expect(TokenType::RParen, "after call arguments");

    return call;
}
