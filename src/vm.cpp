#include "vm.hpp"

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>
#include <utility>

#include "error.hpp"

namespace {

class VirtualMachine {
public:
    explicit VirtualMachine(const BytecodeProgram& program) : program_(program) {}

    int run() {
        if (program_.mainIndex < 0 || static_cast<std::size_t>(program_.mainIndex) >= program_.functions.size()) {
            throw std::logic_error("bytecode has no valid main function");
        }

        pushFrame(program_.mainIndex, 0, 0);

        while (!frames_.empty()) {
            Frame& frame = frames_.back();
            const BytecodeFunction& function = program_.functions[static_cast<std::size_t>(frame.function)];

            if (frame.ip >= function.code.size()) {
                throw CompileError("runtime", function.line, "reached the end of function '" + function.name + "' without returning");
            }

            const Instruction instruction = function.code[frame.ip++];
            execute(instruction);
        }

        return result_;
    }

private:
    struct Frame {
        int function = -1;
        std::size_t ip = 0;
        std::vector<int> locals;
        std::size_t stackBase = 0;
    };

    int pop(int line) {
        if (stack_.empty()) {
            throw CompileError("runtime", line, "operand stack underflow");
        }
        const int value = stack_.back();
        stack_.pop_back();
        return value;
    }

    void checkSlot(const Frame& frame, int slot, int line) const {
        if (slot < 0 || static_cast<std::size_t>(slot) >= frame.locals.size()) {
            throw CompileError("runtime", line, "invalid local slot");
        }
    }

    void checkTarget(int target, int line) const {
        if (target < 0 || static_cast<std::size_t>(target) >= program_.functions.size()) {
            throw CompileError("runtime", line, "invalid function target");
        }
    }

    void pushFrame(int functionIndex, int argumentCount, int line) {
        checkTarget(functionIndex, line == 0 ? 1 : line);
        const BytecodeFunction& function = program_.functions[static_cast<std::size_t>(functionIndex)];

        if (argumentCount != function.paramCount) {
            throw CompileError("runtime", line == 0 ? 1 : line, "wrong argument count in bytecode call");
        }
        if (stack_.size() < static_cast<std::size_t>(argumentCount)) {
            throw CompileError("runtime", line == 0 ? 1 : line, "not enough arguments on operand stack");
        }

        Frame frame;
        frame.function = functionIndex;
        frame.locals.assign(static_cast<std::size_t>(function.localCount), 0);

        for (int i = argumentCount - 1; i >= 0; --i) {
            frame.locals[static_cast<std::size_t>(i)] = pop(line == 0 ? 1 : line);
        }

        frame.stackBase = stack_.size();
        frames_.push_back(std::move(frame));
    }

    void returnValue(int value) {
        const std::size_t base = frames_.back().stackBase;
        frames_.pop_back();
        stack_.resize(base);

        if (frames_.empty()) {
            result_ = value;
        } else {
            stack_.push_back(value);
        }
    }

    void returnVoid() {
        const std::size_t base = frames_.back().stackBase;
        frames_.pop_back();
        stack_.resize(base);
    }

    void binaryArithmetic(const Instruction& instruction, OpCode op) {
        const int rhs = pop(instruction.line);
        const int lhs = pop(instruction.line);

        if ((op == OpCode::Div || op == OpCode::Mod) && rhs == 0) {
            throw CompileError("runtime", instruction.line, op == OpCode::Div ? "division by zero" : "remainder by zero");
        }

        const long long wideLhs = lhs;
        const long long wideRhs = rhs;
        long long wideResult = 0;

        switch (op) {
            case OpCode::Add: wideResult = wideLhs + wideRhs; break;
            case OpCode::Sub: wideResult = wideLhs - wideRhs; break;
            case OpCode::Mul: wideResult = wideLhs * wideRhs; break;
            case OpCode::Div:
                if (lhs == std::numeric_limits<int>::min() && rhs == -1) {
                    throw CompileError("runtime", instruction.line, "integer division overflow");
                }
                stack_.push_back(lhs / rhs); return;
            case OpCode::Mod:
                if (lhs == std::numeric_limits<int>::min() && rhs == -1) {
                    stack_.push_back(0); return;
                }
                stack_.push_back(lhs % rhs); return;
            default: throw std::logic_error("unexpected arithmetic opcode");
        }

        if (wideResult < std::numeric_limits<int>::min() ||
            wideResult > std::numeric_limits<int>::max()) {
            throw CompileError("runtime", instruction.line, "integer overflow");
        }
        stack_.push_back(static_cast<int>(wideResult));
    }

