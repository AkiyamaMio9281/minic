int fib(int n) {
    if (n < 2) {
        return n;
    }

    return fib(n - 1) + fib(n - 2);
}

int add3(int a, int b, int c) {
    return a + b + c;
}

void noop() {
}

int main() {
    noop();
    return add3(fib(10), add3(1, 2, 3), (4));
}
