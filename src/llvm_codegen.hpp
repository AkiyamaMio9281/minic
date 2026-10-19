#pragma once

#include <iosfwd>

#include "ast.hpp"

// Writes textual LLVM IR for the program.
//
// Must run after analyze(): slot numbers and call targets come from it,
// exactly as they do for the bytecode backend.
//
// The IR carries no target triple, so clang supplies the host's and the
// same .ll works on any machine. Pass -Wno-override-module to silence the
// note clang prints about it.
void emitLlvm(const Program& program, std::ostream& out);
