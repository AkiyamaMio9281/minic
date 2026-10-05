#include "semantic.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "error.hpp"

namespace {

CompileError error(int line, const std::string& message) {
    return CompileError("semantic", line, message);
}

// "1 argument" but "2 arguments".
std::string count(std::size_t n, const char* noun) {
    return std::to_string(n) + " " + noun + (n == 1 ? "" : "s");
}

class Analyzer {
public:
    explicit Analyzer(Program& program)
        : program_(program) {}

    void run() {
        collectFunctions();
        checkMain();

        for (Function& function : program_.functions) {
            analyzeFunction(function);
        }
    }

private:
    // -----------------------------------------------------------------
    // Functions
    // -----------------------------------------------------------------

    // Collecting every function first is what lets a call appear before
    // the definition it refers to.
    void collectFunctions() {
        for (std::size_t i = 0; i < program_.functions.size(); ++i) {
            const Function& function = program_.functions[i];

            if (!functions_.emplace(function.name, static_cast<int>(i)).second) {
                throw error(
                    function.line,
                    "function '" + function.name + "' is already defined"
                );
            }
        }
    }

    void checkMain() {
        const auto it = functions_.find("main");

        if (it == functions_.end()) {
            throw error(1, "no function named 'main'; a program needs a place to start");
        }

        const Function& main = program_.functions[it->second];

        if (main.returnsVoid) {
            throw error(main.line, "main must return int");
        }

        if (!main.params.empty()) {
            throw error(main.line, "main must take no parameters");
        }
    }

    void analyzeFunction(Function& function) {
        current_ = &function;
        scopes_.clear();
        scopes_.emplace_back();
        nextSlot_ = 0;
        maxSlots_ = 0;

        // Parameters live in the body's outermost scope, so
        // "int f(int a) { int a; }" is a redefinition, as it is in C.
        for (const Param& param : function.params) {
            declare(param.name, param.line);
        }

        analyzeItems(*function.body);

        function.localCount = maxSlots_;
        current_ = nullptr;
    }

    // -----------------------------------------------------------------
    // Scopes
    // -----------------------------------------------------------------

    int declare(const std::string& name, int line) {
        if (scopes_.back().count(name) != 0) {
            throw error(line, "'" + name + "' is already declared in this scope");
        }

        if (functions_.count(name) != 0) {
            throw error(line, "'" + name + "' is already the name of a function");
        }

        const int slot = nextSlot_++;

        if (nextSlot_ > maxSlots_) {
            maxSlots_ = nextSlot_;
        }

        scopes_.back().emplace(name, slot);

        return slot;
    }

    // Innermost scope wins, so an inner declaration shadows an outer one.
    int lookup(const std::string& name) const {
        for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
            const auto it = scope->find(name);

            if (it != scope->end()) {
                return it->second;
            }
        }

        return -1;
    }

    int resolve(const std::string& name, int line) const {
        const int slot = lookup(name);

        if (slot >= 0) {
            return slot;
        }

        if (functions_.count(name) != 0) {
            throw error(line, "'" + name + "' is a function; write " + name + "() to call it");
        }

        throw error(line, "'" + name + "' is not declared");
    }

    // -----------------------------------------------------------------
    // Statements
    // -----------------------------------------------------------------

    void analyzeBlock(Block& block) {
        scopes_.emplace_back();
        const int slotsOutside = nextSlot_;

        analyzeItems(block);

        // Sibling blocks hand the same slots back and forth; maxSlots_
        // remembers the peak, which is what a call frame has to hold.
        nextSlot_ = slotsOutside;
        scopes_.pop_back();
    }

    void analyzeItems(Block& block) {
        for (const StmtPtr& item : block.items) {
            analyzeStmt(*item);
        }
    }

