#pragma once

#include "ast.hpp"
#include "bytecode.hpp"

BytecodeProgram generateBytecode(const Program& program);
