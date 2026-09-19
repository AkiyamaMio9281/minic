int main() {
    int a = 1;
    int b = 2;
    int c = 3;

    // * binds tighter than +, and - is left-associative: (1 + (2 * 3)) - 4
    a = 1 + 2 * 3 - 4;

    // / and % share a level and associate left: (8 / 2) % 3
    b = 8 / 2 % 3;

    // Parentheses override precedence.
    c = (1 + 2) * 3;

    // = is right-associative: a = (b = (c = 0))
    a = b = c = 0;

    // Unary operators bind tighter than any binary operator.
    c = -a * !b;

    // Comparison binds tighter than &&, which binds tighter than ||.
    // == and != share a level and associate left: (a == c) != 0
    return a < b && b <= c || a == c != 0 && -(-a) > 0;
}
