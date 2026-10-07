#include "codegen.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace {

class Generator {
public:
    explicit Generator(const Program& program) : program_(program) {}

    BytecodeProgram run() {
        output_.functions.reserve(program_.functions.size());

        for (std::size_t i = 0; i < program_.functions.size(); ++i) {
            const Function& function = program_.functions[i];
            BytecodeFunction bytecode;
            bytecode.name = function.name;
            bytecode.returnsVoid = function.returnsVoid;
            bytecode.paramCount = static_cast<int>(function.params.size());
            bytecode.localCount = function.localCount;
            bytecode.line = function.line;
            output_.functions.push_back(std::move(bytecode));

            if (function.name == "main") {
                output_.mainIndex = static_cast<int>(i);
            }
        }

        for (std::size_t i = 0; i < program_.functions.size(); ++i) {
            current_ = &output_.functions[i];
            compileBlock(*program_.functions[i].body);

            if (program_.functions[i].returnsVoid) {
                emit(OpCode::ReturnVoid, 0, program_.functions[i].line);
            } else if (program_.functions[i].name == "main") {
                emit(OpCode::PushConst, 0, program_.functions[i].line);
                emit(OpCode::Return, 0, program_.functions[i].line);
            }
        }

        current_ = nullptr;
        return std::move(output_);
    }

private:
    std::size_t emit(OpCode op, int operand, int line) {
        current_->code.push_back({op, operand, line});
        return current_->code.size() - 1;
    }

    void patch(std::size_t instruction, std::size_t target) {
        current_->code[instruction].operand = static_cast<int>(target);
    }

    void compileBlock(const Block& block) {
        for (const StmtPtr& statement : block.items) {
            compileStmt(*statement);
        }
    }

    bool producesValue(const Expr& expr) const {
        if (expr.kind != Expr::Kind::Call) {
            return true;
        }
        const Call& call = static_cast<const Call&>(expr);
        return !program_.functions[static_cast<std::size_t>(call.target)].returnsVoid;
    }

    void compileStmt(const Stmt& statement) {
        switch (statement.kind) {
            case Stmt::Kind::VarDecl: {
                const auto& decl = static_cast<const VarDecl&>(statement);
                if (decl.init) {
                    compileExpr(*decl.init);
                } else {
                    emit(OpCode::PushConst, 0, decl.line);
                }
                emit(OpCode::Store, decl.slot, decl.line);
                return;
            }

            case Stmt::Kind::ExprStmt: {
                const auto& exprStmt = static_cast<const ExprStmt&>(statement);
                if (exprStmt.expr) {
                    const bool value = producesValue(*exprStmt.expr);
                    compileExpr(*exprStmt.expr);
                    if (value) {
                        emit(OpCode::Pop, 0, exprStmt.line);
                    }
                }
                return;
            }

            case Stmt::Kind::If: {
                const auto& ifStmt = static_cast<const If&>(statement);
                compileExpr(*ifStmt.cond);
                const std::size_t jumpElse = emit(OpCode::JumpIfFalse, 0, ifStmt.line);
                compileStmt(*ifStmt.thenBranch);

                if (ifStmt.elseBranch) {
                    const std::size_t jumpEnd = emit(OpCode::Jump, 0, ifStmt.line);
                    patch(jumpElse, current_->code.size());
                    compileStmt(*ifStmt.elseBranch);
                    patch(jumpEnd, current_->code.size());
                } else {
                    patch(jumpElse, current_->code.size());
                }
                return;
            }

            case Stmt::Kind::While: {
                const auto& whileStmt = static_cast<const While&>(statement);
                const std::size_t start = current_->code.size();
                compileExpr(*whileStmt.cond);
                const std::size_t jumpEnd = emit(OpCode::JumpIfFalse, 0, whileStmt.line);
                compileStmt(*whileStmt.body);
                emit(OpCode::Jump, static_cast<int>(start), whileStmt.line);
                patch(jumpEnd, current_->code.size());
                return;
            }

            case Stmt::Kind::Return: {
                const auto& ret = static_cast<const Return&>(statement);
                if (ret.value) {
                    compileExpr(*ret.value);
                    emit(OpCode::Return, 0, ret.line);
                } else {
                    emit(OpCode::ReturnVoid, 0, ret.line);
                }
                return;
            }

            case Stmt::Kind::Block:
                compileBlock(static_cast<const Block&>(statement));
                return;
        }
    }

