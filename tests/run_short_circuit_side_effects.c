// The right operand of && and || runs only when the left one leaves the
// answer open. touch() prints its argument, so a call that should have
// been skipped shows up in the output, and 1 / zero would trap.
int touch(int value) {
    print(value);
    return value;
}

int main() {
    int zero = 0;
    int a = 0 && touch(1);
    int b = 1 || touch(2);
    int c = 1 && touch(3);
    int d = 0 || touch(4);
    int e = 0 && 1 / zero;
    int f = 1 || 1 / zero;
    print(a);
    print(b);
    print(c);
    print(d);
    print(e);
    print(f);
    return 0;
}
