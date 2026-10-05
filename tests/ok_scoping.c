int outer(int a) {
    int b = a;

    {
        // c and d take the slots after a and b.
        int c = 1;
        int d = 2;
        b = c + d;
    }

    {
        // This block reuses the slots c and d had, because they are gone.
        int e = 3;
        b = b + e;
    }

    {
        // A different b, in its own slot, shadowing the outer one.
        int b = 9;
        a = b;
    }

    return a + b;
}

int main() {
    return outer(1);
}
