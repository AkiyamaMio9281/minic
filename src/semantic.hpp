#pragma once

#include "ast.hpp"

// Checks the rules a grammar cannot express, and resolves every name.
//
// On success each Variable, Assign and VarDecl knows the local slot it
// uses, each Call knows the index of the function it calls, and each
// Function knows how many slots a call frame needs. That is everything
// code generation needs in order to stop looking names up.
//
// Throws CompileError on the first problem found.
void analyze(Program& program);
