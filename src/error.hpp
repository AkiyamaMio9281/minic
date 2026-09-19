#pragma once

#include <stdexcept>
#include <string>
#include <utility>

// Thrown by every compiler phase. `phase` names the phase that failed
// ("lex", "syntax", ...), so main can print "<phase> error: line N: ...".
class CompileError : public std::runtime_error {
public:
    CompileError(std::string phase, int line, const std::string& message)
        : std::runtime_error("line " + std::to_string(line) + ": " + message),
          phase_(std::move(phase)),
          line_(line) {}

    const std::string& phase() const { return phase_; }
    int line() const { return line_; }

private:
    std::string phase_;
    int line_;
};
