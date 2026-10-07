#include "ast.hpp"
#include "bytecode.hpp"
#include "codegen.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "vm.hpp"

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
    Check,
    Bytecode,
    Run,
};

const char* const USAGE =
    "usage: minic [--tokens | --ast | --check | --bytecode | --run] <file.c>\n"
    "  --tokens    print the token stream\n"
    "  --ast       parse, then print the syntax tree\n"
    "  --check     run semantic analysis, then print the resolved tree (default)\n"
    "  --bytecode  compile and print stack-machine bytecode\n"
    "  --run       compile and execute the program, then print main's result\n";

bool readFile(const char* path, std::string& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::ostringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

void dumpTokens(Lexer& lexer) {
    while (true) {
        const Token token = lexer.nextToken();
        std::cout << std::setw(4) << token.line << "  "
                  << std::left << std::setw(13) << tokenTypeName(token.type)
                  << std::right << '"' << token.text << '"';
        if (token.type == TokenType::Integer) std::cout << "  value=" << token.value;
        std::cout << '\n';
        if (token.type == TokenType::EndOfFile) return;
    }
}

}

int main(int argc, char** argv) {
    Mode mode = Mode::Check;
    const char* path = nullptr;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--tokens") mode = Mode::Tokens;
        else if (arg == "--ast") mode = Mode::Ast;
        else if (arg == "--check") mode = Mode::Check;
        else if (arg == "--bytecode") mode = Mode::Bytecode;
        else if (arg == "--run") mode = Mode::Run;
        else if (arg.empty() || arg[0] == '-' || path != nullptr) {
            std::cerr << USAGE;
            return 2;
        } else path = argv[i];
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
            Program program = parser.parseProgram();
            if (mode == Mode::Ast) {
                printAst(program, std::cout);
            } else {
                analyze(program);
                if (mode == Mode::Check) {
                    printAst(program, std::cout);
                } else {
                    const BytecodeProgram bytecode = generateBytecode(program);
                    if (mode == Mode::Bytecode) printBytecode(bytecode, std::cout);
                    else std::cout << run(bytecode) << '\n';
                }
            }
        }
    } catch (const CompileError& e) {
        std::cerr << e.phase() << " error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