    void binaryCompare(const Instruction& instruction, OpCode op) {
        const int rhs = pop(instruction.line);
        const int lhs = pop(instruction.line);
        bool result = false;
        switch (op) {
            case OpCode::Equal: result = lhs == rhs; break;
            case OpCode::NotEqual: result = lhs != rhs; break;
            case OpCode::Less: result = lhs < rhs; break;
            case OpCode::LessEqual: result = lhs <= rhs; break;
            case OpCode::Greater: result = lhs > rhs; break;
            case OpCode::GreaterEqual: result = lhs >= rhs; break;
            default: throw std::logic_error("unexpected comparison opcode");
        }
        stack_.push_back(result ? 1 : 0);
    }

    void execute(const Instruction& instruction) {
        Frame& frame = frames_.back();

        switch (instruction.op) {
            case OpCode::PushConst:
                stack_.push_back(instruction.operand);
                return;

            case OpCode::Load:
                checkSlot(frame, instruction.operand, instruction.line);
                stack_.push_back(frame.locals[static_cast<std::size_t>(instruction.operand)]);
                return;

            case OpCode::Store: {
                checkSlot(frame, instruction.operand, instruction.line);
                frame.locals[static_cast<std::size_t>(instruction.operand)] = pop(instruction.line);
                return;
            }

            case OpCode::Dup:
                if (stack_.empty()) {
                    throw CompileError("runtime", instruction.line, "operand stack underflow");
                }
                stack_.push_back(stack_.back());
                return;

            case OpCode::Pop:
                static_cast<void>(pop(instruction.line));
                return;

            case OpCode::Neg: {
                const int value = pop(instruction.line);
                if (value == std::numeric_limits<int>::min()) {
                    throw CompileError("runtime", instruction.line, "integer negation overflow");
                }
                stack_.push_back(-value);
                return;
            }

            case OpCode::Not:
                stack_.push_back(pop(instruction.line) == 0 ? 1 : 0);
                return;

            case OpCode::Add:
            case OpCode::Sub:
            case OpCode::Mul:
            case OpCode::Div:
            case OpCode::Mod:
                binaryArithmetic(instruction, instruction.op);
                return;

            case OpCode::Equal:
            case OpCode::NotEqual:
            case OpCode::Less:
            case OpCode::LessEqual:
            case OpCode::Greater:
            case OpCode::GreaterEqual:
                binaryCompare(instruction, instruction.op);
                return;

            case OpCode::Jump:
                frame.ip = static_cast<std::size_t>(instruction.operand);
                return;

            case OpCode::JumpIfFalse:
                if (pop(instruction.line) == 0) {
                    frame.ip = static_cast<std::size_t>(instruction.operand);
                }
                return;

            case OpCode::JumpIfTrue:
                if (pop(instruction.line) != 0) {
                    frame.ip = static_cast<std::size_t>(instruction.operand);
                }
                return;

            case OpCode::Call: {
                checkTarget(instruction.operand, instruction.line);
                const int argumentCount = program_.functions[static_cast<std::size_t>(instruction.operand)].paramCount;
                pushFrame(instruction.operand, argumentCount, instruction.line);
                return;
            }

            case OpCode::Return:
                returnValue(pop(instruction.line));
                return;

            case OpCode::ReturnVoid:
                returnVoid();
                return;
        }
    }

    const BytecodeProgram& program_;
    std::vector<int> stack_;
    std::vector<Frame> frames_;
    int result_ = 0;
};

}

int run(const BytecodeProgram& program) {
    return VirtualMachine(program).run();
}