    void analyzeStmt(Stmt& stmt) {
        switch (stmt.kind) {
            case Stmt::Kind::VarDecl: {
                auto& decl = static_cast<VarDecl&>(stmt);

                // Declaring before looking at the initializer is what makes
                // "int x = x + 1;" an error instead of a silent reference
                // to an outer x.
                decl.slot = declare(decl.name, decl.line);

                if (decl.init) {
                    declaring_ = decl.slot;
                    analyzeExpr(*decl.init);
                    declaring_ = -1;
                }

                return;
            }

            case Stmt::Kind::ExprStmt: {
                auto& exprStmt = static_cast<ExprStmt&>(stmt);

                if (exprStmt.expr) {
                    // A statement of its own is the one place a void call
                    // is allowed, because nothing consumes its value.
                    analyzeExpr(*exprStmt.expr, true);
                }

                return;
            }

            case Stmt::Kind::If: {
                auto& ifStmt = static_cast<If&>(stmt);
                analyzeExpr(*ifStmt.cond);
                analyzeStmt(*ifStmt.thenBranch);

                if (ifStmt.elseBranch) {
                    analyzeStmt(*ifStmt.elseBranch);
                }

                return;
            }

            case Stmt::Kind::While: {
                auto& whileStmt = static_cast<While&>(stmt);
                analyzeExpr(*whileStmt.cond);
                analyzeStmt(*whileStmt.body);
                return;
            }

            case Stmt::Kind::Return: {
                auto& ret = static_cast<Return&>(stmt);

                if (ret.value) {
                    if (current_->returnsVoid) {
                        throw error(
                            ret.line,
                            "'" + current_->name + "' returns void, so return takes no value"
                        );
                    }

                    analyzeExpr(*ret.value);
                } else if (!current_->returnsVoid) {
                    throw error(
                        ret.line,
                        "'" + current_->name + "' returns int, so return needs a value"
                    );
                }

                return;
            }

            case Stmt::Kind::Block:
                analyzeBlock(static_cast<Block&>(stmt));
                return;
        }
    }

    // -----------------------------------------------------------------
    // Expressions
    // -----------------------------------------------------------------

    // voidAllowed is set only for the outermost call of an expression
    // statement. It is never passed down, so a void call nested anywhere
    // inside an expression is still rejected.
    void analyzeExpr(Expr& expr, bool voidAllowed = false) {
        switch (expr.kind) {
            case Expr::Kind::IntLiteral:
                return;

            case Expr::Kind::Variable: {
                auto& var = static_cast<Variable&>(expr);
                var.slot = resolve(var.name, var.line);
                checkNotOwnInitializer(var.name, var.slot, var.line);
                return;
            }

            case Expr::Kind::Unary:
                analyzeExpr(*static_cast<Unary&>(expr).operand);
                return;

            case Expr::Kind::Binary: {
                auto& binary = static_cast<Binary&>(expr);
                analyzeExpr(*binary.lhs);
                analyzeExpr(*binary.rhs);
                return;
            }

            case Expr::Kind::Assign: {
                auto& assign = static_cast<Assign&>(expr);
                assign.slot = resolve(assign.name, assign.line);
                checkNotOwnInitializer(assign.name, assign.slot, assign.line);
                analyzeExpr(*assign.value);
                return;
            }

            case Expr::Kind::Call: {
                auto& call = static_cast<Call&>(expr);
                call.target = resolveCall(call);

                if (program_.functions[call.target].returnsVoid && !voidAllowed) {
                    throw error(
                        call.line,
                        "'" + call.callee +
                            "' returns void, so its result cannot be used in an expression"
                    );
                }

                for (const ExprPtr& arg : call.args) {
                    analyzeExpr(*arg);
                }

                return;
            }
        }
    }

    int resolveCall(const Call& call) const {
        const auto it = functions_.find(call.callee);

        if (it == functions_.end()) {
            if (lookup(call.callee) >= 0) {
                throw error(call.line, "'" + call.callee + "' is a variable, not a function");
            }

            throw error(call.line, "no function named '" + call.callee + "'");
        }

        const Function& callee = program_.functions[it->second];

        if (call.args.size() != callee.params.size()) {
            throw error(
                call.line,
                "'" + call.callee + "' takes " + count(callee.params.size(), "argument") +
                    " but was given " + std::to_string(call.args.size())
            );
        }

        return it->second;
    }

    // The slot of the variable whose initializer is being analyzed is the
    // newest one allocated, so no outer variable can share it.
    void checkNotOwnInitializer(const std::string& name, int slot, int line) const {
        if (slot == declaring_) {
            throw error(line, "'" + name + "' is used in its own initializer");
        }
    }

    Program& program_;
    std::unordered_map<std::string, int> functions_;
    std::vector<std::unordered_map<std::string, int>> scopes_;

    Function* current_ = nullptr;
    int nextSlot_ = 0;
    int maxSlots_ = 0;
    int declaring_ = -1;
};

}  // namespace

void analyze(Program& program) {
    Analyzer(program).run();
}
