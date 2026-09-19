#include "ast.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

namespace {

enum class Mode {
    Tokens,
    Ast,
};

const char* const USAGE =
    "usage: minic [--tokens | --ast] <file.c>\n"
    "  --tokens   print the token stream\n"
    "  --ast      print the syntax tree (default)\n";

bool readFile(const char* path, std::string& out) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();

    return true;
}

void dumpTokens(Lexer& lexer) {
    while (true) {
        const Token token = lexer.nextToken();

        std::cout
            << std::setw(4) << token.line
            << "  "
            << std::left << std::setw(13)
            << tokenTypeName(token.type)
            << std::right
            << '"' << token.text << '"';

        if (token.type == TokenType::Integer) {
            std::cout << "  value=" << token.value;
        }

        std::cout << '\n';

        if (token.type == TokenType::EndOfFile) {
            return;
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    Mode mode = Mode::Ast;
    const char* path = nullptr;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--tokens") {
            mode = Mode::Tokens;
        } else if (arg == "--ast") {
            mode = Mode::Ast;
        } else if (arg.empty() || arg[0] == '-' || path != nullptr) {
            std::cerr << USAGE;
            return 2;
        } else {
            path = argv[i];
        }
    }

    if (path == nullptr) {
        std::cerr << USAGE;
        return 2;
    }

    std::string source;

    if (!readFile(path, source)) {
        std::cerr << "cannot open file: " << path << "\n";
        return 1;
    }

    try {
        Lexer lexer(std::move(source));

        if (mode == Mode::Tokens) {
            dumpTokens(lexer);
        } else {
            Parser parser(lexer);
            const Program program = parser.parseProgram();
            printAst(program, std::cout);
        }
    } catch (const CompileError& e) {
        std::cerr << e.phase() << " error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
