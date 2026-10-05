// even and odd call each other, and each is called before it is defined.
// Names resolve after the whole file is parsed, so the order in which
// functions appear does not matter.
int even(int n) {
    if (n == 0) {
        return 1;
    }

    return odd(n - 1);
}

int odd(int n) {
    if (n == 0) {
        return 0;
    }

    return even(n - 1);
}

int main() {
    return even(10) + odd(7);
}
