#include "lexer.hpp"

#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

const char* tokenTypeToString(TokenType type) {
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

// Used when no source file is given on the command line.
const char* const DEMO_SOURCE = R"(
int gcd(int a, int b) {
    while (b != 0) {
        int t = a % b;
        a = b;
        b = t;
    }

    return a;
}

int main() {
    int x = 24;
    int y = 18;

    /* Greatest common divisor, unless the two are equal. */
    if (x >= y && !(x == y)) {
        return gcd(x, y);
    }

    return 0;
}
)";

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

}  // namespace

int main(int argc, char** argv) {
    std::string source;

    if (argc > 1) {
        if (!readFile(argv[1], source)) {
            std::cerr << "cannot open file: " << argv[1] << "\n";
            return 1;
        }
    } else {
        source = DEMO_SOURCE;
    }

    Lexer lexer(source);

    try {
        while (true) {
            const Token token = lexer.nextToken();

            std::cout
                << std::setw(4) << token.line
                << "  "
                << std::left << std::setw(13)
                << tokenTypeToString(token.type)
                << std::right
                << '"' << token.text << '"';

            if (token.type == TokenType::Integer) {
                std::cout << "  value=" << token.value;
            }

            std::cout << '\n';

            if (token.type == TokenType::EndOfFile) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "lex error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
