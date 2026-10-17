#pragma once

#include <iosfwd>

#include "bytecode.hpp"

// print writes to out, and so does nothing else: the result main returned
// is the caller's to report.
int run(const BytecodeProgram& program, std::ostream& out);
