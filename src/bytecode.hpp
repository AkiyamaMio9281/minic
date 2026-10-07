#pragma once

#include <iosfwd>
#include <string>
#include <vector>

enum class OpCode {
    PushConst,
    Load,
    Store,
    Dup,
    Pop,
    Neg,
    Not,
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Jump,
    JumpIfFalse,
    JumpIfTrue,
    Call,
    Return,
    ReturnVoid,
};

struct Instruction {
    OpCode op;
    int operand = 0;
    int line = 1;
};

struct BytecodeFunction {
    std::string name;
    bool returnsVoid = false;
    int paramCount = 0;
    int localCount = 0;
    int line = 1;
    std::vector<Instruction> code;
};

struct BytecodeProgram {
    std::vector<BytecodeFunction> functions;
    int mainIndex = -1;
};

const char* opcodeName(OpCode op);
void printBytecode(const BytecodeProgram& program, std::ostream& out);
