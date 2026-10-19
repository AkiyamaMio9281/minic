// Naive recursive Fibonacci: about 7 million calls for n = 32, which is
// almost entirely call overhead, comparison and addition. fib(32) is
// 2178309.
int fib(int n) {
    if (n < 2) {
        return n;
    }

    return fib(n - 1) + fib(n - 2);
}

int main() {
    print(fib(32));
    return 0;
}
