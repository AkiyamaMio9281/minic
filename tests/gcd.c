int gcd(int a, int b) {
    while (b != 0) {
        int t = a % b;
        a = b;
        b = t;
    }

    return a;
}

int main() {
    int x = 24;
    int y = 18;

    /* Greatest common divisor, unless the two are equal. */
    if (x >= y && !(x == y)) {
        return gcd(x, y);
    }

    return 0;
}
