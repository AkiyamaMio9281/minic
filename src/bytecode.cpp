#include "bytecode.hpp"

#include <iomanip>
#include <ostream>

const char* opcodeName(OpCode op) {
    switch (op) {
        case OpCode::PushConst: return "PUSH";
        case OpCode::Load: return "LOAD";
        case OpCode::Store: return "STORE";
        case OpCode::Dup: return "DUP";
        case OpCode::Pop: return "POP";
        case OpCode::Neg: return "NEG";
        case OpCode::Not: return "NOT";
        case OpCode::Add: return "ADD";
        case OpCode::Sub: return "SUB";
        case OpCode::Mul: return "MUL";
        case OpCode::Div: return "DIV";
        case OpCode::Mod: return "MOD";
        case OpCode::Equal: return "EQ";
        case OpCode::NotEqual: return "NE";
        case OpCode::Less: return "LT";
        case OpCode::LessEqual: return "LE";
        case OpCode::Greater: return "GT";
        case OpCode::GreaterEqual: return "GE";
        case OpCode::Jump: return "JMP";
        case OpCode::JumpIfFalse: return "JZ";
        case OpCode::JumpIfTrue: return "JNZ";
        case OpCode::Call: return "CALL";
        case OpCode::Return: return "RET";
        case OpCode::ReturnVoid: return "RET_VOID";
    }
    return "?";
}

namespace {
bool hasOperand(OpCode op) {
    switch (op) {
        case OpCode::PushConst:
        case OpCode::Load:
        case OpCode::Store:
        case OpCode::Jump:
        case OpCode::JumpIfFalse:
        case OpCode::JumpIfTrue:
        case OpCode::Call:
            return true;
        default:
            return false;
    }
}
}

void printBytecode(const BytecodeProgram& program, std::ostream& out) {
    for (std::size_t f = 0; f < program.functions.size(); ++f) {
        const BytecodeFunction& function = program.functions[f];
        out << "Function " << f << " " << function.name
            << " (params " << function.paramCount
            << ", locals " << function.localCount << ")\n";

        for (std::size_t i = 0; i < function.code.size(); ++i) {
            const Instruction& instruction = function.code[i];
            out << "  " << std::setw(4) << i << "  " << opcodeName(instruction.op);
            if (hasOperand(instruction.op)) {
                out << ' ' << instruction.operand;
            }
            out << '\n';
        }
    }
}