    void compileLogicalAnd(const Binary& binary) {
        compileExpr(*binary.lhs);
        const std::size_t jumpFalse = emit(OpCode::JumpIfFalse, 0, binary.line);
        compileExpr(*binary.rhs);
        emit(OpCode::Not, 0, binary.line);
        emit(OpCode::Not, 0, binary.line);
        const std::size_t jumpEnd = emit(OpCode::Jump, 0, binary.line);
        patch(jumpFalse, current_->code.size());
        emit(OpCode::PushConst, 0, binary.line);
        patch(jumpEnd, current_->code.size());
    }

    void compileLogicalOr(const Binary& binary) {
        compileExpr(*binary.lhs);
        const std::size_t jumpTrue = emit(OpCode::JumpIfTrue, 0, binary.line);
        compileExpr(*binary.rhs);
        emit(OpCode::Not, 0, binary.line);
        emit(OpCode::Not, 0, binary.line);
        const std::size_t jumpEnd = emit(OpCode::Jump, 0, binary.line);
        patch(jumpTrue, current_->code.size());
        emit(OpCode::PushConst, 1, binary.line);
        patch(jumpEnd, current_->code.size());
    }

    void compileExpr(const Expr& expression) {
        switch (expression.kind) {
            case Expr::Kind::IntLiteral: {
                const auto& literal = static_cast<const IntLiteral&>(expression);
                emit(OpCode::PushConst, literal.value, literal.line);
                return;
            }

            case Expr::Kind::Variable: {
                const auto& variable = static_cast<const Variable&>(expression);
                emit(OpCode::Load, variable.slot, variable.line);
                return;
            }

            case Exppr::Kind::Unary: {
                const auto& unary = static_cast<const Unary&>(expression);
                compileExpr(*unary.operand);
                emit(urary.op == TokenType::Minus ? OpCode::Neg : OpCode::Not, 0, unary.line);
                return;
            }

            case Expr::Kind::Binary: {
                const auto& binary = static_cast<const Binary&>(expression);
                if (binary.op == TokenType::AndAnd) {
                    compileLogicalAnd(binary);
                    return;
                }
                if (binary.op == TokenType::OrOr) {
                    compileLogicalOr(binary);
                    return;
                }

                compileExpr(*binary.lhs);
                compileExpr(*binary.rhs);

                switch (binary.op) {
                    case TokenType::Plus: emit(OpCode::Add, 0, binary.line); break;
                    case TokenType::Minus: emit(OpCode::Sub, 0, binary.line); break;
                    case TokenType::Star: emit(OpCode::Mul, 0, binary.line); break;
                    case TokenType::Slash: emit(OpCode::Div, 0, binary.line); break;
                    case TokenType::Percent: emit(OpCode::Mod, 0, binary.line); break;
                    case TokenType::EqualEqual: emit(OpCode::Equal, 0, binary.line); break;
                    case TokenType::NotEqual: emit(OpCode::NotEqual, 0, binary.line); break;
                    case TokenType::Less: emit(OpCode::Less, 0, binary.line); break;
                    case TokenType::LessEqual: emit(OpCode::LessEqual, 0, binary.line); break;
                    case TokenType::Greater: emit(OpCode::Greater, 0, binary.line); break;
                    case TokenType::GreaterEqual: emit(OpCode::GreaterEqual, 0, binary.line); break;
                    default: throw std::logic_error("unexpected binary operator in code generation");
                }
                return;
            }

            case Expr::Kind::Assign: {
                const auto& assign = static_cast<const Assign&>(expression);
                compileExpr(*assign.value);
                emit(OpCode::Dup, 0, assign.line);
                emit(OpCode::Store, assign.slot, assign.line);
                return;
            }

            case Expr::Kind::Call: {
                const auto& call = static_cast<const Call&>(expression);
                for (const ExprPtr& argument : call.args) {
                    compileExpr(*argument);
                }
                emit(OpCode::Call, call.target, call.line);
                return;
            }
        }
    }

    const Program& program_;
    BytecodeProgram output_;
    BytecodeFunction* current_ = nullptr;
};

}

BytecodeProgram generateBytecode(const Program& program) {
    return Generator(program).run();
}
