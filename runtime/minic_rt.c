/* The native runtime for the LLVM backend.
 *
 * It exists so the generated IR can stay free of string constants and
 * varargs, and so that a trap reports exactly what the VM reports: the
 * two backends are compared against the same .expected files.
 */
#include <stdio.h>
#include <stdlib.h>

/* Copied verbatim from vm.cpp, and indexed by the kind the generated code
 * passes in. The order is part of the contract with llvm_codegen.cpp.
 */
static const char *const TRAP_MESSAGES[] = {
    "integer overflow",
    "division by zero",
    "remainder by zero",
    "integer division overflow",
    "integer negation overflow",
};

/* stdout is flushed first. The test runner merges the two streams, so
 * anything print already wrote has to stay ahead of the error. The VM
 * side gets this for free because C++ ties cerr to cout.
 */
static void fail(int line, const char *message) {
    fflush(stdout);
    fprintf(stderr, "runtime error: line %d: %s\n", line, message);
    exit(1);
}

void minic_print(int value) {
    printf("%d\n", value);
}

void minic_trap(int kind, int line) {
    fail(line, TRAP_MESSAGES[kind]);
}

/* Separate entry point because this is the one message that names the
 * function it came from.
 */
void minic_trap_no_return(const char *name, int line) {
    char message[256];
    snprintf(message, sizeof message,
             "reached the end of function '%s' without returning", name);
    fail(line, message);
}

int mc_main(void);

/* The VM prints what main returned once the run is over, and so does this.
 */
int main(void) {
    const int result = mc_main();
    printf("%d\n", result);
    return 0;
}
