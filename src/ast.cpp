#include "ast.hpp"

#include <ostream>

namespace {

class AstPrinter {
public:
    explicit AstPrinter(std::ostream& out) : out_(out) {}

    void program(const Program& program) {
        indent(0) << "Program\n";

        for (const Function& function : program.functions) {
            this->function(function, 1);
        }
    }

private:
    std::ostream& indent(int depth) {
        for (int i = 0; i < depth; ++i) {
            out_ << "  ";
        }

        return out_;
    }

    void function(const Function& function, int depth) {
        indent(depth)
            << "Function "
            << (function.returnsVoid ? "void " : "int ")
            << function.name << "(";

        for (std::size_t i = 0; i < function.params.size(); ++i) {
            if (i > 0) {
                out_ << ", ";
            }

            out_ << function.params[i].name;
        }

        out_ << ")\n";
        stmt(*function.body, depth + 1);
    }

    void stmt(const Stmt& stmt, int depth) {
        switch (stmt.kind) {
            case Stmt::Kind::VarDecl: {
                const auto& decl = static_cast<const VarDecl&>(stmt);
                indent(depth) << "VarDecl " << decl.name << "\n";

                if (decl.init) {
                    expr(*decl.init, depth + 1);
                }

                return;
            }

            case Stmt::Kind::ExprStmt: {
                const auto& exprStmt = static_cast<const ExprStmt&>(stmt);

                if (!exprStmt.expr) {
                    indent(depth) << "Empty\n";
                    return;
                }

                indent(depth) << "ExprStmt\n";
                expr(*exprStmt.expr, depth + 1);
                return;
            }

            case Stmt::Kind::If: {
                const auto& ifStmt = static_cast<const If&>(stmt);
                indent(depth) << "If\n";
                expr(*ifStmt.cond, depth + 1);
                this->stmt(*ifStmt.thenBranch, depth + 1);

                // Else sits inside its If, so which if an else belongs to
                // is visible from the indentation alone.
                if (ifStmt.elseBranch) {
                    indent(depth + 1) << "Else\n";
                    this->stmt(*ifStmt.elseBranch, depth + 2);
                }

                return;
            }

            case Stmt::Kind::While: {
                const auto& whileStmt = static_cast<const While&>(stmt);
                indent(depth) << "While\n";
                expr(*whileStmt.cond, depth + 1);
                this->stmt(*whileStmt.body, depth + 1);
                return;
            }

            case Stmt::Kind::Return: {
                const auto& ret = static_cast<const Return&>(stmt);
                indent(depth) << "Return\n";

                if (ret.value) {
                    expr(*ret.value, depth + 1);
                }

                return;
            }

            case Stmt::Kind::Block: {
                const auto& block = static_cast<const Block&>(stmt);
                indent(depth) << "Block\n";

                for (const StmtPtr& item : block.items) {
                    this->stmt(*item, depth + 1);
                }

                return;
            }
        }
    }

    void expr(const Expr& expr, int depth) {
        switch (expr.kind) {
            case Expr::Kind::IntLiteral: {
                const auto& lit = static_cast<const IntLiteral&>(expr);
                indent(depth) << "Int " << lit.value << "\n";
                return;
            }

            case Expr::Kind::Variable: {
                const auto& var = static_cast<const Variable&>(expr);
                indent(depth) << "Var " << var.name << "\n";
                return;
            }

            case Expr::Kind::Unary: {
                const auto& unary = static_cast<const Unary&>(expr);
                indent(depth) << "Unary " << tokenSpelling(unary.op) << "\n";
                this->expr(*unary.operand, depth + 1);
                return;
            }

            case Expr::Kind::Binary: {
                const auto& binary = static_cast<const Binary&>(expr);
                indent(depth) << "Binary " << tokenSpelling(binary.op) << "\n";
                this->expr(*binary.lhs, depth + 1);
                this->expr(*binary.rhs, depth + 1);
                return;
            }

            case Expr::Kind::Assign: {
                const auto& assign = static_cast<const Assign&>(expr);
                indent(depth) << "Assign " << assign.name << "\n";
                this->expr(*assign.value, depth + 1);
                return;
            }

            case Expr::Kind::Call: {
                const auto& call = static_cast<const Call&>(expr);
                indent(depth) << "Call " << call.callee << "\n";

                for (const ExprPtr& arg : call.args) {
                    this->expr(*arg, depth + 1);
                }

                return;
            }
        }
    }

    std::ostream& out_;
};

}  // namespace

void printAst(const Program& program, std::ostream& out) {
    AstPrinter(out).program(program);
}
